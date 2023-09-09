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
#ifndef _EXYNOSHWCFORMAT_H
#define _EXYNOSHWCFORMAT_H

#include <system/graphics.h>
#include <utils/String8.h>
#include <drm_fourcc.h>
#include "exynos_format.h"
#include "DeconCommonHeader.h"

using namespace android;

#define MAX_USE_FORMAT 27
#ifndef P010M_Y_SIZE
#define P010M_Y_SIZE(w, h) (__ALIGN_UP((w), 16) * 2 * __ALIGN_UP((h), 16) + 256)
#endif
#ifndef P010M_CBCR_SIZE
#define P010M_CBCR_SIZE(w, h) ((__ALIGN_UP((w), 16) * 2 * __ALIGN_UP((h), 16) / 2) + 256)
#endif
#ifndef P010_Y_SIZE
#define P010_Y_SIZE(w, h) ((w) * (h)*2)
#endif
#ifndef P010_CBCR_SIZE
#define P010_CBCR_SIZE(w, h) ((w) * (h))
#endif

typedef enum format_type {
    TYPE_UNDEF = 0,

    /* format */
    FORMAT_SHIFT = 0,

    FORMAT_MASK = 0x0000ffff,
    FORMAT_RGB_MASK = 0x0000000f,
    RGB = 0x00000001,

    FORMAT_YUV_MASK = 0x000000f0,
    YUV420 = 0x00000010,
    YUV422 = 0x00000020,
    P010 = 0x00000030,

    FORMAT_SBWC_MASK = 0x00000f00,
    SBWC_LOSSLESS = 0x00000100,
    SBWC_LOSSY_40 = 0x00000200,
    SBWC_LOSSY_50 = 0x00000300,
    SBWC_LOSSY_60 = 0x00000400,
    SBWC_LOSSY_75 = 0x00000500,
    SBWC_LOSSY_80 = 0x00000600,

    /* bit */
    BIT_SHIFT = 16,
    BIT_MASK = 0x000f0000,
    BIT8 = 0x00010000,
    BIT10 = 0x00020000,
    BIT8_2 = 0x00030000,
    BIT16 = 0x00040000,

    /* Compression types */
    /* Caution : This field use bit operations */
    COMP_SHIFT = 20,
    COMP_TYPE_MASK = 0x0ff00000,
    COMP_TYPE_NONE = 0x08000000,
    COMP_TYPE_AFBC = 0x00100000,
    COMP_TYPE_SBWC = 0x00200000,
    COMP_TYPE_SAJC = 0x00400000,

} format_type_t;

typedef struct format_description {
    inline uint32_t getFormat() const { return type & FORMAT_MASK; }
    inline uint32_t getBit() const { return type & BIT_MASK; }
    inline bool isCompressionSupported(uint32_t inType) const {
        return (type & inType) != 0 ? true : false;
    }
    int halFormat;
    decon_pixel_format s3cFormat;
    int drmFormat;
    uint32_t planeNum;
    uint32_t bufferNum;
    uint8_t bpp;
    uint32_t type;
    bool hasAlpha;
    String8 name;
    uint32_t reserved;
} format_description_t;

#define HAL_PIXEL_FORMAT_EXYNOS_UNDEFINED 0
#define DRM_FORMAT_UNDEFINED 0

