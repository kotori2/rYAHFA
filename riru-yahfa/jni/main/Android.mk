LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Change "libriru_template" to your module name, must start with "libriru_"
LOCAL_MODULE     := libriru_yahfa
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)
LOCAL_CPPFLAGS += $(CPPFLAGS)
LOCAL_LDLIBS += -ldl -llog
LOCAL_LDFLAGS := -Wl

LOCAL_SRC_FILES:= main.cpp HookMain.c trampoline.c

include $(BUILD_SHARED_LIBRARY)