LOCAL_PATH := $(call my-dir)

$(warning #############################################)
$(warning ########       GeoTrans10       #############)
$(warning #############################################)

include $(CLEAR_VARS)
LOCAL_MODULE := libGeoTrans10
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_64 := lib64/libGeoTrans10.so
LOCAL_SRC_FILES_32 := lib/libGeoTrans10.so
LOCAL_MULTILIB := both
LOCAL_SHARED_LIBRARIES := android.hidl.memory@1.0 libc++ libc libcutils libdl libhidlbase libhidlmemory libhidltransport libion liblog libm libutils vendor.samsung_slsi.hardware.geoTransService@1.0
include $(BUILD_PREBUILT)