const format_description_t exynos_format_desc[] = {
    /* RGB */
    {HAL_PIXEL_FORMAT_RGBA_8888, DECON_PIXEL_FORMAT_RGBA_8888, DRM_FORMAT_ABGR8888,
     1, 1, 32, RGB | BIT8 | COMP_TYPE_AFBC | COMP_TYPE_SAJC, true, String8("RGBA_8888"), 0},
    {HAL_PIXEL_FORMAT_RGBX_8888, DECON_PIXEL_FORMAT_RGBX_8888, DRM_FORMAT_XBGR8888,
     1, 1, 32, RGB | BIT8 | COMP_TYPE_AFBC | COMP_TYPE_SAJC, false, String8("RGBx_8888"), 0},
    {HAL_PIXEL_FORMAT_RGB_888, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_BGR888,
     1, 1, 24, RGB | BIT8 | COMP_TYPE_AFBC, false, String8("RGB_888"), 0},
    /* CAUTION : Don't change the HAL_PIXEL_FORMAT_RGB_565's order
     * If HAL format is binded different DRM format as compressionType,
     * Non compression format should locate first */
    {HAL_PIXEL_FORMAT_RGB_565, DECON_PIXEL_FORMAT_RGB_565, DRM_FORMAT_RGB565,
     1, 1, 16, RGB | COMP_TYPE_SAJC, false, String8("RGB_565"), 0},
    {HAL_PIXEL_FORMAT_RGB_565, DECON_PIXEL_FORMAT_RGB_565, DRM_FORMAT_BGR565,
     1, 1, 16, RGB | COMP_TYPE_AFBC, false, String8("RGB_565_AFBC"), 0},
    /************************************/
    {HAL_PIXEL_FORMAT_BGRA_8888, DECON_PIXEL_FORMAT_BGRA_8888, DRM_FORMAT_ARGB8888,
     1, 1, 32, RGB | BIT8 | COMP_TYPE_AFBC | COMP_TYPE_SAJC, true, String8("BGRA_8888"), 0},
    {HAL_PIXEL_FORMAT_RGBA_1010102, DECON_PIXEL_FORMAT_ABGR_2101010, DRM_FORMAT_ABGR2101010,
     1, 1, 32, RGB | BIT10 | COMP_TYPE_AFBC | COMP_TYPE_SAJC, true, String8("RGBA_1010102"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_BGRA8888,
     1, 1, 32, RGB | BIT8 | COMP_TYPE_AFBC, true, String8("EXYNOS_ARGB_8888"), 0},
    {HAL_PIXEL_FORMAT_RGBA_FP16, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_ABGR16161616F,
     1, 1, 64, RGB | BIT16 | COMP_TYPE_AFBC | COMP_TYPE_SAJC, true, String8("RGBA_FP16"), 0},

    /* YUV 420 */
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M, DECON_PIXEL_FORMAT_YUV420M, DRM_FORMAT_UNDEFINED,
     3, 3, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_P_M"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M, DECON_PIXEL_FORMAT_NV12M, DRM_FORMAT_NV12,
     2, 2, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_SP_M"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     2, 2, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_SP_M_TILED"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YV12_M, DECON_PIXEL_FORMAT_YVU420M, DRM_FORMAT_UNDEFINED,
     3, 3, 12, YUV420 | BIT8, false, String8("EXYNOS_YV12_M"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M, DECON_PIXEL_FORMAT_NV21M, DRM_FORMAT_NV21,
     2, 2, 12, YUV420 | BIT8, false, String8("EXYNOS_YCrCb_420_SP_M"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL, DECON_PIXEL_FORMAT_NV21M, DRM_FORMAT_NV21,
     2, 2, 12, YUV420 | BIT8, false, String8("EXYNOS_YCrCb_420_SP_M_FULL"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     3, 1, 0, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_P"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     2, 1, 0, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_SP"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV, DECON_PIXEL_FORMAT_NV12M, DRM_FORMAT_NV12,
     2, 2, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_SP_M_PRIV"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     3, 1, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_PN"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN, DECON_PIXEL_FORMAT_NV12N, DRM_FORMAT_NV12,
     2, 1, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_SPN"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_420_SPN_SBWC_DECOMP, DECON_PIXEL_FORMAT_NV12N, DRM_FORMAT_NV12,
     2, 1, 12, YUV420 | BIT8, false, String8("EXYNOS_420_SPN_SBWC_DECOMP"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     2, 1, 12, YUV420 | BIT8, false, String8("EXYNOS_YCbCr_420_SPN_TILED"), 0},
    {HAL_PIXEL_FORMAT_YCrCb_420_SP, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     2, 1, 12, YUV420 | BIT8, false, String8("YCrCb_420_SP"), 0},
    {HAL_PIXEL_FORMAT_YV12, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     3, 1, 12, YUV420 | BIT8, false, String8("YV12"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B, DECON_PIXEL_FORMAT_NV12M_S10B, DRM_FORMAT_UNDEFINED,
     2, 2, 12, YUV420 | BIT10, false, String8("EXYNOS_YCbCr_420_SP_M_S10B"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B, DECON_PIXEL_FORMAT_NV12N_10B, DRM_FORMAT_UNDEFINED,
     2, 1, 12, YUV420 | BIT10, false, String8("EXYNOS_YCbCr_420_SPN_S10B"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M, DECON_PIXEL_FORMAT_NV12M_P010, DRM_FORMAT_P010,
     2, 2, 24, P010 | BIT10, false, String8("EXYNOS_YCbCr_P010_M"), 0},
    {HAL_PIXEL_FORMAT_YCBCR_P010, DECON_PIXEL_FORMAT_NV12_P010, DRM_FORMAT_P010,
     2, 1, 24, P010 | BIT10, false, String8("EXYNOS_YCbCr_P010"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_P010_N_SBWC_DECOMP, DECON_PIXEL_FORMAT_NV12_P010, DRM_FORMAT_P010,
     2, 1, 24, P010 | BIT10, false, String8("EXYNOS_P010_N_SBWC_DECOMP"), 0},

    /* YUV 422 */
    {HAL_PIXEL_FORMAT_EXYNOS_CbYCrY_422_I, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     0, 0, 0, YUV422 | BIT8, false, String8("EXYNOS_CbYCrY_422_I"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_SP, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     0, 0, 0, YUV422 | BIT8, false, String8("EXYNOS_YCrCb_422_SP"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     0, 0, 0, YUV422 | BIT8, false, String8("EXYNOS_YCrCb_422_I"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_CrYCbY_422_I, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     0, 0, 0, YUV422 | BIT8, false, String8("EXYNOS_CrYCbY_422_I"), 0},

    /* SBWC formats */
    /* NV12, YCbCr, Multi */
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC, DECON_PIXEL_FORMAT_NV12M_SBWC_8B, DRM_FORMAT_NV12,
     2, 2, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCbCr_420_SP_M_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50, DECON_PIXEL_FORMAT_NV12M_SBWC_8B_L50, DRM_FORMAT_NV12,
     2, 2, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSY_50, false, String8("EXYNOS_YCbCr_420_SP_M_SBWC_L50"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75, DECON_PIXEL_FORMAT_NV12M_SBWC_8B_L75, DRM_FORMAT_NV12,
     2, 2, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSY_75, false, String8("EXYNOS_YCbCr_420_SP_M_SBWC_L75"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC, DECON_PIXEL_FORMAT_NV12M_SBWC_10B, DRM_FORMAT_P010,
     2, 2, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCbCr_420_SP_M_10B_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40, DECON_PIXEL_FORMAT_NV12M_SBWC_10B_L40, DRM_FORMAT_P010,
     2, 2, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSY_40, false, String8("EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60, DECON_PIXEL_FORMAT_NV12M_SBWC_10B_L60, DRM_FORMAT_P010,
     2, 2, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSY_60, false, String8("EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80, DECON_PIXEL_FORMAT_NV12M_SBWC_10B_L80, DRM_FORMAT_P010,
     2, 2, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSY_80, false, String8("EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80"), 0},

    /* NV12, YCbCr, Single */
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC, DECON_PIXEL_FORMAT_NV12N_SBWC_8B, DRM_FORMAT_NV12,
     2, 1, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCbCr_420_SPN_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC, DECON_PIXEL_FORMAT_NV12N_SBWC_8B, DRM_FORMAT_NV12,
     2, 1, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCbCr_420_SPN_256_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50, DECON_PIXEL_FORMAT_NV12N_SBWC_8B_L50, DRM_FORMAT_NV12,
     2, 1, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSY_50, false, String8("EXYNOS_YCbCr_420_SPN_SBWC_L50"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75, DECON_PIXEL_FORMAT_NV12N_SBWC_8B_L75, DRM_FORMAT_NV12,
     2, 1, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSY_75, false, String8("EXYNOS_YCbCr_420_SPN_SBWC_75"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC, DECON_PIXEL_FORMAT_NV12N_SBWC_10B, DRM_FORMAT_P010,
     2, 1, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCbCr_420_SPN_10B_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC, DECON_PIXEL_FORMAT_NV12N_SBWC_10B, DRM_FORMAT_P010,
     2, 1, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCbCr_420_SPN_10B_256_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40, DECON_PIXEL_FORMAT_NV12N_SBWC_10B_L40, DRM_FORMAT_P010,
     2, 1, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSY_40, false, String8("EXYNOS_YCbCr_420_SPN_10B_SBWC_L40"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60, DECON_PIXEL_FORMAT_NV12N_SBWC_10B_L60, DRM_FORMAT_P010,
     2, 1, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSY_60, false, String8("EXYNOS_YCbCr_420_SPN_10B_SBWC_L60"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80, DECON_PIXEL_FORMAT_NV12N_SBWC_10B_L80, DRM_FORMAT_P010,
     2, 1, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSY_80, false, String8("EXYNOS_YCbCr_420_SPN_10B_SBWC_L80"), 0},

    /* NV12, YCrCb */
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC, DECON_PIXEL_FORMAT_NV21M_SBWC_8B, DRM_FORMAT_NV12,
     2, 2, 12, YUV420 | BIT8 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCrCb_420_SP_M_SBWC"), 0},
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC, DECON_PIXEL_FORMAT_NV21M_SBWC_10B, DRM_FORMAT_P010_YVU,
     2, 2, 12, P010 | BIT10 | COMP_TYPE_SBWC | SBWC_LOSSLESS, false, String8("EXYNOS_YCrbCb_420_SP_M_10B_SBWC"), 0},

    {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, DECON_PIXEL_FORMAT_MAX, DRM_FORMAT_UNDEFINED,
     0, 0, 0, TYPE_UNDEF, false, String8("ImplDef"), 0}};

#define FORMAT_MAX_CNT sizeof(exynos_format_desc) / sizeof(format_description)

constexpr uint32_t kDefaultFormatIndex = FORMAT_MAX_CNT - 1;
class ExynosFormat {
  public:
    ExynosFormat();
    ExynosFormat(int halFormat, uint32_t compressType);
    ExynosFormat(int halFormat) : ExynosFormat(halFormat, COMP_TYPE_NONE){};
    ExynosFormat(const ExynosFormat &other) : mDescIndex(other.mDescIndex){};

    bool operator==(const int compareHalFormat) const {
        return halFormat() == compareHalFormat;
    };
    bool operator!=(const int compareHalFormat) const {
        return halFormat() != compareHalFormat;
    };
    bool operator==(const ExynosFormat &exynosFormat) const {
        return halFormat() == exynosFormat.halFormat();
    };
    bool operator!=(const ExynosFormat &other) const {
        return mDescIndex != other.mDescIndex;
    };
    ExynosFormat &operator=(const int initialHalFormat) {
        init(initialHalFormat, COMP_TYPE_NONE);
        return *this;
    };

    inline int halFormat() const { return exynos_format_desc[mDescIndex].halFormat; };
    inline uint8_t bpp() const { return exynos_format_desc[mDescIndex].bpp; };
    inline uint32_t bufferNum() const { return exynos_format_desc[mDescIndex].bufferNum; };
    inline uint32_t planeNum() const { return exynos_format_desc[mDescIndex].planeNum; };
    inline decon_pixel_format dpuFormat() const { return exynos_format_desc[mDescIndex].s3cFormat; };
    inline int drmFormat() const { return exynos_format_desc[mDescIndex].drmFormat; };
    inline const String8 &name() const { return exynos_format_desc[mDescIndex].name; };
    inline bool isRgb() const {
        return (exynos_format_desc[mDescIndex].type & FORMAT_RGB_MASK) ? true : false;
    };
    inline bool isYUV() const {
        return isRgb() ? false : true;
    };
    inline bool isYUV420() const {
        uint32_t yuvType = exynos_format_desc[mDescIndex].type & FORMAT_YUV_MASK;
        return ((yuvType == YUV420) || (yuvType == P010)) ? true : false;
    };
    inline bool isYUV422() const {
        return ((exynos_format_desc[mDescIndex].type & FORMAT_YUV_MASK) == YUV422) ? true : false;
    };
    inline bool isYUV8_2() const {
        return (((exynos_format_desc[mDescIndex].type & FORMAT_YUV_MASK) == YUV420) && ((exynos_format_desc[mDescIndex].type & BIT_MASK) == BIT8_2)) ? true : false;
    };
    inline bool is10BitYUV420() const {
        uint32_t yuvType = exynos_format_desc[mDescIndex].type & FORMAT_YUV_MASK;
        return (((yuvType == YUV420) || (yuvType == P010)) && ((exynos_format_desc[mDescIndex].type & BIT_MASK) == BIT10)) ? true : false;
    };
    inline bool isP010() const {
        return ((exynos_format_desc[mDescIndex].type & FORMAT_YUV_MASK) == P010) ? true : false;
    };
    inline bool isSBWC() const {
        return (exynos_format_desc[mDescIndex].type & FORMAT_SBWC_MASK) ? true : false;
    };
    inline uint32_t sbwcType() const {
        return exynos_format_desc[mDescIndex].type & FORMAT_SBWC_MASK;
    }
    inline bool is10Bit() const {
        return ((exynos_format_desc[mDescIndex].type & BIT_MASK) == BIT10) ? true : false;
    };
    inline bool is8Bit() const {
        return ((exynos_format_desc[mDescIndex].type & BIT_MASK) == BIT8) ? true : false;
    };
    inline bool hasAlphaChannel() const {
        return exynos_format_desc[mDescIndex].hasAlpha;
    };
    inline const format_description_t &getFormatDesc() const {
        return exynos_format_desc[mDescIndex];
    };

  private:
    uint32_t mDescIndex;
    void init(int halFormat, uint32_t compressType);
};

class PredefinedFormat {
  public:
    static void init() {
        exynosFormatP010 = ExynosFormat(HAL_PIXEL_FORMAT_YCBCR_P010);
        exynosFormatRgba8 = ExynosFormat(HAL_PIXEL_FORMAT_RGBA_8888);
        exynosFormatRgba10 = ExynosFormat(HAL_PIXEL_FORMAT_RGBA_1010102);
        exynosFormatUnDefined = ExynosFormat();
    };
    static ExynosFormat exynosFormatP010;
    static ExynosFormat exynosFormatRgba8;
    static ExynosFormat exynosFormatRgba10;
    static ExynosFormat exynosFormatUnDefined;
};

const format_description_t *halFormatToExynosFormat(int halFormat,
                                                    uint32_t compressType);
enum decon_pixel_format halFormatToDpuFormat(int format);
uint32_t DpuFormatToHalFormat(int format);
uint32_t S3CFormatToHalFormat(int format);
#define MAX_SAME_HAL_PIXEL_FORMAT 10
uint32_t drmFormatToHalFormats(int format, uint32_t *numFormat, uint32_t halFormats[MAX_SAME_HAL_PIXEL_FORMAT]);
int drmFormatToHalFormat(int format);
uint8_t formatToBpp(int format);

bool isFormatRgb(int format);
bool isFormatYUV(int format);
bool isFormatYUV420(int format);
bool isFormatYUV422(int format);
bool isFormatYUV8_2(int format);
bool isFormat10BitYUV420(int format);
bool isFormatLossy(int format);
bool isFormatSBWC(int format);
bool isFormatP010(int format);
bool isFormat10Bit(int format);
bool isFormat8Bit(int format);
bool formatHasAlphaChannel(int format);
String8 getFormatStr(int format);
uint32_t getBufferNumOfFormat(int format);
uint32_t getTypeOfFormat(int format);
uint32_t getPlaneNumOfFormat(int format);
uint32_t getBytePerPixelOfPrimaryPlane(int format);
#endif
