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

#ifndef _EXYNOSDEVICEFBINTERFACE_H
#define _EXYNOSDEVICEFBINTERFACE_H

#include <sys/types.h>
#include <thread>
#include <poll.h>
#include "ExynosDeviceInterface.h"

class ExynosDeviceFbInterface : public ExynosDeviceInterface {
  public:
    ExynosDeviceFbInterface();
    virtual ~ExynosDeviceFbInterface();
    virtual void init(void *deviceData, size_t &deviceDataSize) override;
    virtual int32_t getRestrictions(struct dpp_restrictions_info_v2 *&restrictions, uint32_t otfMPPSize);
    virtual void setPrimaryDisplayFd(int32_t displayFd) { mDisplayFd = displayFd; };
    virtual void registerHotplugHandler(ExynosHotplugHandler *param);
    virtual void hwcEventHandlerThread();
    virtual void registerVsyncHandler(ExynosDisplayHandler handle, int32_t mDisplayId) override {
        if (mVsyncHandlers.find(mDisplayId) == mVsyncHandlers.end())
            mVsyncHandlers.insert(std::make_pair(mDisplayId, handle));
    };

    std::atomic<bool> mEventHandlerRunning = true;

  protected:
    /**
         * Kernel event handling thread (e.g.) Vsync, hotplug, TUI enable events.
         */
    std::thread mEventHandlerThread;
    /* framebuffer fd for main display */
    int mDisplayFd = -1;
    // map <DisplayID, {ExynosVsyncHandler, mVsyncFd}>
    std::map<int32_t, ExynosDisplayHandler> mVsyncHandlers;
};

#endif  //_EXYNOSDEVICEFBINTERFACE_H
