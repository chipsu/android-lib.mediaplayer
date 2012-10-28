#
LOCAL_PATH :=  $(call my-dir)
FFMPEG_PATH := $(LOCAL_PATH)/../../lib.ffmpeg

include $(CLEAR_VARS)

LOCAL_MODULE     := libmediaplayer
LOCAL_SRC_FILES  := JavaAPI.cpp
LOCAL_SRC_FILES  += icecore/Profiler.cpp
LOCAL_SRC_FILES  += icegl/Program.cpp
LOCAL_SRC_FILES  += icegl/Texture.cpp
LOCAL_SRC_FILES  += ffmp/MediaPlayer.cpp
LOCAL_LDLIBS     := -lgcc -llog -lffmpeg -lGLESv2
#LOCAL_LDLIBS     += -ljnigraphics
LOCAL_C_INCLUDES += $(FFMPEG_PATH)/jni/
LOCAL_LDLIBS     += -L$(FFMPEG_PATH)/libs/$(TARGET_ARCH_ABI)

ifdef ICE_OMX
	LOCAL_CPPFLAGS += -DICE_OMX
	LOCAL_SRC_FILES += libstagefright.cpp
	LOCAL_LDLIBS += -L$(FFMPEG_PATH)/android-libs
	LOCAL_LDLIBS += -lmedia -lbinder -lstagefright -lutils
endif

include $(BUILD_SHARED_LIBRARY)

