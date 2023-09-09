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

/**
 * Project HWC 2.0 Design
 */

#ifndef _EXYNOSRESOURCEMANAGER_H
#define _EXYNOSRESOURCEMANAGER_H

#include <vector>
#include "ExynosDisplay.h"
#include "ExynosHWCHelper.h"
#include "ExynosMPPModule.h"
#include "ExynosResourceRestriction.h"

using namespace android;

class ExynosDisplay;
class ExynosMPP;

#define ASSIGN_RESOURCE_TRY_COUNT 100

#define MAX_OVERLAY_LAYER_NUM 30

struct EnableMPPRequest {
    EnableMPPRequest(uint32_t _physicalType, uint32_t _physicalIndex,
                     uint32_t _logicalIndex, uint32_t _enable) : physicalType(_physicalType), physicalIndex(_physicalIndex),
                                                                 logicalIndex(_logicalIndex), enable(_enable){};
    uint32_t physicalType;
    uint32_t physicalIndex;
    uint32_t logicalIndex;
    uint32_t enable;
};

/* List of logic that used to fill table */
enum {
    UNDEFINED = 0,
    DEFAULT_LOGIC = 1,
};

typedef uint32_t displayEnableMap_t;

#ifndef USE_MODULE_ATTRIBUTE_PRIORITY
const size_t layer_attribute_priority_table[] =
    {
        ATTRIBUTE_DRM,
        ATTRIBUTE_HDR10PLUS,
        ATTRIBUTE_HDR10};
#endif

#ifndef USE_MODULE_SW_FEATURE
const std::map<mpp_phycal_type_t, uint64_t> sw_feature_table =
    {
        {MPP_DPP_G, MPP_ATTR_DIM},
        {MPP_DPP_GF, MPP_ATTR_DIM},
        {MPP_DPP_VG, MPP_ATTR_DIM},
        {MPP_DPP_VGS, MPP_ATTR_DIM},
        {MPP_DPP_VGF, MPP_ATTR_DIM},
        {MPP_DPP_VGFS, MPP_ATTR_DIM},
        {MPP_DPP_VGRFS, MPP_ATTR_DIM},
};
#endif

#ifndef USE_MODULE_DPU_ATTR_MAP
const dpu_attr_map_t dpu_attr_map_table[] =
    {
        {DPP_ATTR_AFBC, MPP_ATTR_AFBC},
        {DPP_ATTR_SAJC, MPP_ATTR_SAJC},
        {DPP_ATTR_BLOCK, MPP_ATTR_BLOCK_MODE},
        {DPP_ATTR_FLIP, MPP_ATTR_FLIP_H | MPP_ATTR_FLIP_V},
        {DPP_ATTR_ROT, MPP_ATTR_ROT_90},
        {DPP_ATTR_SCALE, MPP_ATTR_SCALE},
        {DPP_ATTR_HDR, MPP_ATTR_HDR10},
        {DPP_ATTR_C_HDR, MPP_ATTR_HDR10},
        {DPP_ATTR_HDR10_PLUS, MPP_ATTR_HDR10PLUS},
        {DPP_ATTR_C_HDR10_PLUS, MPP_ATTR_HDR10PLUS},
        {DPP_ATTR_WCG, MPP_ATTR_WCG},
};
#endif

class ExynosMPPVector : public android::SortedVector<ExynosMPP *> {
  public:
    ExynosMPPVector();
    ExynosMPPVector(const ExynosMPPVector &rhs);
    virtual int do_compare(const void *lhs, const void *rhs) const;
};

class ExynosResourceManager {
  public:
    uint32_t mSizeRestrictionCnt[RESTRICTION_MAX];
    restriction_key_t mFormatRestrictions[RESTRICTION_CNT_MAX];
    restriction_size_element_t mSizeRestrictions[RESTRICTION_MAX][RESTRICTION_CNT_MAX];

    android::Vector<uint32_t> mLayerAttributePriority;

    uint32_t mVirtualMPPNum = 0;

