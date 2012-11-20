// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Mutex.h"

namespace icecore {

struct Condition {

    Condition() {
        int result;
        result = pthread_condattr_init(&m_attr);
        if(result != 0) LOGE("pthread_contattr_init failed: %d", result);
        result = pthread_cond_init(&m_cond, &m_attr);
        if(result != 0) LOGE("pthread_cond_init failed: %d", result);
    }

    ~Condition() {
        int result;
        result = pthread_cond_destroy(&m_cond);
        if(result != 0) LOGE("pthread_cond_destroy failed: %d", result);
        result = pthread_condattr_destroy(&m_attr);
        if(result != 0) LOGE("pthread_contattr_destroy failed: %d", result);
    }

    void lock(Mutex &mutex) {
        int result = pthread_cond_wait(&m_cond, &(mutex.m_mutex));
        switch(result) {
        case 0:
            return;
        default:
            LOGE("pthread_cond_wait failed: %d", result);
            break;
        }
    }

    bool tryLock(Mutex &mutex, long ns) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += ns / 1000000000;
        timeout.tv_nsec += ns % 1000000000;
        int result = pthread_cond_timedwait(&m_cond, &(mutex.m_mutex), &timeout);
        switch(result) {
        case 0:
            return true;
        case ETIMEDOUT:
            return false;
        default:
            LOGE("pthread_cond_timedwait failed: %d", result);
            break;
        }
        return false;
    }

    void unlock() {
        int result = pthread_cond_signal(&m_cond);
        switch(result) {
        case 0:
            return;
        default:
            LOGE("pthread_cond_signal failed: %d", result);
            break;
        }
    }

    void unlockAll() {
        int result = pthread_cond_broadcast(&m_cond);
        switch(result) {
        case 0:
            return;
        default:
            LOGE("pthread_cond_broadcast failed: %d", result);
            break;
        }
    }

protected:
    Condition(const Condition &);
    Condition &operator = (const Condition &);

    pthread_cond_t m_cond;
    pthread_condattr_t m_attr;
};

}

