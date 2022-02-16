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
#ifndef EXYNOS_PRIMARY_DISPLAY_H
#define EXYNOS_PRIMARY_DISPLAY_H

#include <vector>
#include "ExynosDisplay.h"
#include "ExynosDisplayFbInterface.h"
#include "ExynosHWCTypes.h"

class ExynosMPPModule;

class ExynosPrimaryDisplay : public ExynosDisplay {
  public:
    /* Methods */
    ExynosPrimaryDisplay(DisplayIdentifier node);
    ~ExynosPrimaryDisplay();
    virtual void initDisplayInterface(uint32_t interfaceType,
                                      void *deviceData, size_t &deviceDataSize) override;
    virtual void init(uint32_t maxWindowNum, ExynosMPP *blendingMPP);

    virtual void checkLayersForSettingDR();

    virtual int32_t setBootDisplayConfig(int32_t config) override;
    virtual int32_t clearBootDisplayConfig() override;
    virtual int32_t getPreferredDisplayConfigInternal(int32_t *outConfig) override;

    std::vector<ResolutionInfo> mResolutionInfo;

  protected:
    /* setPowerMode(int32_t mode)
         * Descriptor: HWC2_FUNCTION_SET_POWER_MODE
         * Parameters:
         *   mode - hwc2_power_mode_t and ext_hwc2_power_mode_t
         *
         * Returns HWC2_ERROR_NONE or the following error:
         *   HWC2_ERROR_UNSUPPORTED when DOZE mode not support
         */
    virtual int32_t setPowerMode(int32_t /*hwc2_power_mode_t*/ mode,
                                 uint64_t &geometryFlag) override;
    virtual bool getHDRException(ExynosLayer *__unused layer,
                                 DevicePresentInfo &deviceInfo) override;

    int32_t getClientTargetSupport(
        uint32_t width, uint32_t height,
        int32_t /*android_pixel_format_t*/ format,
        int32_t /*android_dataspace_t*/ dataspace);

  private:
    int32_t setPowerOn();
    int32_t setPowerOff();
    int32_t setPowerDoze(bool suspend);
};

#endif
