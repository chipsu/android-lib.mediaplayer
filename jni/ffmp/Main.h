// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#define __STDINT_MACROS
	
#include <icecore/Log.h>
#include <icecore/Mutex.h>
#include <icecore/Semaphore.h>
#include <icecore/Condition.h>
#include <icecore/Thread.h>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

#include <vector>
#include <list>

namespace ffmp {
	
typedef uint64_t microsec_t;

struct Config {
	int maxFrameDelay;
	int maxPacketQueueSize;
	int maxRenderQueueSize;
	bool asyncDecode[AVMEDIA_TYPE_NB];
	bool asyncRender[AVMEDIA_TYPE_NB];
	int lastOpenStream[AVMEDIA_TYPE_NB];
	Config() {
		maxFrameDelay = 100000;
		maxPacketQueueSize = 10;
		maxRenderQueueSize = 10;
		for(int i = 0; i < AVMEDIA_TYPE_NB; ++i) lastOpenStream[i] = -1;
	}
};

inline microsec_t ffmp_time() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000000 + now.tv_nsec / 1000;
	//return icecore::_global_time_us();
	//return av_gettime();
}

inline void ffmp_sleep(microsec_t time) {
	//av_usleep(time);
	icecore::_usleep(time);
}

}
