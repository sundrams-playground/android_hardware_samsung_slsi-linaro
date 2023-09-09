/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <cutils/log.h>
#include "ExynosHWCFormat.h"

ExynosFormat PredefinedFormat::exynosFormatP010;
ExynosFormat PredefinedFormat::exynosFormatRgba8;
ExynosFormat PredefinedFormat::exynosFormatRgba10;
ExynosFormat PredefinedFormat::exynosFormatUnDefined;

uint8_t formatToBpp(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format)
            return exynos_format_desc[i].bpp;
    }

    ALOGW("unrecognized pixel format %u", format);
    return 0;
}

bool isFormatRgb(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if (exynos_format_desc[i].type & FORMAT_RGB_MASK)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormatYUV(int format) {
    if (isFormatRgb(format))
        return false;
    return true;
}

bool isFormatSBWC(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if (exynos_format_desc[i].type & FORMAT_SBWC_MASK)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormatYUV420(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            uint32_t yuvType = exynos_format_desc[i].type & FORMAT_YUV_MASK;
            if ((yuvType == YUV420) || (yuvType == P010))
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormatYUV8_2(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if (((exynos_format_desc[i].type & FORMAT_YUV_MASK) == YUV420) &&
                ((exynos_format_desc[i].type & BIT_MASK) == BIT8_2))
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormat10BitYUV420(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            uint32_t yuvType = exynos_format_desc[i].type & FORMAT_YUV_MASK;
            if (((yuvType == YUV420) || (yuvType == P010)) &&
                ((exynos_format_desc[i].type & BIT_MASK) == BIT10))
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormatYUV422(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if ((exynos_format_desc[i].type & FORMAT_YUV_MASK) == YUV422)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormatP010(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if ((exynos_format_desc[i].type & FORMAT_YUV_MASK) == P010)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormat10Bit(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if ((exynos_format_desc[i].type & BIT_MASK) == BIT10)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormat8Bit(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            if ((exynos_format_desc[i].type & BIT_MASK) == BIT8)
                return true;
            else
                return false;
        }
    }
    return false;
}

bool isFormatLossy(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            uint32_t sbwcType = exynos_format_desc[i].type & FORMAT_SBWC_MASK;
            if (sbwcType && !(sbwcType == SBWC_LOSSLESS))
                return true;
            else
                return false;
        }
    }
    return false;
}

bool formatHasAlphaChannel(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format) {
            return exynos_format_desc[i].hasAlpha;
        }
    }
    return false;
}

enum decon_pixel_format halFormatToDpuFormat(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format)
            return exynos_format_desc[i].s3cFormat;
    }
    return DECON_PIXEL_FORMAT_MAX;
}

uint32_t DpuFormatToHalFormat(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].s3cFormat == static_cast<decon_pixel_format>(format))
            return exynos_format_desc[i].halFormat;
    }
    return HAL_PIXEL_FORMAT_EXYNOS_UNDEFINED;
}

const format_description_t *halFormatToExynosFormat(int halFormat,
                                                    uint32_t compressType) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        /* If HAL format is binded different DRM format as compressType,
         * Non compression format should be found first */
        bool compressionSupport = (compressType == COMP_TYPE_NONE) ? true : exynos_format_desc[i].isCompressionSupported(compressType);

        if ((exynos_format_desc[i].halFormat == halFormat) && compressionSupport)
            return &exynos_format_desc[i];
    }
    return nullptr;
}

uint32_t drmFormatToHalFormats(int format, uint32_t *numFormat,
                               uint32_t halFormats[MAX_SAME_HAL_PIXEL_FORMAT]) {
    *numFormat = 0;
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].drmFormat == format) {
            halFormats[*numFormat] = exynos_format_desc[i].halFormat;
            *numFormat = *numFormat + 1;
        }
        if (*numFormat >= MAX_SAME_HAL_PIXEL_FORMAT)
            break;
    }
    return NO_ERROR;
}

int drmFormatToHalFormat(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].drmFormat == format)
            return exynos_format_desc[i].halFormat;
    }
    return HAL_PIXEL_FORMAT_EXYNOS_UNDEFINED;
}

String8 getFormatStr(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format)
            return exynos_format_desc[i].name;
    }
    String8 result;
    result.appendFormat("? %08x", format);
    return result;
}

uint32_t getBufferNumOfFormat(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format)
            return exynos_format_desc[i].bufferNum;
    }
    return 0;
}

uint32_t getTypeOfFormat(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format)
            return exynos_format_desc[i].type;
    }
    return 0;
}

uint32_t getPlaneNumOfFormat(int format) {
    for (unsigned int i = 0; i < FORMAT_MAX_CNT; i++) {
        if (exynos_format_desc[i].halFormat == format)
            return exynos_format_desc[i].planeNum;
    }
    return 0;
}

uint32_t getBytePerPixelOfPrimaryPlane(int format) {
    if (isFormatRgb(format))
        return (formatToBpp(format) / 8);
    else if (isFormat10BitYUV420(format))
        return 2;
    else if (isFormatYUV420(format))
        return 1;
    else
        return 0;
}

ExynosFormat::ExynosFormat()
    : mDescIndex(kDefaultFormatIndex) {
}

ExynosFormat::ExynosFormat(int halFormat, uint32_t compressType) {
    init(halFormat, compressType);
}

void ExynosFormat::init(int halFormat, uint32_t compressType) {
    uint32_t index = 0;
    for (auto &formatDesc : exynos_format_desc) {
        /* If HAL format is binded different DRM format as compressType,
         * Non compression format should be found first */
        bool compressionSupport = (compressType == COMP_TYPE_NONE) ? true : formatDesc.isCompressionSupported(compressType);

        if ((formatDesc.halFormat == halFormat) && compressionSupport) {
            mDescIndex = index;
            return;
        }
        index++;
    }
    ALOGW("unrecognized pixel halFormat %u", halFormat);
    mDescIndex = kDefaultFormatIndex;
}
