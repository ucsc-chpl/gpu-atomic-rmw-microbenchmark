LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := atomic_rmw_test.run
LOCAL_C_INCLUDES := ../easyvk/src
LOCAL_SRC_FILES := atomic_rmw_test.cpp ../easyvk/src/easyvk.cpp
LOCAL_LDLIBS    += -lvulkan -llog

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)