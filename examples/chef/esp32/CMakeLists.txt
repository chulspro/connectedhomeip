#
# Copyright (c) 2021 Project CHIP Authors
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "v1.0")
set(PROJECT_VER_NUMBER 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../common/cmake/idf_flashing.cmake)

set(EXTRA_COMPONENT_DIRS
  "${CMAKE_CURRENT_LIST_DIR}/third_party/connectedhomeip/config/esp32/components"
  "${CMAKE_CURRENT_LIST_DIR}/../../common/QRCode"
)

if(${IDF_TARGET} STREQUAL "esp32")
  list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../common/m5stack-tft/repo/components/tft"
    "${CMAKE_CURRENT_LIST_DIR}/../../common/m5stack-tft/repo/components/spidriver"
    "${CMAKE_CURRENT_LIST_DIR}/../../common/screen-framework")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(${CMAKE_CURRENT_LIST_DIR}/../project_include.cmake)
message(STATUS "Vendor ID " ${CONFIG_DEVICE_VENDOR_ID})
message(STATUS "Product ID " ${CONFIG_DEVICE_PRODUCT_ID})
message(STATUS "Product Name " ${CONFIG_DEVICE_PRODUCT_NAME})
message(STATUS "SW Version String" ${CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING})
idf_build_set_property(COMPILE_OPTIONS "-DCHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID=${CONFIG_DEVICE_VENDOR_ID}" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DCHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID=${CONFIG_DEVICE_PRODUCT_ID}" APPEND)

if(NOT ${CONFIG_DEVICE_PRODUCT_NAME} STREQUAL "")
  idf_build_set_property(COMPILE_OPTIONS "-DCHIP_DEVICE_CONFIG_DEVICE_PRODUCT_NAME=\"${CONFIG_DEVICE_PRODUCT_NAME}\"" APPEND)
endif()

# Forwarding the customized software version string to the ESP32 firmware image
if(NOT ${CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING} STREQUAL "")
  idf_build_set_property(COMPILE_OPTIONS "-DCHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING=\"${CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING}\"" APPEND)
  set(PROJECT_VER ${CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING})
endif()

idf_build_set_property(COMPILE_OPTIONS "-DCHIP_PLATFORM_ESP32=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DCHIP_CONFIG_ENABLE_ACL_EXTENSIONS=1" APPEND)

project(chip-shell)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)

# For the C3, project_include.cmake sets -Wno-format, but does not clear various
# flags that depend on -Wformat
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)

# -Wmaybe-uninitialized has too many false positives, including on std::optional
# and chip::Optional.  Make it nonfatal.
#
# See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)

flashing_script()

if(CONFIG_ENABLE_PW_RPC)
  get_filename_component(CHIP_ROOT ./third_party/connectedhomeip REALPATH)
  include(third_party/connectedhomeip/third_party/pigweed/repo/pw_build/pigweed.cmake)

  pw_set_module_config(pw_rpc_CONFIG pw_rpc.disable_global_mutex_config)

  pw_set_backend(pw_log pw_log_basic)
  pw_set_backend(pw_assert.check pw_assert_log.check_backend)
  pw_set_backend(pw_assert.assert pw_assert.assert_compatibility_backend)
  pw_set_backend(pw_sys_io pw_sys_io.esp32)
  pw_set_backend(pw_trace pw_trace_tokenized)

  add_subdirectory(third_party/connectedhomeip/third_party/pigweed/repo)
  add_subdirectory(third_party/connectedhomeip/third_party/nanopb/repo)
  add_subdirectory(third_party/connectedhomeip/examples/platform/esp32/pw_sys_io)
endif(CONFIG_ENABLE_PW_RPC)
