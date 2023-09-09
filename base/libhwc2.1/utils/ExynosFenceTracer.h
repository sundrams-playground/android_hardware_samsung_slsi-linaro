/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef _EXYNOSFENCEMANAGER_H
#define _EXYNOSFENCEMANAGER_H

#include "ExynosHWCHelper.h"
#include "ExynosHWCTypes.h"
#include <vector>
#include <unordered_map>
#include <utils/Singleton.h>

#define MAX_FENCE_NAME 64
#define MAX_FENCE_THRESHOLD 500
#define MAX_FENCE_SEQUENCE 3

using namespace android;
typedef enum hwc_fdebug_fence_type {
    FENCE_TYPE_SRC_RELEASE = 1,
    FENCE_TYPE_SRC_ACQUIRE = 2,
    FENCE_TYPE_DST_RELEASE = 3,
    FENCE_TYPE_DST_ACQUIRE = 4,
    FENCE_TYPE_HW_STATE = 7,
    FENCE_TYPE_PRESENT = 8,
    FENCE_TYPE_READBACK_ACQUIRE = 9,
    FENCE_TYPE_READBACK_RELEASE = 10,
    FENCE_TYPE_SRC_DUP_ACQUIRE = 11,
    FENCE_TYPE_ALL = 12,
    FENCE_TYPE_UNDEFINED = 13,
    FENCE_TYPE_MAX
} hwc_fdebug_fence_type_t;

const std::map<int32_t, String8> fence_type_map = {
    {FENCE_TYPE_SRC_RELEASE, String8("SRC_REL")},
    {FENCE_TYPE_SRC_ACQUIRE, String8("SRC_ACQ")},
    {FENCE_TYPE_DST_RELEASE, String8("DST_REL")},
    {FENCE_TYPE_DST_ACQUIRE, String8("DST_ACQ")},
    {FENCE_TYPE_HW_STATE, String8("HWC_STATE")},
    {FENCE_TYPE_PRESENT, String8("PRESENT")},
    {FENCE_TYPE_READBACK_ACQUIRE, String8("READBACK_ACQ")},
    {FENCE_TYPE_READBACK_RELEASE, String8("READBACK_REL")},
    {FENCE_TYPE_SRC_DUP_ACQUIRE, String8("SRC_DUP_ACQ")},
    {FENCE_TYPE_ALL, String8("ALL")},
    {FENCE_TYPE_UNDEFINED, String8("UNDEF")},
};

typedef enum hwc_fdebug_ip_type {
    FENCE_IP_DPP = 0,
    FENCE_IP_MSC = 1,
    FENCE_IP_G2D = 2,
    FENCE_IP_FB = 3,
    FENCE_IP_LAYER = 4,
    FENCE_IP_OUTBUF = 5,
    FENCE_IP_UNDEFINED = 6,
    FENCE_IP_ALL = 7,
    FENCE_IP_MAX
} hwc_fdebug_ip_type_t;

const std::map<int32_t, String8> fence_ip_map = {
    {FENCE_IP_DPP, String8("DPP")},
    {FENCE_IP_MSC, String8("MSC")},
    {FENCE_IP_G2D, String8("G2D")},
    {FENCE_IP_FB, String8("FB")},
    {FENCE_IP_LAYER, String8("LAYER")},
    {FENCE_IP_OUTBUF, String8("OUTBUF")},
    {FENCE_IP_ALL, String8("ALL")},
    {FENCE_IP_UNDEFINED, String8("UNDEF")}};

typedef enum fence_dir {
    FENCE_FROM = 0,
    FENCE_TO,
    FENCE_DUP,
    FENCE_CLOSE,
    FENCE_DIR_MAX
} fence_dir_t;

const std::map<int32_t, String8> fence_dir_map = {
    {FENCE_FROM, String8("From")},
    {FENCE_TO, String8("To")},
    {FENCE_DUP, String8("Dup")},
    {FENCE_CLOSE, String8("Close")},
};

typedef struct fenceTrace {
    uint32_t dir;
    hwc_fdebug_fence_type type;
    hwc_fdebug_ip_type ip;
    struct timeval time;
    int32_t curFlag;
    int32_t usage;
    bool isLast;
} fenceTrace_t;

typedef struct hwc_fence_info {
    uint32_t displayId;
    fenceTrace_t seq[MAX_FENCE_SEQUENCE];
    uint32_t seq_no = 0;
    uint32_t last_dir;
    int32_t usage;
    bool pendingAllowed = false;
    bool leaking = false;
} hwc_fence_info_t;

extern int hwcFenceDebug[FENCE_IP_MAX];
class ExynosFenceTracer : public Singleton<ExynosFenceTracer> {
  public:
    ExynosFenceTracer();
    ~ExynosFenceTracer();
    void writeFenceInfo(uint32_t fd, hwc_fence_info_t *info,
                        hwc_fdebug_fence_type type, hwc_fdebug_ip_type ip,
                        uint32_t direction, bool pendingAllowed);
    void changeFenceInfoState(uint32_t fd, const DisplayIdentifier &display,
                              hwc_fdebug_fence_type type, hwc_fdebug_ip_type ip,
                              uint32_t direction, bool pendingAllowed = false);
    void setFenceInfo(uint32_t fd, const DisplayIdentifier &display,
                      hwc_fdebug_fence_type type, hwc_fdebug_ip_type ip,
                      uint32_t direction, bool pendingAllowed = false);
    struct tm *getLocalTime(struct timeval tv) {
        return (struct tm *)localtime((time_t *)&tv.tv_sec);
    };
    void printLastFenceInfo(uint32_t fd);
    void dumpFenceInfo(int32_t __unused depth);
    bool fenceWarn(uint32_t threshold);
    void resetFenceCurFlag();
    void printFenceTrace(String8 &saveString, struct tm *localTime);
    void printLeakFds();
    bool validateFencePerFrame(const DisplayIdentifier &display);
    void dumpNCheckLeak(int32_t __unused depth);
    int hwc_dup(int fd, const DisplayIdentifier &display,
                hwc_fdebug_fence_type type, hwc_fdebug_ip_type ip, bool pendingAllowed = false);
    int fence_close(int fence, const DisplayIdentifier &displayIdentifier,
                    hwc_fdebug_fence_type type, hwc_fdebug_ip_type ip,
                    const char *debug_str = NULL);
    bool fence_valid(int fence);
    bool validateFences(const DisplayIdentifier &display);
    int32_t saveFenceTrace();
    hwc_fdebug_ip_type_t getM2MIPFenceType(uint32_t physicalType);
    inline int checkFenceDebug(const DisplayIdentifier &display,
                               uint32_t fence_type, uint32_t ip_type, int fence) {
        if ((hwcFenceDebug[ip_type] & (1 << fence_type)) && fence_valid(fence))
            return fence_close(fence, display, FENCE_TYPE_ALL, FENCE_IP_ALL);
        else
            return fence;
    }

    // Variable for fence tracer
    std::unordered_map<int32_t, hwc_fence_info> mFenceInfo;
    uint32_t mFenceLogSize = 0;
};

#endif
