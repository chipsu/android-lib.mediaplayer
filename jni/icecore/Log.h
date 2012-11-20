// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"
#include <stdio.h>
#include <string.h>

#ifdef __ANDROID__
    #include <android/log.h>
#endif

#define ICE_LOG_DEBUG 1
#define ICE_LOG_INFO 2
#define ICE_LOG_WARN 3
#define ICE_LOG_ERROR 4

#ifndef ICE_LOG_TAG
    #define ICE_LOG_TAG "icecore"
#endif

#ifndef ICE_LOG_BUFFER_SIZE
    #define ICE_LOG_BUFFER_SIZE 2048
#endif

#ifndef ICE_LOG
    #if ICE_DEBUG
        #define ICE_LOG ICE_LOG_DEBUG
    #else
        #define ICE_LOG ICE_LOG_WARN
    #endif
#endif

#if ICE_LOG
    #define LOGD(...) ::icecore::LogPrint(ICE_LOG_DEBUG,ICE_LOG_TAG,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
    #define LOGI(...) ::icecore::LogPrint(ICE_LOG_INFO,ICE_LOG_TAG,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
    #define LOGW(...) ::icecore::LogPrint(ICE_LOG_WARN,ICE_LOG_TAG,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
    #define LOGE(...) ::icecore::LogPrint(ICE_LOG_ERROR,ICE_LOG_TAG,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)

    #define LOGD_ONCE(...) do { static int __logged##__LINE__ = 0; if(!__logged##__LINE__) { LOGD(__VA_ARGS__); __logged##__LINE__ = 1; } } while(0)
    #define LOGI_ONCE(...) do { static int __logged##__LINE__ = 0; if(!__logged##__LINE__) { LOGI(__VA_ARGS__); __logged##__LINE__ = 1; } } while(0)
    #define LOGW_ONCE(...) do { static int __logged##__LINE__ = 0; if(!__logged##__LINE__) { LOGW(__VA_ARGS__); __logged##__LINE__ = 1; } } while(0)
    #define LOGE_ONCE(...) do { static int __logged##__LINE__ = 0; if(!__logged##__LINE__) { LOGE(__VA_ARGS__); __logged##__LINE__ = 1; } } while(0)

    #define LOG_TIMED(l,t,...) \
        do { \
            static uint64_t __logged##__LINE__ = 0; \
            static uint64_t __suppressed##__LINE__ = 0; \
            if(__logged##__LINE__ < ::icecore::_global_time_ms()) { \
                ::icecore::LogPrint(l,ICE_LOG_TAG,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__); \
                if(__suppressed##__LINE__) ::icecore::LogPrint(l,ICE_LOG_TAG,__FILE__,__FUNCTION__,__LINE__,"(Previous message was suppressed %d times)", __suppressed##__LINE__); \
                __logged##__LINE__ = ::icecore::_global_time_ms() + 1000*t; \
                __suppressed##__LINE__ = 0; \
            } else {\
                ++__suppressed##__LINE__; \
            } \
        } while(0)

    #define LOGD_TIMED(t,...) LOG_TIMED(ICE_LOG_DEBUG,t,__VA_ARGS__)
    #define LOGI_TIMED(t,...) LOG_TIMED(ICE_LOG_INFO,t,__VA_ARGS__)
    #define LOGW_TIMED(t,...) LOG_TIMED(ICE_LOG_WARN,t,__VA_ARGS__)
    #define LOGE_TIMED(t,...) LOG_TIMED(ICE_LOG_ERROR,t,__VA_ARGS__)

#else
    #define LOGD(...)
    #define LOGI(...)
    #define LOGW(...)
    #define LOGE(...)
    #define LOGD_ONCE(...)
    #define LOGI_ONCE(...)
    #define LOGW_ONCE(...)
    #define LOGE_ONCE(...)
    #define LOGD_TIMED(...)
    #define LOGI_TIMED(...)
    #define LOGW_TIMED(...)
    #define LOGE_TIMED(...)
#endif

namespace icecore {

inline void LogPrintArgs(int level, const char *tag, const char *file, const char *func, int line, const char *format, va_list args) {
    if(level >= ICE_LOG) {
        char buffer[ICE_LOG_BUFFER_SIZE];
#ifdef __ANDROID__
        char final[ICE_LOG_BUFFER_SIZE];
#endif
        file = strrchr(file, '/');
        vsnprintf(buffer, ICE_LOG_BUFFER_SIZE, format, args);
#ifdef __ANDROID__
        int lvl = ANDROID_LOG_INFO;
        switch(level) {
        case ICE_LOG_DEBUG: lvl = ANDROID_LOG_DEBUG; break;
        case ICE_LOG_INFO: lvl = ANDROID_LOG_INFO; break;
        case ICE_LOG_WARN: lvl = ANDROID_LOG_WARN; break;
        case ICE_LOG_ERROR: lvl = ANDROID_LOG_ERROR; break;
        }
        snprintf(final, ICE_LOG_BUFFER_SIZE, "[%s:%d->%s] %s\n", file ? file + 1 : NULL, line, func, buffer);
        __android_log_print(lvl, tag, final);
#else
        printf("[%s:%d->%s] %s\n", file ? file + 1 : NULL, line, func, buffer);
#endif
    }
}

inline void LogPrint(int level, const char *tag, const char *file, const char *func, int line, const char *format, ...) {
    if(level >= ICE_LOG) {
        va_list args;
        va_start(args, format);
        LogPrintArgs(level, tag, file, func, line, format, args);
        va_end(args);
    }
}

}
