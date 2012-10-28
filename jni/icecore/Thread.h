// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Log.h"

namespace icecore {

struct Thread {
	struct Callback {
		virtual void main() = 0;
	};
	template<typename TClass>
	struct MethodCallback : Callback {
		MethodCallback(TClass *self, void(TClass::*func)()) : m_self(self), m_func(func) {}
		virtual void main() {
			(m_self->*m_func)();
		}
	protected:
		TClass *m_self;
		void(TClass::*m_func)();
	};
	
	Thread() : m_handle(0), m_callback(NULL), m_callback2(NULL) {
	}
	
	virtual ~Thread() {
		if(m_handle) {
			LOGE("Thread is (maybe) still running!");
		}
	}
	
	template<typename TClass, void(TClass::*func)()>
	bool start(TClass *self) {
		if(m_handle) {
			LOGE("Thread is already running!");
			return false;
		}
		if(NULL == func) {
			LOGE("Callback cannot be NULL!");
			return false;
		}
		m_callback = new MethodCallback<TClass>(self, func);
		pthread_create(&m_handle, NULL, staticMain, this);
		return true;
	}
	
#if 0
	void stop() {
		if(m_handle) {
			pthread_cancel(m_handle);
			m_handle = NULL;
			m_callback = NULL;
		}
	}
#endif
	
	bool join() {
		if(m_handle) {
			int result = pthread_join(m_handle, NULL);
			switch(result) {
			case EINVAL:
				LOGE("The implementation has detected that the value specified by thread does not refer to a joinable thread.");
				return false;
			case ESRCH:
				LOGE("No thread could be found corresponding to that specified by the given thread ID.");
				return false;
			case EDEADLK:
				LOGE("A deadlock was detected or the value of thread specifies the calling thread.");
				return false;
			}
			m_handle = 0;
			return true;
		} else {
			LOGE("Thread is not running");
		}
		return false;
	}
	
	static void exit(void *result = NULL) {
		pthread_exit(result);
	}

    static inline void nsleep(uint64_t ns) {
        _nsleep(ns);
    }

	static inline void usleep(uint64_t us) {
        _usleep(us);
    }	

	static inline void msleep(uint64_t ms) {
		_usleep(ms * 1000);
	}

#if 0
	static void yield() {
		//pthread_yield();
	}
#endif

protected:
    Thread(const Thread &);
	Thread &operator = (const Thread &);

	static void *staticMain(void *userdata) {
		Thread *self = (Thread*)userdata;
        self->m_callback->main();
		return NULL;
	}

	pthread_t m_handle;
	Callback *m_callback;
	Callback *m_callback2;
};

}

