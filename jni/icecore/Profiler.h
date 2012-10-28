// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"
#include "Mutex.h"
#include <vector>
#include <algorithm>

#if ICE_DEBUG
	#define ICE_PROFILE(name) static ::icecore::Profiler __profiler_##__LINE__(__FILE__,__PRETTY_FUNCTION__,__LINE__,name); \
									  ::icecore::AutoProfiler __autoProfiler_##__LINE__(&__profiler_##__LINE__)
#else
	#define ICE_PROFILE(name) do {} while(0)
#endif

namespace icecore {

struct Profiler {
	
	static void _add(Profiler *profiler) {
		s_globalLock.lock();
		s_profilers.push_back(profiler);
		s_globalLock.unlock();
	}
	
	static void _remove(Profiler *profiler) {
		s_globalLock.lock();
		for(std::vector<Profiler*>::iterator it = s_profilers.begin(); it != s_profilers.end(); ++it) {
			if((*it) == profiler) {
				s_profilers.erase(it);
				break;
			}
		}
		s_globalLock.unlock();
	}
	
	static void reportAll() {
		s_globalLock.lock();
		__android_log_print(ANDROID_LOG_INFO, ICE_LOG_TAG, "PROFILER LOG START - Times in microseconds");
		__android_log_print(ANDROID_LOG_INFO, ICE_LOG_TAG, "PROFILER:      CALLS,      TOTAL,    AVERAGE,        MIN,        MAX, FUNC:LINE (NAME)");
		std::sort(s_profilers.begin(), s_profilers.end());
		for(std::vector<Profiler*>::iterator it = s_profilers.begin(); it != s_profilers.end(); ++it) {
			(*it)->report();
		}
		__android_log_print(ANDROID_LOG_INFO, ICE_LOG_TAG, "PROFILER LOG END");
		s_globalLock.unlock();
	}
	
	Profiler(const char *file, const char *func, int line, const char *name) {
		m_file = file;
		m_func = func;
		m_line = line;
		m_name = name;
		m_calls = 0;
		m_time = 0;
        m_min = -1;
        m_max = 0;
		_add(this);
	}
	
	~Profiler() {
		_remove(this);
	}
	
	void update(long delta) {
		m_time += delta;
        if(m_min < 0 || delta < m_min) m_min = delta;
        if(delta > m_max) m_max = delta;
		m_calls++;
	}
	
	void report() {
		long avg = m_time / m_calls;
		__android_log_print(ANDROID_LOG_INFO, ICE_LOG_TAG, "PROFILER: %10d, %10d, %10d, %10d, %10d, %s:%04d (%s)", m_calls, m_time, avg, m_min, m_max, m_func, m_line, m_name);
	}
	
	bool operator < (const Profiler &other) {
		return m_time < other.m_time;
	}
	
	const char *m_file;
	const char *m_func;
	int m_line;
	const char *m_name;
	
	size_t m_calls;
	long m_time;
    long m_min;
    long m_max;
	
	static Mutex s_globalLock;
	static std::vector<Profiler*> s_profilers;
};

struct AutoProfiler {

	explicit AutoProfiler(Profiler *profiler) {
		m_profiler = profiler;
		m_time = _global_time_us();
	}
	
	~AutoProfiler() {
		if(m_profiler != NULL) {
			long delta = _global_time_us() - m_time;
			m_profiler->update(delta);
		}
	}
	
	Profiler *m_profiler;
	long m_time;
};

}
