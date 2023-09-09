/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VENDOR_SAMSUNG_SLSI_HARDWARE_SBWCDECOMPSERVICE_V1_0_SBWCDECOMPSERVICE_H
#define VENDOR_SAMSUNG_SLSI_HARDWARE_SBWCDECOMPSERVICE_V1_0_SBWCDECOMPSERVICE_H

#include <vendor/samsung_slsi/hardware/SbwcDecompService/1.0/ISbwcDecompService.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <log/log.h>

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace SbwcDecompService {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_handle;
using ::android::hardware::Return;

struct SbwcDecompService : public ISbwcDecompService {
    // Methods from ::vendor::samsung_slsi::hardware::SbwcDecompService::V1_0::ISbwcDecompService follow.
    Return<int32_t> decode(const hidl_handle &srcHandle, const hidl_handle &dstHandle, uint32_t attr) override;
    Return<int32_t> decodeWithFramerate(const hidl_handle &srcHandle, const hidl_handle &dstHandle, uint32_t attr, uint32_t framerate) override;
    Return<int32_t> decodeWithCrop(const hidl_handle &srcHandle, const hidl_handle &dstHandle, uint32_t attr, uint32_t cropWidth, uint32_t cropHeight) override;
    Return<int32_t> decodeWithCropAndFps(const hidl_handle &srcHandle, const hidl_handle &dstHandle, uint32_t attr, uint32_t cropWidth, uint32_t cropHeight, uint32_t framerate) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};


}  // namespace implementation
}  // namespace V1_0
}  // namespace SbwcDecompService
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor

#endif
