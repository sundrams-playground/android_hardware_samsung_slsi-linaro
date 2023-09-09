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

#ifndef EXYNOS_EXTERNAL_DISPLAY_H
#define EXYNOS_EXTERNAL_DISPLAY_H

#include "ExynosDisplay.h"
#include <cutils/properties.h>
#include "ExynosDisplayFbInterface.h"
#include "ExynosExternalDisplayFbInterface.h"

#define EXTERNAL_DISPLAY_SKIP_LAYER 0x00000100
#define SKIP_EXTERNAL_FRAME 5

#ifndef PRIMARY_MAIN_EXTERNAL_WINCNT
#define PRIMARY_MAIN_EXTERNAL_WINCNT 2
#endif

class ExynosExternalDisplay : public ExynosDisplay {
  public:
    int mExternalDisplayResolution = DP_RESOLUTION_DEFAULT;  //preset

    /* Methods */
    ExynosExternalDisplay(DisplayIdentifier node);
    ~ExynosExternalDisplay();

    virtual int32_t getDisplayInfo(DisplayInfo &dispInfo) override {
        int32_t ret = ExynosDisplay::getDisplayInfo(dispInfo);
        dispInfo.sinkHdrSupported = mSinkHdrSupported;
        return ret;
    };

    int getDisplayConfigs(uint32_t *outNumConfigs, hwc2_config_t *outConfigs);

    virtual int32_t getHdrCapabilities(uint32_t *outNumTypes, int32_t * /*android_hdr_t*/ outTypes, float *outMaxLuminance,
                                       float *outMaxAverageLuminance, float *outMinLuminance);

    virtual int enable();
    int disable();

    virtual int32_t preProcessValidate(DeviceValidateInfo &validateInfo,
                                       uint64_t &geometryChanged) override;
    virtual int32_t postProcessValidate() override;

    virtual int32_t canSkipValidate();
    virtual int32_t presentDisplay(DevicePresentInfo &presentInfo,
                                   int32_t *outPresentFence);
    virtual int openExternalDisplay();
    virtual void closeExternalDisplay();
    virtual int32_t getActiveConfig(hwc2_config_t *outconfig);
    virtual int32_t startPostProcessing();
    virtual int32_t setClientTarget(
        buffer_handle_t target,
        int32_t acquireFence, int32_t /*android_dataspace_t*/ dataspace,
        uint64_t &geometryFlag);
    virtual int32_t setPowerMode(int32_t /*hwc2_power_mode_t*/ mode,
                                 uint64_t &geometryFlag);
    virtual void initDisplayInterface(uint32_t interfaceType,
                                      void *deviceData, size_t &deviceDataSize) override;
    virtual void init(uint32_t maxWindowNum, ExynosMPP *blendingMPP);
    bool checkRotate();
    bool handleRotate();
    virtual void handleHotplugEvent(bool hpdStatus);
    virtual int32_t getDisplayVsyncPeriod(hwc2_vsync_period_t *__unused outVsyncPeriod);
    virtual int32_t getDisplayVsyncPeriodInternal(hwc2_vsync_period_t *outVsyncPeriod);
    virtual int32_t setActiveConfigWithConstraints(hwc2_config_t __unused config,
                                                   hwc_vsync_period_change_constraints_t *__unused vsyncPeriodChangeConstraints,
                                                   hwc_vsync_period_change_timeline_t *__unused outTimeline,
                                                   bool needUpdateTimeline = true);
    virtual int32_t doDisplayConfigPostProcess();

    virtual bool checkDisplayUnstarted();

    bool mEnabled;
    bool mBlanked;
    bool mVirtualDisplayState;
    bool mSinkHdrSupported;
    Mutex mExternalMutex;

    int mSkipFrameCount;
    int mSkipStartFrame;
    String8 mEventNodeName;

  protected:
    int getDVTimingsIndex(int preset);
    virtual bool getHDRException(ExynosLayer *layer,
                                 DevicePresentInfo &deviceInfo) override;
    virtual bool getHDRException();

  private:
};

#endif
