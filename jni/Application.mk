APP_ABI := armeabi-v7a
APP_STL := stlport_static
#APP_STL := gnustl_static
APP_OPTIM := release
APP_CPPFLAGS := -O2 -mfpu=neon -mfloat-abi=softfp -march=armv7-a -ftree-vectorize -ffast-math -fomit-frame-pointer -fno-rtti -fno-exceptions
ifdef ICE_OMX
	ifndef ICE_OMX_PATH
		ICE_ANDROID_SRC_PATH=/home/mu/Temp/android-src2
	endif
	APP_CPPFLAGS += -I$(ICE_ANDROID_SRC_PATH)/platform_system_core/include
	APP_CPPFLAGS += -I$(ICE_ANDROID_SRC_PATH)/platform_frameworks_base/include
	APP_CPPFLAGS += -I$(ICE_ANDROID_SRC_PATH)/platform_frameworks_base/include/media/stagefright/openmax
endif