    ExynosResourceManager();
    virtual ~ExynosResourceManager();
    DeviceResourceInfo &getDeviceInfo() { return mDeviceInfo; };
    void setDeviceInfo(const DeviceResourceInfo &info) { mDeviceInfo = info; };
    void reloadResourceForHWFC();
    void setTargetDisplayLuminance(uint16_t min, uint16_t max);
    void setTargetDisplayDevice(int device);
    int32_t doPreProcessing();
    int32_t doAllocLutParcels(ExynosDisplay *display);
    int32_t doAllocDstBufs(uint32_t mXres, uint32_t mYres);
    int32_t assignResource(ExynosDisplay *display);
    int32_t assignResourceInternal(ExynosDisplay *display);
    static ExynosMPP *getExynosMPP(uint32_t physicalType, uint32_t physicalIndex);
    ExynosMPP *getExynosMPPForBlending(ExynosDisplay *display);
    static void enableMPP(uint32_t physicalType, uint32_t physicalIndex, uint32_t logicalIndex, uint32_t enable);
    static bool applyEnableMPPRequests();
    int32_t updateSupportedMPPFlag(ExynosDisplay *display);
    int32_t resetResources();
    virtual int32_t preAssignResources();
    /* This function should be implemented by module */
    virtual void preAssignWindows() = 0;
    int32_t resetAssignedResources(ExynosDisplay *display, bool forceReset = false);
    virtual int32_t assignCompositionTarget(ExynosDisplay *display, uint32_t targetType);
    int32_t validateLayer(uint32_t index, ExynosDisplay *display, ExynosLayer *layer);
    int32_t assignLayers(ExynosDisplay *display, uint32_t priority);
    virtual int32_t otfMppReordering(ExynosDisplay *__unused display, ExynosMPPVector __unused &otfMPPs,
                                     struct exynos_image __unused &src, struct exynos_image __unused &dst) { return 0; }

    virtual int32_t assignLayer(ExynosDisplay *display, ExynosLayer *layer, uint32_t layer_index,
                                exynos_image &m2m_out_img, ExynosMPP **m2mMPP, ExynosMPP **otfMPP, uint32_t &overlayInfo);

    /* If product needs specific assign policy, describe at their module codes */
    virtual int32_t checkExceptionScenario(uint64_t &geometryFlag);
    virtual uint32_t getExceptionScenarioFlag(ExynosMPP *mpp);

    virtual int32_t assignWindow(ExynosDisplay *display);
    int32_t updateResourceState();
    static float getResourceUsedCapa(ExynosMPP &mpp);
    int32_t updateExynosComposition(ExynosDisplay *display);
    int32_t updateClientComposition(ExynosDisplay *display);
    int32_t getCandidateM2mMPPOutImages(ExynosDisplay *display,
                                        ExynosLayer *layer, std::vector<exynos_image> &image_lists);
    int32_t setResourcePriority(ExynosDisplay *display);
    int32_t deliverPerformanceInfo(ExynosDisplay *display);
    virtual int32_t prepareResources();
    int32_t finishAssignResourceWork();

    static uint32_t getOtfMPPSize() { return (uint32_t)mOtfMPPs.size(); };
    static ExynosMPP *getOtfMPP(uint32_t index) { return mOtfMPPs[index]; };

    static uint32_t getM2mMPPSize() { return (uint32_t)mM2mMPPs.size(); };
    static ExynosMPP *getM2mMPP(uint32_t index) { return mM2mMPPs[index]; };

    virtual void makeM2MRestrictions();
    virtual void makeAcrylRestrictions(mpp_phycal_type_t type);
    void addFormatRestrictions(restriction_key_t restrictionNode);
    void addSizeRestrictions(uint32_t mppId,
                             restriction_size src_size,
                             restriction_size dst_size,
                             restriction_classification format);
    virtual void makeDPURestrictions(struct dpp_restrictions_info_v2 *dpuInfo,
                                     bool checkOverlap);
    void updateFeatureTable(struct dpp_restrictions_info_v2 *dpuInfo);
    void updateMPPFeature(bool updateOtfMPP);
    void setDPUFeature(ExynosMPP *mpp, uint64_t dpuAttr);

    void updateRestrictions();
    virtual void setVirtualOtfMPPsRestrictions() { return; };

