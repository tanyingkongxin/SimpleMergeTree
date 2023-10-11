#pragma once

#include "DistVec.h"
#include "DataManager.h"
#include <deque>

#include <hpx/hpx.hpp>

/* 可能尝试其他数据结构 #include <boost/heap/fibonacci_heap.hpp> */
class SweepQueue{
public:
    SweepQueue(DistVec<uint64_t>* swept):swept(swept), queue(6){

    }

    // 是否需要加锁保护呢
    void push(const uint64_t & target){
        std::lock_guard<hpx::lcos::local::mutex> lock_(lock);
        this->queue.push_front(target);
    }

    void push(const std::vector<uint64_t>& boundaryVertices){
        std::lock_guard<hpx::lcos::local::mutex> tmplock(lock);
        for (const uint64_t& c : boundaryVertices) {
            this->queue.push_front(c);
        }
    }

    // TODO: 单纯查询操作是否需要加锁保护
    bool empty() const {
        // std::lock_guard<hpx::lcos::local::mutex> lock_(lock);
        return this->queue.empty();
    }

    // TODO
    uint64_t pop(){
        std::lock_guard<hpx::lcos::local::mutex> tmplock(lock);
        while (!this->queue.empty()) {
            uint64_t result = this->queue.front();
            this->queue.pop_front();
            if (swept->operator [](result) == INVALID_VERTEX)
                return result;
        }
        return INVALID_VERTEX;
    }
private:
    mutable hpx::lcos::local::mutex lock;
    DistVec<uint64_t>* swept;
    std::deque<uint64_t> queue;
};