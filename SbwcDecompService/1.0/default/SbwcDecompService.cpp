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

#include <memory>

#include <mutex>

#include <ExynosGraphicBuffer.h>
#include <hardware/exynos/sbwcwrapper.h>
#include "SbwcDecompService.h"

#define DEFAULT_FRAMERATE   1000

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace SbwcDecompService {
namespace V1_0 {
namespace implementation {

using ::vendor::graphics::ExynosGraphicBufferMeta;

// Methods from ::vendor::samsung_slsi::hardware::SbwcDecompService::V1_0::ISbwcDecompService follow.

Return<int32_t> SbwcDecompService::decode(const hidl_handle &srcHandle, const hidl_handle &dstHandle,
                                          uint32_t attr)
{
    return decodeWithFramerate(srcHandle, dstHandle, attr, DEFAULT_FRAMERATE);
}

Return<int32_t> SbwcDecompService::decodeWithFramerate(const hidl_handle &srcHandle, const hidl_handle &dstHandle,
                                                       uint32_t attr, uint32_t framerate)
{
    auto *srcBH = static_cast<buffer_handle_t>(const_cast<native_handle_t*>(srcHandle.getNativeHandle()));
    if (!srcBH)
        return android::BAD_VALUE;

    return decodeWithCropAndFps(srcHandle, dstHandle, attr,
                                static_cast<unsigned int>(ExynosGraphicBufferMeta::get_width(srcBH)),
                                static_cast<unsigned int>(ExynosGraphicBufferMeta::get_height(srcBH)), framerate);
}

Return<int32_t> SbwcDecompService::decodeWithCrop(const hidl_handle &srcHandle, const hidl_handle &dstHandle,
                                                  uint32_t attr, uint32_t cropWidth, uint32_t cropHeight)
{
    return decodeWithCropAndFps(srcHandle, dstHandle, attr, cropWidth, cropHeight, DEFAULT_FRAMERATE);
}

Return<int32_t> SbwcDecompService::decodeWithCropAndFps(const hidl_handle &srcHandle, const hidl_handle &dstHandle,
                                                        uint32_t attr, uint32_t cropWidth, uint32_t cropHeight, uint32_t framerate)
{
    static std::mutex mDecompMutex;

    ATRACE_CALL();

    std::lock_guard<std::mutex> lock(mDecompMutex);

    static SbwcWrapper *decoder;

    if (!decoder) {
        decoder = new SbwcWrapper();
        if (!decoder)
            return android::NO_MEMORY;
    }

    auto *srcBH = const_cast<native_handle_t*>(srcHandle.getNativeHandle());
    if (!srcBH)
        return android::BAD_VALUE;

    auto *dstBH = const_cast<native_handle_t*>(dstHandle.getNativeHandle());
    if (!dstBH)
        return android::BAD_VALUE;

    if (!decoder->decode(static_cast<void*>(srcBH), static_cast<void*>(dstBH), attr, cropWidth, cropHeight, framerate)) {
        ALOGE("decode is failed");
        return android::BAD_VALUE;
    }

    return android::NO_ERROR;
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

//
}  // namespace implementation
}  // namespace V1_0
}  // namespace SbwcDecompService
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
