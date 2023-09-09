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

#ifndef _EXYNOSPRIMARYDISPLAYFBINTERFACE_H
#define _EXYNOSPRIMARYDISPLAYFBINTERFACE_H

#include <vector>
#include "ExynosDisplayInterface.h"
#include "DeconHeader.h"
#include "DeconFbHeader.h"
#include "ExynosHWCTypes.h"

class ExynosPrimaryDisplayFbInterface : public ExynosDisplayFbInterface {
  public:
    ExynosPrimaryDisplayFbInterface();
    virtual void init(const DisplayIdentifier &display, void *deviceData,
                      const size_t deviceDataSize) override;
    virtual int32_t setPowerMode(int32_t mode);
    virtual void getDisplayHWInfo(uint32_t &xres, uint32_t &yres, int &psrMode,
                                  std::vector<ResolutionInfo> &resolutionInfo);

    virtual int32_t setLowPowerMode(bool suspend);

    /* For HWC 2.4 APIs */
    virtual int32_t getVsyncAppliedTime(hwc2_config_t __unused configId, displayConfigs &config, int64_t *__unused actualChangeTime);

  protected:
    void alignDSCBlockSize(hwc_rect &merge_rect) override;
    /*
     * Each display panel's resolution have their own slice size that is
     * compression block size.
     * And window update area should be re-calculated to fit as slice size align.
     * This function is used for getting slice size (mDSCHSliceNum, mDSCYSliceSize)
     * from current resolution information based on display configurations
     */
    void updateDSCBlockSize() override;

  protected:
    int32_t mCurPowerMode = HWC_POWER_MODE_OFF;
    std::vector<ResolutionInfo> mResolutionInfo;
};

#endif
