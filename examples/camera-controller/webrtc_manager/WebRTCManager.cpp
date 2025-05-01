/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "WebRTCManager.h"

#include <app/dynamic_server/AccessControl.h>
#include <arpa/inet.h>
#include <commands/interactive/InteractiveCommands.h>
#include <crypto/RandUtils.h>
#include <lib/support/StringBuilder.h>

#include <cstdio>
#include <string>

using namespace chip;
using namespace chip::app;
using namespace std::chrono_literals;

WebRTCManager::WebRTCManager() : mWebRTCRequestorServer(kWebRTCRequesterDynamicEndpointId, mWebRTCRequestorDelegate) {}

WebRTCManager::~WebRTCManager()
{
    // Close the data channel and peer connection if they exist
    if (mDataChannel)
    {
        mDataChannel->close();
        mDataChannel.reset();
    }

    if (mPeerConnection)
    {
        mPeerConnection->close();
        mPeerConnection.reset();
    }
}

void WebRTCManager::Init()
{
    dynamic_server::InitAccessControl();
    mWebRTCRequestorServer.Init();
}

CHIP_ERROR WebRTCManager::SetRemoteDescription(uint16_t webRTCSessionID, const std::string & sdp)
{
    if (!mPeerConnection)
    {
        ChipLogError(Camera, "Cannot set remote description: mPeerConnection is null");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    ChipLogProgress(Camera, "WebRTCManager::SetRemoteDescription");
    mPeerConnection->setRemoteDescription(sdp);

    // Schedule the ProvideICECandidates() call to run asynchronously.
    DeviceLayer::SystemLayer().ScheduleLambda([this, webRTCSessionID]() { ProvideICECandidates(webRTCSessionID); });

    return CHIP_NO_ERROR;
}

CHIP_ERROR WebRTCManager::Connnect(Controller::DeviceCommissioner & commissioner, NodeId nodeId, EndpointId endpointId)
{
    ChipLogProgress(Camera, "Attempting to establish WebRTC connection to node 0x" ChipLogFormatX64 " on endpoint 0x%x",
                    ChipLogValueX64(nodeId), endpointId);

    FabricIndex fabricIndex       = commissioner.GetFabricIndex();
    const FabricInfo * fabricInfo = commissioner.GetFabricTable()->FindFabricWithIndex(fabricIndex);
    VerifyOrReturnError(fabricInfo != nullptr, CHIP_ERROR_INCORRECT_STATE);

    uint64_t fabricId = fabricInfo->GetFabricId();
    ChipLogProgress(Camera, "Commissioner is on Fabric ID 0x" ChipLogFormatX64, ChipLogValueX64(fabricId));

    chip::ScopedNodeId peerId(nodeId, fabricIndex);

    mWebRTCProviderClient.Init(peerId, endpointId, &mWebRTCRequestorServer);

    rtc::InitLogger(rtc::LogLevel::Warning);

    VerifyOrReturnError(initializeSocket(), CHIP_ERROR_INTERNAL);

    // Create the peer connection
    rtc::Configuration config;

    // directly configure stun servers for now
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    config.iceServers.emplace_back("stun:stun1.l.google.com:19302");
    config.enableIceUdpMux = true;

    mPeerConnection = std::make_shared<rtc::PeerConnection>(config);

    mPeerConnection->onLocalDescription([this](rtc::Description description) {
        mLocalDescription = std::string(description);
        ChipLogProgress(Camera, "Local Description:");
        ChipLogProgress(Camera, "%s", mLocalDescription.c_str());
    });

    mPeerConnection->onLocalCandidate([this](rtc::Candidate candidate) {
        std::string candidateStr = candidate.candidate();
        mLocalCandidates.push_back(candidateStr);
        ChipLogProgress(Camera, "Local Candidate:");
        ChipLogProgress(Camera, "%s", candidateStr.c_str());
    });

    mPeerConnection->onStateChange(
        [](rtc::PeerConnection::State state) { ChipLogProgress(Camera, "[PeerConnection State: %d]", static_cast<int>(state)); });

    mPeerConnection->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        ChipLogProgress(Camera, "[Gathering State: %d]", static_cast<int>(state));
    });

    // Add a media track so that controller can receive video
    // TODO get track configuration from allocated streams
    // use fixed values for now
    mMedia = rtc::Description::Video("video", rtc::Description::Direction::RecvOnly);
    mMedia.addH264Codec(VIDEO_H264_CODEC);
    mMedia.setBitrate(VIDEO_BITRATE);
    mTrack = mPeerConnection->addTrack(mMedia);

    mDepacketizer = std::make_shared<rtc::H264RtpDepacketizer>();
    mTrack->setMediaHandler(mDepacketizer);
    mTrack->onFrame([this](rtc::binary message, rtc::FrameInfo frameInfo) {
        // send H264 frames to sock so that a client can pick it up to dispaly it.
        if (sendto(sock, reinterpret_cast<const char *>(message.data()), int(message.size()), 0,
                   reinterpret_cast<const struct sockaddr *>(&socket_address), sizeof(socket_address)) == -1)
        {
            ChipLogError(Camera, "Failed to send video frame");
        }
    });

    // Create a data channel for this offerer
    mDataChannel = mPeerConnection->createDataChannel("test");

    if (mDataChannel)
    {
        mDataChannel->onOpen(
            [&]() { ChipLogProgress(Camera, "[DataChannel open: %s]", mDataChannel ? mDataChannel->label().c_str() : "unknown"); });

        mDataChannel->onClosed([&]() {
            ChipLogProgress(Camera, "[DataChannel closed: %s]", mDataChannel ? mDataChannel->label().c_str() : "unknown");
        });

        mDataChannel->onMessage([](auto data) {
            if (std::holds_alternative<std::string>(data))
            {
                ChipLogProgress(Camera, "[Received: %s]", std::get<std::string>(data).c_str());
            }
        });
    }

    mPeerConnection->setLocalDescription();

    return CHIP_NO_ERROR;
}

