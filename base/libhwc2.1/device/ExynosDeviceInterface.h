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

#ifndef _EXYNOSDEVICEINTERFACE_H
#define _EXYNOSDEVICEINTERFACE_H

#include "ExynosHWCHelper.h"
#include "ExynosHWCTypes.h"

/* for restriction query */
typedef struct dpu_dpp_info {
    struct dpp_restrictions_info_v2 dpuInfo;
    bool overlap[16] = {
        false,
    };
} dpu_dpp_info_t;

class ExynosDeviceInterface {
  protected:
    // Gathered DPU resctrictions
    dpu_dpp_info_t mDPUInfo;

  public:
    virtual ~ExynosDeviceInterface(){};
    /*
         * set deviceDataSize if deviceData is nullptr
         * set deviceData if it is not nullptr
         */
    virtual void init(void *deviceData, size_t &deviceDataSize) = 0;
    virtual void registerHotplugHandler(ExynosHotplugHandler *param) { mHotplugHandler = param; };
    virtual void registerPanelResetHandler(ExynosPanelResetHandler *param) { mPanelResetHandler = param; };
    virtual void registerVsyncHandler(ExynosDisplayHandler __unused handle,
                                      int32_t __unused mDisplayId){};
    /* This function must be implemented in abstracted class */
    virtual int32_t getRestrictions(struct dpp_restrictions_info_v2 *&restrictions, uint32_t otfMPPSize) = 0;
    virtual void setPrimaryDisplayFd(int32_t displayFd){};

  protected:
    /* Print restriction */
    void printDppRestriction(struct dpp_ch_restriction res);

  public:
    uint32_t mType = INTERFACE_TYPE_NONE;
    ExynosHotplugHandler *mHotplugHandler = NULL;
    ExynosPanelResetHandler *mPanelResetHandler = NULL;
};
#endif  //_EXYNOSDEVICEINTERFACE_H
