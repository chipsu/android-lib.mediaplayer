// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

#include "Main.h"

namespace ffmp {

template<typename TItem, typename TManager>
struct Queue {
    typedef std::list<TItem> TList;
    
    Queue() {
        m_abort = false;
        m_maxSize = 0;
    }
    
    ~Queue() {
        flush();
    }
    
    size_t put(TItem &packet) {
        size_t size;
        m_lock.lock();
        while(m_maxSize && m_list.size() > m_maxSize) {
            if(m_abort) {
                TManager::free(packet);
                return 0;
            }
            m_lock.unlock();
            ffmp_sleep(10000); // TODO: cond?
            m_lock.lock();
        }
        m_list.push_back(packet);
        size = m_list.size();
        m_cond.unlock();
        m_lock.unlock();
        return size;
    }
    
    bool get(TItem &result, bool block) {
        bool ret = false;
        m_lock.lock();
        while(1) {
            if(m_abort) {
                break;
            }
            if(!m_list.empty()) {
                result = m_list.front();
                m_list.pop_front();
                ret = true;
                break;
            }
            if(!block) {
                break;
            }
            m_cond.lock(m_lock);
        };
        m_lock.unlock();
        return ret;
    }
    
    void flush() {
        m_lock.lock();
        for(typename TList::iterator it = m_list.begin(); it != m_list.end(); ++it) {
            TManager::free(*it);
        }
        m_list.clear();
        m_lock.unlock();
    }
    
    void abort() {
        m_lock.lock();
        m_abort = true;
        m_cond.unlock();
        m_lock.unlock();
    }
    
    void resume() {
        m_lock.lock();
        m_abort = false;
        m_lock.unlock();
    }
    
    bool aborted() const {
        bool result;
        m_lock.lock();
        result = m_abort;
        m_lock.unlock();
        return result;
    }
    
    size_t size() const {
        size_t result;
        m_lock.lock();
        result = m_list.size();
        m_lock.unlock();
        return result;
    }
    
    void setMaxSize(size_t size) {
        m_maxSize = size;
    }
    
    mutable icecore::Mutex m_lock;
    mutable icecore::Condition m_cond;
    TList m_list;
    bool m_abort;
    size_t m_maxSize;
};
    
}
