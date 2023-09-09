/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef _DECON_FB_HELPER_H
#define _DECON_FB_HELPER_H

struct decon_color_mode_info {
    int index;
    uint32_t color_mode;
};

struct decon_display_mode {
    uint32_t index;
    uint32_t width;
    uint32_t height;
    uint32_t mm_width;
    uint32_t mm_height;
    uint32_t fps;
    uint32_t group;
};

struct decon_brightness {
    uint32_t current_brightness;
    uint32_t max_brightness;
};

struct decon_hdr_capabilities {
    unsigned int out_types[HDR_CAPABILITIES_NUM];
};
struct decon_hdr_capabilities_info {
    int out_num;
    int max_luminance;
    int max_average_luminance;
    int min_luminance;
};

#define DECON_MATRIX_ELEMENT_NUM 16
struct decon_color_transform_info {
    u32 hint;
    int matrix[DECON_MATRIX_ELEMENT_NUM];
};
struct decon_render_intents_num_info {
    u32 color_mode;
    u32 render_intent_num;
};
struct decon_render_intent_info {
    u32 color_mode;
    u32 index;
    u32 render_intent;
};
struct decon_color_mode_with_render_intent_info {
    u32 color_mode;
    u32 render_intent;
};
struct decon_readback_attribute {
    u32 format;
    u32 dataspace;
};

struct vsync_applied_time_data {
    uint32_t config;
    uint64_t time;
#ifndef USE_NOT_RESERVED_FIELD
    uint32_t reserved[4];
#endif
};

#define MAX_EDID_BLOCK 4
#define EDID_BLOCK_SIZE 128

struct decon_edid_data {
    int size;
    uint8_t edid_data[EDID_BLOCK_SIZE * MAX_EDID_BLOCK];
};

#define S3CFB_SET_VSYNC_INT _IOW('F', 206, __u32)
#define S3CFB_DECON_SELF_REFRESH _IOW('F', 207, __u32)
#define S3CFB_WIN_CONFIG _IOW('F', 209, struct decon_win_config_data)
#define EXYNOS_DISP_INFO _IOW('F', 260, struct decon_disp_info)
#define S3CFB_FORCE_PANIC _IOW('F', 211, __u32)
#define S3CFB_WIN_POSITION _IOW('F', 222, struct decon_user_window)
#define S3CFB_POWER_MODE _IOW('F', 223, __u32)
#define EXYNOS_DISP_RESTRICTIONS _IOW('F', 261, struct dpp_restrictions_info)
#define EXYNOS_DISP_RESTRICTIONS_V2 _IOW('F', 261, struct dpp_restrictions_info_v2)
#define S3CFB_START_CRC _IOW('F', 270, u32)
#define S3CFB_SEL_CRC_BITS _IOW('F', 271, u32)
#define S3CFB_GET_CRC_DATA _IOR('F', 272, u32)
#define EXYNOS_GET_DISPLAYPORT_CONFIG _IOW('F', 300, struct exynos_displayport_data)
#define EXYNOS_SET_DISPLAYPORT_CONFIG _IOW('F', 301, struct exynos_displayport_data)
#define EXYNOS_DPU_DUMP _IOW('F', 302, struct decon_win_config_data)
#define S3CFB_GET_HDR_CAPABILITIES _IOW('F', 400, struct decon_hdr_capabilities)
#define S3CFB_GET_HDR_CAPABILITIES_NUM _IOW('F', 401, struct decon_hdr_capabilities_info)
#define EXYNOS_GET_COLOR_MODE_NUM _IOW('F', 600, __u32)
#define EXYNOS_GET_COLOR_MODE _IOW('F', 601, struct decon_color_mode_info)
#define EXYNOS_SET_COLOR_MODE _IOW('F', 602, __u32)
#define EXYNOS_GET_RENDER_INTENTS_NUM _IOW('F', 610, struct decon_render_intents_num_info)
#define EXYNOS_GET_RENDER_INTENT _IOW('F', 611, struct decon_render_intent_info)
#define EXYNOS_SET_COLOR_TRANSFORM _IOW('F', 612, struct decon_color_transform_info)
#define EXYNOS_SET_COLOR_MODE_WITH_RENDER_INTENT _IOW('F', 613, struct decon_color_mode_with_render_intent_info)
#define EXYNOS_GET_READBACK_ATTRIBUTE _IOW('F', 614, struct decon_readback_attribute)
#define EXYNOS_SET_REPEATER_BUF _IOW('F', 615, bool)

/* HWC 2.3 */
#define EXYNOS_GET_DISPLAY_MODE_NUM _IOW('F', 700, u32)
#define EXYNOS_GET_DISPLAY_MODE _IOW('F', 701, struct decon_display_mode)
#define EXYNOS_SET_DISPLAY_MODE _IOW('F', 702, struct decon_display_mode)
#define EXYNOS_SET_DISPLAY_RESOLUTION _IOW('F', 703, struct decon_display_mode)
#define EXYNOS_SET_DISPLAY_REFRESH_RATE _IOW('F', 704, struct decon_display_mode)
#define EXYNOS_GET_DISPLAY_CURRENT_MODE _IOW('F', 705, u32)

#define EXYNOS_GET_EDID _IOW('F', 800, struct decon_edid_data)

/* For HWC 2.4 */
#define EXYNOS_GET_VSYNC_CHANGE_TIMELINE _IOW('F', 850, struct vsync_applied_time_data)

#endif
