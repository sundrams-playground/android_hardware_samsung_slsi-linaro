/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __SAMSUNG_DRM_H__
#define __SAMSUNG_DRM_H__
#ifdef __linux__
#include <linux/types.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define DRM_SAMSUNG_HDR_EOTF_LUT_LEN 129
struct hdr_eotf_lut {
  __u16 posx[DRM_SAMSUNG_HDR_EOTF_LUT_LEN];
  __u32 posy[DRM_SAMSUNG_HDR_EOTF_LUT_LEN];
};
#define DRM_SAMSUNG_HDR_OETF_LUT_LEN 33
struct hdr_oetf_lut {
  __u16 posx[DRM_SAMSUNG_HDR_OETF_LUT_LEN];
  __u16 posy[DRM_SAMSUNG_HDR_OETF_LUT_LEN];
};
#define DRM_SAMSUNG_HDR_GM_DIMENS 3
struct hdr_gm_data {
  __u32 coeffs[DRM_SAMSUNG_HDR_GM_DIMENS * DRM_SAMSUNG_HDR_GM_DIMENS];
  __u32 offsets[DRM_SAMSUNG_HDR_GM_DIMENS];
};
#define DRM_SAMSUNG_HDR_TM_LUT_LEN 33
struct hdr_tm_data {
  __u16 coeff_r;
  __u16 coeff_g;
  __u16 coeff_b;
  __u16 rng_x_min;
  __u16 rng_x_max;
  __u16 rng_y_min;
  __u16 rng_y_max;
  __u16 posx[DRM_SAMSUNG_HDR_TM_LUT_LEN];
  __u32 posy[DRM_SAMSUNG_HDR_TM_LUT_LEN];
};
#define DRM_SAMSUNG_CGC_LUT_REG_CNT 2457
struct cgc_lut {
  __u32 r_values[DRM_SAMSUNG_CGC_LUT_REG_CNT];
  __u32 g_values[DRM_SAMSUNG_CGC_LUT_REG_CNT];
  __u32 b_values[DRM_SAMSUNG_CGC_LUT_REG_CNT];
};
#define DRM_SAMSUNG_MATRIX_DIMENS 3
struct exynos_matrix {
  __u16 coeffs[DRM_SAMSUNG_MATRIX_DIMENS * DRM_SAMSUNG_MATRIX_DIMENS];
  __u16 offsets[DRM_SAMSUNG_MATRIX_DIMENS];
};
#ifdef __cplusplus
}
#endif
#endif
