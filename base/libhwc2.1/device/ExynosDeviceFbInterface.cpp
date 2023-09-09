/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "ExynosDeviceFbInterface.h"
#include "ExynosHWCHelper.h"
#include "ExynosHWCDebug.h"
#include "DeconFbHeader.h"

extern update_time_info updateTimeInfo;

void ExynosDeviceFbInterface::hwcEventHandlerThread() {
    /** uevent init **/
    char uevent_desc[4096] = {0};
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    uevent_init();

    int32_t cnt_of_event = 1;  // uevent is default
    for (size_t i = 0; i < DISPLAY_COUNT; i++) {
        exynos_display_t display_t = AVAILABLE_DISPLAY_UNITS[i];
        if (display_t.type != HWC_DISPLAY_VIRTUAL)
            cnt_of_event++;
    }

    struct pollfd fds[cnt_of_event];
    fds[0].fd = uevent_get_fd();
    fds[0].events = POLLIN;

    /** Set external display's uevent name **/
    char ueventname_ext[MAX_DEV_NAME + 1];
    ueventname_ext[MAX_DEV_NAME] = '\0';
    sprintf(ueventname_ext, DP_UEVENT_NAME, DP_LINK_NAME);
    ALOGI("uevent name of ext: %s", ueventname_ext);
    int32_t event_index = 0;

    for (auto it = mVsyncHandlers.begin(); it != mVsyncHandlers.end(); it++) {
        if (it->second.mVsyncFd == -1)
            continue;
        int err = lseek(it->second.mVsyncFd, 0, SEEK_SET);

        char buf[4096];
        err = read(it->second.mVsyncFd, buf, sizeof(buf));
        if (err < 0)
            ALOGE("error reading vsync timestamp: %s", strerror(errno));

        fds[event_index + 1].fd = it->second.mVsyncFd;
        fds[event_index + 1].events = POLLPRI;
        event_index++;
    }

    /** Polling events **/
    while (mEventHandlerRunning) {
        int err = poll(fds, cnt_of_event, -1);

        if (err > 0) {
            if (fds[0].revents & POLLIN) {
                uevent_next_event(uevent_desc, sizeof(uevent_desc) - 2);
                bool dp_status = !strcmp(uevent_desc, ueventname_ext);
                if (dp_status)
                    mHotplugHandler->handleHotplug();
            } else {
                size_t index = 0;
                for (auto it = mVsyncHandlers.begin(); it != mVsyncHandlers.end(); it++) {
                    if (it->second.mVsyncFd == -1)
                        continue;
                    if (fds[index + 1].revents & POLLPRI) {
                        for (auto it = mVsyncHandlers.begin(); it != mVsyncHandlers.end(); it++) {
                            if (it->second.mVsyncFd == fds[index + 1].fd)
                                it->second.handle->handleVsync(0);
                        }
                    }
                    index++;
                }
            }
        } else if (err == -1) {
            if (errno == EINTR)
                break;
            ALOGE("error in event thread: %s", strerror(errno));
        }
    }
}

ExynosDeviceFbInterface::ExynosDeviceFbInterface() {
    mType = INTERFACE_TYPE_FB;
}

ExynosDeviceFbInterface::~ExynosDeviceFbInterface() {
    mEventHandlerRunning = false;
    mEventHandlerThread.join();
}

void ExynosDeviceFbInterface::init(void *deviceData, size_t &deviceDataSize) {
    /* Fb interface doesn't have data to pass to display interface */
    deviceDataSize = 0;
}

int32_t ExynosDeviceFbInterface::getRestrictions(struct dpp_restrictions_info_v2 *&restrictions, uint32_t otfMPPSize) {
    struct dpp_restrictions_info dpuInfo = {};
    struct dpp_restrictions_info_v2 *dpuInfoV2 = &mDPUInfo.dpuInfo;
    int32_t ret = 0;

    if (otfMPPSize <= MAX_DPP_CNT) {
        ret = ioctl(mDisplayFd, EXYNOS_DISP_RESTRICTIONS, &dpuInfo);
        if (ret < 0) {
            ALOGI("EXYNOS_DISP_RESTRICTIONS ioctl failed: ret = %d, %s", ret, strerror(errno));
            return ret;
        }
        dpuInfoV2->ver = dpuInfo.ver;
        dpuInfoV2->dpp_cnt = dpuInfo.dpp_cnt;
        for (uint32_t i = 0; i < dpuInfo.dpp_cnt; i++)
            dpuInfoV2->dpp_ch[i] = dpuInfo.dpp_ch[i];
    } else {
        ret = ioctl(mDisplayFd, EXYNOS_DISP_RESTRICTIONS_V2, dpuInfoV2);
        if (ret < 0) {
            ALOGI("EXYNOS_DISP_RESTRICTIONS ioctl failed: ret = %d, %s", ret, strerror(errno));
            return ret;
        }
    }

    /* Convert dpu format to hal format */
    for (int i = 0; i < dpuInfoV2->dpp_cnt; i++) {
        uint32_t halFormatNum = 0;
        u32 halFormats[MAX_FMT_CNT] = {0};
        dpp_restriction r = dpuInfoV2->dpp_ch[i].restriction;
        for (int j = 0; j < r.format_cnt; j++) {
            uint32_t halFormat = DpuFormatToHalFormat(r.format[j]);
            if (halFormat != HAL_PIXEL_FORMAT_EXYNOS_UNDEFINED) {
                halFormats[halFormatNum] = halFormat;
                halFormatNum++;
            }
        }
        dpuInfoV2->dpp_ch[i].restriction.format_cnt = halFormatNum;
        memcpy(dpuInfoV2->dpp_ch[i].restriction.format, halFormats, sizeof(halFormats));
        if (hwcCheckDebugMessages(eDebugAttrSetting))
            printDppRestriction(dpuInfoV2->dpp_ch[i]);
    }
    restrictions = dpuInfoV2;
    return ret;
}

void ExynosDeviceFbInterface::registerHotplugHandler(ExynosHotplugHandler *param) {
    mHotplugHandler = param;
    /** Event handler thread creation **/
    mEventHandlerThread = std::thread(&ExynosDeviceFbInterface::hwcEventHandlerThread, this);
}
