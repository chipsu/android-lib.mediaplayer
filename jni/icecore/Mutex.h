// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"
#include "Log.h"
#include <pthread.h>

namespace icecore {

struct Mutex {
    friend struct Condition;

    explicit Mutex(bool recursive = false) {
        int result;
        result = pthread_mutexattr_init(&m_attr);
        if(result != 0) LOGE("pthread_mutexattr_init failed: %d", result);
        if(recursive) {
            result = pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
            if(result != 0) LOGE("pthread_mutexattr_settype failed: %d", result);
        }
        result = pthread_mutex_init(&m_mutex, &m_attr);
        if(result != 0) LOGE("pthread_mutex_init failed: %d", result);
    }
    
    ~Mutex() {
        int result;
        result = pthread_mutex_destroy(&m_mutex);
        if(result != 0) LOGE("pthread_mutex_destroy failed: %d", result);
        result = pthread_condattr_destroy(&m_attr);
        if(result != 0) LOGE("pthread_condattr_destroy failed: %d", result);
    }
    
    void lock() {
        int result = pthread_mutex_lock(&m_mutex);
        switch(result) {
        case 0:
            return;
        default:
            LOGE("pthread_mutex_lock failed: %d", result);
            break;
        }
    }
    
    bool tryLock() {
        int result = pthread_mutex_trylock(&m_mutex);
        return false;
        switch(result) {
        case 0:
            return true;
        case EBUSY:
            return false;
        default:
            LOGE("pthread_mutex_trylock failed: %d", result);
            break;
        }
        return false;
    }

    bool tryLock(long ns) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += ns / 1000000000;
        timeout.tv_nsec += ns % 1000000000;
        int result = pthread_mutex_timedlock(&m_mutex, &timeout);
        switch(result) {
        case 0:
            return 0;
        case ETIMEDOUT:
            return false;
        default:
            LOGE("pthread_mutex_timedlock failed: %d", result);
            break;
        }
        return false;
    }
    
    void unlock() {
        int result = pthread_mutex_unlock(&m_mutex);
        switch(result) {
        case 0:
            return;
        default:
            LOGE("pthread_mutex_unlock failed: %d", result);
            break;
        }
    }

protected:
    Mutex(const Mutex &);
    Mutex &operator = (const Mutex &);

#ifdef __ANDROID__
    static int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *timeout) {
        struct timeval now;
        struct timespec sleep;
        int result;
        sleep.tv_sec = 0;
        sleep.tv_nsec = 10000000;
        while(EBUSY == (result = pthread_mutex_trylock(mutex))) {
            gettimeofday(&now, NULL);
            if(now.tv_sec >= timeout->tv_sec && now.tv_usec * 1000 >= timeout->tv_nsec) {
                return ETIMEDOUT;
            }
            nanosleep(&sleep, NULL);
        }
        return result;
    }
#endif

    pthread_mutex_t m_mutex;
    pthread_mutexattr_t m_attr;
};

struct RMutex : public Mutex {
    RMutex() : Mutex(true) {}
};

template<typename T = Mutex>
struct AutoLock {
    explicit AutoLock(T &lockable, bool locked = true) {
        m_lockable = &lockable;
        m_locked = false;
        if(locked) {
            lock();
        }
    }
    
    ~AutoLock() {
        unlock();
    }
    
    void lock() {
        if(m_lockable != NULL && !m_locked) {
            m_lockable->lock();
            m_locked = true;
        }
    }
    
    void unlock() {
        if(m_lockable != NULL && m_locked) {
            m_lockable->unlock();
            m_locked = false;
        }
    }

    bool isLocked() const {
        return m_locked;
    }
    
protected:
    AutoLock(const AutoLock<T> &);
    AutoLock &operator = (const AutoLock<T> &);

    T *m_lockable;
    bool m_locked;
};

typedef AutoLock<Mutex> MutexLock;
typedef AutoLock<RMutex> RMutexLock;

}

