/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/**
 * @file
 *   This file implements the handler for data model messages.
 */

#include <system/SystemPacketBuffer.h>

#include "BoltLockManager.h"
#include "DataModelHandler.h"
#include "nrf_log.h"

#include "chip-zcl/chip-zcl.h"

extern "C" {
#include "gen/gen-cluster-id.h"
#include "gen/gen-types.h"
}

using namespace ::chip;

void InitDataModelHandler()
{
    chipZclEndpointInit();
}

void HandleDataModelMessage(System::PacketBuffer * buffer)
{
    ChipZclStatus_t zclStatus = chipZclProcessIncoming((ChipZclBuffer_t *) buffer);
    if (zclStatus == CHIP_ZCL_STATUS_SUCCESS)
    {
        NRF_LOG_INFO("Data model processing success!");
    }
    else
    {
        NRF_LOG_INFO("Data model processing failure: %d", zclStatus);
    }
    System::PacketBuffer::Free(buffer);
}

extern "C" void chipZclPostAttributeChangeCallback(uint8_t endpoint, ChipZclClusterId clusterId, ChipZclAttributeId attributeId,
                                                   uint8_t mask, uint16_t manufacturerCode, uint8_t type, uint8_t size,
                                                   uint8_t * value)
{
    if (clusterId != CHIP_ZCL_CLUSTER_ON_OFF)
    {
        NRF_LOG_INFO("Unknown cluster ID: %d", clusterId);
        return;
    }

    if (attributeId != CHIP_ZCL_CLUSTER_ON_OFF_SERVER_ATTRIBUTE_ON_OFF)
    {
        NRF_LOG_INFO("Unknown attribute ID: %d", attributeId);
        return;
    }

    if (*value)
    {
        BoltLockMgr().InitiateAction(0, BoltLockManager::LOCK_ACTION);
    }
    else
    {
        BoltLockMgr().InitiateAction(0, BoltLockManager::UNLOCK_ACTION);
    }
}
