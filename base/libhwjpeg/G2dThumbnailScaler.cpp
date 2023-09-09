/*
 * Copyright (C) 2019 Samsung Electronics Co.,LTD.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <linux/videodev2.h>

#include <system/graphics.h>
#include <log/log.h>

#include <exynos_format.h>
#include <hardware/hwcomposer2.h>

#include "G2dThumbnailScaler.h"

const static unsigned int v4l2_to_hal_format_table[][2] = {
    {V4L2_PIX_FMT_NV12, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP},
    {V4L2_PIX_FMT_NV21, HAL_PIXEL_FORMAT_YCrCb_420_SP},
    {V4L2_PIX_FMT_NV12M, HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M},
    {V4L2_PIX_FMT_NV21M, HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M},
};

static unsigned int getHalFormat(unsigned int v4l2fmt)
{
    for (auto &ent: v4l2_to_hal_format_table)
        if (v4l2fmt == ent[0])
            return ent[1];

    ALOGE("V4L2 format %#x is not found", v4l2fmt);
    return ~0;
}

G2dThumbnailScaler::G2dThumbnailScaler()
{
    mCompositor = Acrylic::createBlter();
    if (mCompositor == nullptr) {
        ALOGE("G2dThumbnailScaler init failed");
    } else {
        mSource = mCompositor->createLayer();
    }
}

G2dThumbnailScaler::~G2dThumbnailScaler()
{
    if (mSource != nullptr)
        delete mSource;

    if (mCompositor != nullptr)
        delete mCompositor;
}

bool G2dThumbnailScaler::SetSrcImage(unsigned int width, unsigned int height, unsigned int v4l2_format)
{
    mSrcWidth = width;
    mSrcHeight = height;

    if (useM2mScaler())
        return mThumbnailScaler.SetSrcImage(width, height, v4l2_format);

    if (!mSource->setImageDimension(width, height))
        return false;

    if (!mSource->setImageType(getHalFormat(v4l2_format), HAL_DATASPACE_V0_JFIF))
        return false;

    return true;
}

bool G2dThumbnailScaler::SetDstImage(unsigned int width, unsigned int height, unsigned int v4l2_format)
{
    if (useM2mScaler())
        return mThumbnailScaler.SetDstImage(width, height, v4l2_format);

    if (!mCompositor->setCanvasDimension(width, height))
        return false;

    if (!mCompositor->setCanvasImageType(getHalFormat(v4l2_format), HAL_DATASPACE_V0_JFIF))
        return false;

    return true;
}

bool G2dThumbnailScaler::RunStream(int srcBuf[SCALER_MAX_PLANES], int srcLen[SCALER_MAX_PLANES],
                                     int dstBuf, size_t dstLen)
{
    if (useM2mScaler())
        return mThumbnailScaler.RunStream(srcBuf, srcLen, dstBuf, dstLen);

    int dBuf[SCALER_MAX_PLANES]{dstBuf, 0, 0};
    size_t dLen[SCALER_MAX_PLANES]{dstLen, 0, 0};

    size_t sLen[SCALER_MAX_PLANES];
    for (size_t i = 0; i < SCALER_MAX_PLANES; i++) {
        sLen[i] = static_cast<size_t>(srcLen[i]);
    }

    if (!mSource->setImageBuffer(srcBuf, sLen, 1))
        return false;

    if (!mCompositor->setCanvasBuffer(dBuf, dLen, 1))
        return false;

    return mCompositor->execute();
}

bool G2dThumbnailScaler::RunStream(char __unused *srcBuf[SCALER_MAX_PLANES], int __unused srcLen[SCALER_MAX_PLANES],
                                     int __unused dstBuf, size_t __unused dstLen)
{
    ALOGE("G2dScaler does not support userptr buffers");
    return false;
}
