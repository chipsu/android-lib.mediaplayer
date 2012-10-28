// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"
#include <semaphore.h>

namespace icecore {

struct Semaphore {
	
	explicit Semaphore(int value = 1) {
		sem_init(&m_handle, 0, value);
	}
	
	~Semaphore() {
		sem_destroy(&m_handle);
	}
	
	void lock() {
		int result = sem_wait(&m_handle);
		if(result != 0) {
			LOGE("sem_wait failed: %d", result);
		}
	}
	
	bool tryLock() {
		if(sem_trywait(&m_handle) == 0) {
			return true;
		}
		return false;
	}

    bool tryLock(long ns) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += ns / 1000000000;
        timeout.tv_nsec += ns % 1000000000;
        return sem_timedwait(&m_handle, &timeout) == 0;
    }

    bool tryLockMs(long ms) {
        return tryLock(ms * 1000000);
    }
	
	void unlock() {
		int result = sem_post(&m_handle);
		if(result != 0) {
			LOGE("sem_post failed: %d", result);
		}
	}

protected:	
    Semaphore(const Semaphore &);
	Semaphore &operator = (const Semaphore &);
	sem_t m_handle;
};

}