    mpp_phycal_type_t getPhysicalType(int ch) const;
    uint32_t getFeatureTableSize() const;
    const static ExynosMPPVector &getOtfMPPs() { return mOtfMPPs; };

    virtual bool checkLayerAttribute(ExynosDisplay *display, ExynosLayer *layer, size_t type);
    virtual bool isProcessingWithInternals(ExynosDisplay *display, exynos_image *src_img, exynos_image *dst_img);
    virtual int setPreAssignedCapacity(ExynosDisplay *display, ExynosLayer *layer, exynos_image *src_img, exynos_image *dst_img, mpp_phycal_type_t type);
    virtual void preAssignM2MResources();
    float getAssignedCapacity(uint32_t physicalType);

    void setM2MCapa(uint32_t physicalType, uint32_t capa);
    float getM2MCapa(uint32_t physicalType);

    virtual bool hasHDR10PlusMPP();
    ExynosMPP *getHDR10OtfMPP();
    virtual void setM2mTargetCompression();

    virtual bool useCameraException() { return false; };

    virtual uint32_t findAlternateDisplay(ExynosMPP __unused *otfMPP) {
        return getDisplayId(HWC_DISPLAY_PRIMARY, 0);
    };

    void processHdr10plusToHdr10(ExynosDisplay *display);
    virtual void updateSupportWCG();
    virtual bool deviceSupportWCG() { return mDeviceSupportWCG; };

    ExynosMPP *getOtfMPPWithChannel(int ch);
    void initDisplays(android::Vector<ExynosDisplay *> displays,
                      std::map<uint32_t, ExynosDisplay *> displayMap) {
        mDisplays = displays;
        mDisplayMap = displayMap;
    };
    ExynosDisplay *getDisplay(uint32_t displayId) {
        return mDisplayMap[displayId];
    };
    bool isAssignable(ExynosMPP *candidateMPP, ExynosDisplay *display,
                      struct exynos_image &src, struct exynos_image &dst, ExynosMPPSource *mppSrc);
    int32_t printMppsAttr();

    void checkAttrMPP(ExynosDisplay *display);

    /* return 1 if it's needed */
    uint32_t needHWResource(ExynosDisplay *display, exynos_image &srcImg, exynos_image &dstImg, tdm_attr_t attr);
    void refineTransfer(uint32_t &ids);
    bool needHdrProcessing(ExynosDisplay *display, exynos_image &srcImg, exynos_image &dstImg);
    bool needHdrProcessingInternal(ExynosDisplay *display, exynos_image &srcImg, exynos_image &dstImg);

  private:
    int32_t changeLayerFromClientToDevice(ExynosDisplay *display, ExynosLayer *layer,
                                          uint32_t layer_index, exynos_image &m2m_out_img, ExynosMPP *m2mMPP, ExynosMPP *otfMPP);
    int setClientTargetBufferToExynosCompositor(ExynosDisplay *display);

  protected:
    virtual void setFrameRateForPerformance(ExynosMPP &mpp, AcrylicPerformanceRequestFrame *frame);
    static ExynosMPPVector mOtfMPPs;
    static ExynosMPPVector mM2mMPPs;
    static std::vector<EnableMPPRequest> mEnableMPPRequests;
    android::Vector<ExynosDisplay *> mDisplays;
    std::map<uint32_t, ExynosDisplay *> mDisplayMap;
    DeviceResourceInfo mDeviceInfo;
    bool mDeviceSupportWCG = false;

  public:
    virtual bool isHWResourceAvailable(ExynosDisplay __unused *display, ExynosMPP __unused *currentMPP, ExynosMPPSource __unused *mppSrc) { return true; };
    virtual uint32_t setDisplaysTDMInfo() { return 0; };
    virtual uint32_t initDisplaysTDMInfo() { return 0; };
    virtual uint32_t calculateHWResourceAmount(ExynosMPPSource __unused *mppSrc) { return 0; };
    virtual uint32_t calculateHWResourceAmount(ExynosDisplay __unused *display, ExynosMPPSource __unused *mppSrc) { return 0; };
};

#endif  //_EXYNOSRESOURCEMANAGER_H