bool WebRTCManager::initializeSocket()
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        ChipLogError(Camera, "Socket creation failed");
        return false;
    }
    socket_address                 = {};
    socket_address.sin_family      = AF_INET;
    socket_address.sin_addr.s_addr = inet_addr(LOCALHOST_IP);
    socket_address.sin_port        = htons(VIDEO_STREAM_DEST_PORT);
    // Since this is an outbound only socket, no need to bind
    return true;
}

CHIP_ERROR WebRTCManager::ProvideOffer(DataModel::Nullable<uint16_t> webRTCSessionID,
                                       Clusters::WebRTCTransportProvider::StreamUsageEnum streamUsage)
{
    ChipLogProgress(Camera, "Sending ProvideOffer command to the peer device");

    CHIP_ERROR err =
        mWebRTCProviderClient.ProvideOffer(webRTCSessionID, mLocalDescription, streamUsage, kWebRTCRequesterDynamicEndpointId,
                                           MakeOptional(DataModel::NullNullable), // "Null" for video
                                           MakeOptional(DataModel::NullNullable), // "Null" for audio
                                           NullOptional,                          // Omit ICEServers (Optional not present)
                                           NullOptional                           // Omit ICETransportPolicy (Optional not present)
        );

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Camera, "Failed to send ProvideOffer: %" CHIP_ERROR_FORMAT, err.Format());
    }

    return err;
}

CHIP_ERROR WebRTCManager::ProvideICECandidates(uint16_t webRTCSessionID)
{
    ChipLogProgress(Camera, "Sending ProvideICECandidates command to the peer device");

    if (mLocalCandidates.empty())
    {
        ChipLogError(Camera, "No local ICE candidates to send");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    // Convert mLocalCandidates (std::vector<std::string>) into a list of CharSpans.
    std::vector<chip::CharSpan> candidateSpans;
    candidateSpans.reserve(mLocalCandidates.size());
    for (const auto & candidate : mLocalCandidates)
    {
        candidateSpans.push_back(chip::CharSpan(candidate.c_str(), static_cast<uint16_t>(candidate.size())));
    }

    auto ICECandidates = chip::app::DataModel::List<const chip::CharSpan>(candidateSpans.data(), candidateSpans.size());

    CHIP_ERROR err = mWebRTCProviderClient.ProvideICECandidates(webRTCSessionID, ICECandidates);

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Camera, "Failed to send ProvideICECandidates: %" CHIP_ERROR_FORMAT, err.Format());
    }

    return err;
}
