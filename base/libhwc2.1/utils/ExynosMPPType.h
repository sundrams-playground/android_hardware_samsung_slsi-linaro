/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef _EXYNOSMPPTYPE_H
#define _EXYNOSMPPTYPE_H

#include <system/graphics.h>
#include <unordered_map>
#include <utils/String8.h>
#include "DeconHeader.h"

#define MIN_WIDTH_USING_VIRTUAL_8K 4096
#define VIRTUAL_8K_WIDTH 7680
#define VIRTUAL_8K_HEIGHT 4320

using namespace android;
/*
 * physical types
 * Resources are sorted by physical type.
 * Resource that has smaller type value is assigned first.
 */
typedef enum {
    MPP_DPP_G = 0,
    MPP_DPP_GF,
    MPP_DPP_VG,
    MPP_DPP_VGS,
    MPP_DPP_VGF,
    MPP_DPP_VGFS,
    MPP_DPP_VGRS,
    MPP_DPP_VGRFS,
    /* DPP count */
    /* If you add DPP, You should increase this value */
    MPP_DPP_NUM = 20,

    MPP_MSC = 20,
    MPP_G2D,
    MPP_P_TYPE_MAX
} mpp_phycal_type_t;

/* logical types */
typedef enum {
    MPP_LOGICAL_DPP_G = 0x01,
    MPP_LOGICAL_DPP_GF = 0x02,
    MPP_LOGICAL_DPP_VG = 0x04,
    MPP_LOGICAL_DPP_VGS = 0x08,
    MPP_LOGICAL_DPP_VGF = 0x10,
    MPP_LOGICAL_DPP_VGFS = 0x20,
    MPP_LOGICAL_DPP_VGRS = 0x40,
    MPP_LOGICAL_DPP_VGRFS = 0x80,
    MPP_LOGICAL_MSC = 0x100,
    MPP_LOGICAL_MSC_YUV = 0x200,
    MPP_LOGICAL_MSC_COMBO = 0x400,
    MPP_LOGICAL_G2D_YUV = 0x1000,
    MPP_LOGICAL_G2D_RGB = 0x2000,
    MPP_LOGICAL_G2D_COMBO = 0x4000,
    /* Virtual MPPs for 8K */
    MPP_LOGICAL_DPP_VGS8K = 0x10000,
    MPP_LOGICAL_DPP_VGFS8K = 0x20000,
    MPP_LOGICAL_DPP_VGRFS8K = 0x40000,
    MPP_LOGICAL_DPP_VIR_MASK = 0xf0000,
    /*
     * Increase MPP_LOGICAL_TYPE_NUM
     * if type is added
     */
    MPP_LOGICAL_TYPE_NUM = 16
} mpp_logical_type_t;

const std::unordered_map<uint32_t, String8> mppTypeStr = {
    {MPP_LOGICAL_DPP_G, String8("DPP_G")},
    {MPP_LOGICAL_DPP_GF, String8("DPP_GF")},
    {MPP_LOGICAL_DPP_VG, String8("DPP_VG")},
    {MPP_LOGICAL_DPP_VGS, String8("DPP_VGS")},
    {MPP_LOGICAL_DPP_VGF, String8("DPP_VGF")},
    {MPP_LOGICAL_DPP_VGFS, String8("DPP_VGFS")},
    {MPP_LOGICAL_DPP_VGRS, String8("DPP_VGRS")},
    {MPP_LOGICAL_DPP_VGRFS, String8("DPP_VGRFS")},
    {MPP_LOGICAL_MSC, String8("MSC")},
    {MPP_LOGICAL_MSC_YUV, String8("MSC_YUV")},
    {MPP_LOGICAL_MSC_COMBO, String8("MSC_COMBO")},
    {MPP_LOGICAL_G2D_YUV, String8("G2D_YUV")},
    {MPP_LOGICAL_G2D_RGB, String8("G2D_RGB")},
    {MPP_LOGICAL_G2D_COMBO, String8("G2D_COMBO")},
    {MPP_LOGICAL_DPP_VGS8K, String8("DPP_VGS8K")},
    {MPP_LOGICAL_DPP_VGFS8K, String8("DPP_VGFS8K")},
    {MPP_LOGICAL_DPP_VGRFS8K, String8("DPP_VGRFS8K")},
};

typedef enum {
    MPP_ATTR_AFBC = 0x00000001,
    MPP_ATTR_SAJC = 0x00000001,
    MPP_ATTR_WINDOW_UPDATE = 0x00000002,
    MPP_ATTR_BLOCK_MODE = 0x00000004,
    MPP_ATTR_USE_CAPA = 0x00000008,
    MPP_ATTR_FLIP_H = 0x00000020,
    MPP_ATTR_FLIP_V = 0x00000040,
    MPP_ATTR_ROT_90 = 0x00000080,
    MPP_ATTR_SCALE = 0x00000800,
    MPP_ATTR_DIM = 0x00001000,
    MPP_ATTR_LAYER_TRANSFORM = 0x00002000,

    MPP_ATTR_WCG = 0x00100000,
    MPP_ATTR_HDR10 = 0x00200000,

    MPP_ATTR_HDR10PLUS = 0x10000000,
} mpp_attr_t;

typedef enum {
    TDM_ATTR_SRAM_AMOUNT,
    TDM_ATTR_SAJC,
    TDM_ATTR_SBWC,
    TDM_ATTR_ITP,  // CSC //
    TDM_ATTR_ROT_90,
    TDM_ATTR_SCALE,
    TDM_ATTR_WCG,
    TDM_ATTR_MAX,
} tdm_attr_t;

const std::unordered_map<tdm_attr_t, String8> HWAttrs = {
    {TDM_ATTR_SRAM_AMOUNT, String8("SRAM")},
    {TDM_ATTR_SAJC, String8("SAJC")},
    {TDM_ATTR_SBWC, String8("SBWC")},
    {TDM_ATTR_ITP, String8("CSC")},  // CSC //
    {TDM_ATTR_ROT_90, String8("ROT")},
    {TDM_ATTR_SCALE, String8("SCALE")},
    {TDM_ATTR_WCG, String8("WCG")},
};

typedef struct feature_support_t {
    mpp_phycal_type_t hwType; /* MPP_DPP_VG, MPP_DPP_VGFS, ... */
    uint64_t attr;
} feature_support_t;

typedef struct transform_map {
    android_transform_t hal_tr;
    uint32_t hwc_tr;
} transform_map_t;

const transform_map_t transform_map_table[] = {
    {HAL_TRANSFORM_FLIP_H, MPP_ATTR_FLIP_H},
    {HAL_TRANSFORM_FLIP_V, MPP_ATTR_FLIP_V},
    {HAL_TRANSFORM_ROT_90, MPP_ATTR_ROT_90},
};

typedef struct dpu_attr_map {
    uint32_t dpp_attr;
    uint32_t hwc_attr = 0;
} dpu_attr_map_t;

typedef struct dpp_channel_map {
    mpp_phycal_type_t type;
    uint32_t index;
    uint32_t idma;  // DECON_IDMA
    decon_idma_type channel;
} dpp_channel_map_t;

#endif
