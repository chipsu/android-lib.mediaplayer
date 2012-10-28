// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include <errno.h>
#include <time.h>

#define ICE_STR(x) # x

#ifndef ICE_DEBUG
    #define ICE_DEBUG 1
#endif

#ifndef ICE_ASSERT
    #if ICE_DEBUG
	    #define ICE_ASSERT(x) do { if(!(x)) { LOGE("Assertion failed: " ICE_STR(x)); exit(0); } } while(0)
    #else
	    #define ICE_ASSERT(x) do {} while(0)
    #endif
#endif

namespace icecore {
	
inline uint64_t _global_time_ms() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

inline uint64_t _global_time_us() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

inline void _nsleep(uint64_t ns) {
	struct timespec ts;
	ts.tv_sec  = ns / 1000000000UL;
#ifdef __arm__
	ts.tv_nsec = (ns - ts.tv_sec * 1000000000UL);
#else
	ts.tv_nsec = (ns % 1000000000UL);
#endif
	for(;;) {
		if(nanosleep(&ts, &ts) >= 0 || errno != EINTR)
			break;
	}
}

inline void _usleep(uint64_t us) {
	struct timespec ts;
	ts.tv_sec  = us / 1000000UL;
#ifdef __arm__
	ts.tv_nsec = (us - ts.tv_sec * 1000000UL) * 1000;
#else
	ts.tv_nsec = (us % 1000000UL) * 1000UL;
#endif
	for(;;) {
		if(nanosleep(&ts, &ts) >= 0 || errno != EINTR)
			break;
	}
}

template<typename T>
void safe_delete(T *&x) {
	delete x;
	x = NULL;
}

}

