#pragma once

#include "Boundary.h"
#include "Augmentation.h"
#include "DataManager.h"
#include "Log.h"
#include "RawManager.h"
#include "SweepQueue.h"
#include "DistVec.h"
#include <hpx/hpx.hpp>
#include <memory>

enum State{
    not_start = 0,
    active = 1,
    finalizing = 2,
    inactive = 3
};

class ArcBody {
public:
    ArcBody(DataManager* data, DistVec<std::uint64_t>* swept)
        :boundary(data), augmentation(data), queue(swept), state(State::not_start){}

    /* member */
    State state;
    Boundary boundary;
    Augmentation augmentation;
    std::vector<std::uint64_t> children = std::vector<std::uint64_t>();
    std::vector<Augmentation> inheritedAugmentations = std::vector<Augmentation>();
    SweepQueue queue;

    hpx::lcos::local::mutex remoteCallLock;
};



class Arc {
public:
    /* 
     * 现有的方案在 Arc 构造时多了条件判断，需进一步判断是否会影响程序性能
     */
    Arc(uint64_t extremum, DataManager* data, DistVec<std::uint64_t>* swept) : extremum(extremum) {
        body = std::make_unique<ArcBody>(data, swept);
    }

    
    void releaseArcBody(){
        deactivated = true;
        if(body != nullptr){
            body.reset();
        }
    }

    bool deactivated = false;
    uint64_t extremum = INVALID_VERTEX;
    uint64_t saddle = INVALID_VERTEX;
    std::unique_ptr<ArcBody> body;
};
