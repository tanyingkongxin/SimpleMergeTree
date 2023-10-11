#include "TreeConstructor.h"
#include <boost/algorithm/string.hpp>
#include <cstdint>

#include "DataManager.h"
#include "Log.h"
#include "RawManager.h"
#include "Value.h"

HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<TreeConstructor> TreeConstructor_type;
HPX_REGISTER_COMPONENT(TreeConstructor_type, TreeConstructor);

// typedef TreeConstructor::init_action init_action;
// HPX_REGISTER_ACTION(init_action);

HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::init_action, treeConstructor_init_action);
HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::construct_action, treeConstructor_construct_action);
HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::startSweep_action, treeConstructor_startSweep_action);
HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::continueLocalSweep_action, treeConstructor_continueLocalSweep_action);

void TreeConstructor::init(const std::vector<hpx::id_type>& treeConstructors, const std::string& input, const Options& options){

    this->options = options;

    // Store list of tree constructor components and determine index of this component
    this->treeConstructors = treeConstructors;
    for (uint32_t i = 0; i < this->treeConstructors.size(); ++i) {
        if (this->treeConstructors[i] == this->get_id()) {
            this->index = i;
            break;
        }
    }

    /* load data */
    if(boost::algorithm::ends_with(input, ".mhd")){
        if(boost::algorithm::ends_with(input, "uint8.mhd")){
            this->dataManager = new RawManager<char>(input);
        }
        // TODO: 补充其他类型
    }

    if(this->dataManager){
        this->dataManager->init(this->index, this->treeConstructors.size());
    }
    else{
        LogError() << "Error: unknow file format\n";
        return ;
    }

    /* init data structure */
    this->numMinima = 0;
    this->numVertices = this->dataManager->getNumVerticesLocal(true);
    this->arcMap.init(this->numVertices, nullptr, this->dataManager);
    this->sweeps.store(0);
    this->swept.init(this->numVertices, INVALID_VERTEX, this->dataManager);
    this->UF.init(this->numVertices, INVALID_VERTEX, this->dataManager);
}

/*
 * @return 
 */
uint64_t TreeConstructor::construct(){
    hpx::chrono::high_resolution_timer timer;
    /* search local minima */
    std::vector<uint64_t> minimaList = this->dataManager->getLocalMinima();
    this->numMinima = minimaList.size();
    LogInfo() << minimaList.size();
    
    // TODO: process minimum
    for(uint64_t m: minimaList){
        // hpx::apply(this->executor_start_sweeps, TreeConstructor::startSweep_action(), this->get_id(), v, true);
    }

    if (this->numMinima == 0l)
        this->numMinima = std::numeric_limits<std::int64_t>::max();

    LogInfo() << "termination wait finish!";
    Log().tag(std::to_string(this->index)) << "num of minima: " << this->numMinima;


    // TODO
    return 0;
}

void TreeConstructor::startSweep(uint64_t v, bool leaf){
    // Fetch Arc
    Arc* arc;
    this->mapLock.lock(); // 加锁是因为 fetchCreateArc 可能会修改 DistVec 的内容
    fetchCreateArc(arc, v);
    this->mapLock.unlock();

    this->swept[v] = v;
    
    mergeBoundaries(arc); // 处理了 queue 和 boundary
    // 接着处理 augmentation
    arc->body->augmentation.inherit(arc->body->inheritedAugmentations); // gathered and merged here because no lock required here
    arc->body->augmentation.sweep(v); // also add saddle/local minimum to augmentation

    // TODO: if done occured

    arc->body->state = State::active;

    for (int i = 0; (i < 6); i++){
        // 如果邻居在其他 locality 上，且 no be swept
        if (this->dataManager->isGhost(this->dataManager->getNeighbor(v, i)) && (this->swept[this->dataManager->getNeighbor(v, i)] == INVALID_VERTEX)){ 
            // TODO: 此处是否需要加锁?
            arc->body->remoteCallLock.lock();
    //Now we can authorize remote sweeps:
    //Initial Fill of Queue
            // 那么就向其他 locality 请求执行 continueSweep，结果保存在 remoteCalls 中
            // arc->body->remoteCalls.push_back(hpx::async<TreeConstructor::continueSweep_action>(this->treeConstructors[this->dataManager->getBlockIndex(this->dataManager->getNeighbor(v, i))], v, this->dataManager->convertToGlobal(this->dataManager->getNeighbor(v,i)), this->dataManager->convertToGlobal(v), false));
            arc->body->remoteCallLock.unlock();
        } 
        // 否则直接将邻居放入 queue 中
        else {
            arc->body->queue.push(this->dataManager->getNeighbor(v, i));
        }
    }

    // TODO: peers 在此处起何作用？
    // arc->body->remoteCallLock.lock();
    // for (hpx::id_type peer : arc->body->peers){
    //     if (peer != this->get_id())
    //         arc->body->remoteCalls.push_back(hpx::async<TreeConstructor::continueSweepPeer_action>(peer, v, arc->body->children));
    // }
    // arc->body->remoteCallLock.unlock();

    // 正式开始处理本地的 sweep
    continueLocalSweep(v);
}

void TreeConstructor::continueLocalSweep(uint64_t v){
    uint64_t min;
    Arc* arc;
    this->mapLock.lock();
    fetchCreateArc(arc, v);
    // arc->body->leaf = true;
    this->mapLock.unlock();

    /* sweep loop */
    while(!arc->body->queue.empty()){
        uint64_t c = arc->body->queue.pop();
        if(c == INVALID_VERTEX)break;

        if(this->dataManager->isGhost(c)){
            arc->body->boundary.remove(c);
            swept[c] = v;
            
            uint64_t neighbors[6];
            uint32_t numNeighbors = this->dataManager->getNeighbors(c, neighbors);

            for (uint64_t i = 0; (i < numNeighbors); i++){
                if ((!this->dataManager->isGhost(neighbors[i])) && (this->swept[neighbors[i]] == INVALID_VERTEX)){
                    arc->body->queue.push(neighbors[i]);
                }
            }
            continue;
        }

        // if can be swept
        if(this->touch(c, v)){
            arc->body->boundary.remove(c); // remove from boundary
            this->swept[c] = v;     // put into our augmentation and mark as swept
            arc->body->augmentation.sweep(c);

            uint64_t neighbors[6];
            uint32_t numNeighbors = this->dataManager->getNeighbors(c, neighbors);

            // TODO: 补充 remote call 的部分
            for (uint64_t i = 0; (i < numNeighbors); i++){
                if (this->dataManager->isGhost(neighbors[i]) && (this->swept[neighbors[i]] == INVALID_VERTEX)){
                    // arc->body->remoteCallLock.lock();
                    // arc->body->remoteCalls.push_back(hpx::async<TreeConstructor::continueSweep_action>(this->treeConstructors[this->dataManager->getBlockIndex(neighbors[i])], v, this->dataManager->convertToGlobal(neighbors[i]), this->dataManager->convertToGlobal(c), false));
                    // arc->body->remoteCallLock.unlock();
                } else {
                    arc->body->queue.push(neighbors[i]);
                }
            }
        }
        // else can not be swept now
        else{
            arc->body->boundary.add(c);
        }
    } /* end sweep loop */

    // arc->body->lock.lock();
    // 正常情况下扫描结束后 queue 为空
    if (arc->body->queue.empty()){
        arc->body->state = State::finalizing;
        // 找到其中的最小值即为 saddle 
        if (arc->body->boundary.empty()){ // 特殊情况? 还是最后的根节点？
            min = INVALID_VERTEX;
        } else {
            min = arc->body->boundary.min();
        }
    }
    // Q: 此处暂时看不懂
    // else {
    //     if (done.occurred()){
    //         arc->body->lock.unlock();
    //         awaitRemoteAnswers(arc);
    //         hpx::apply(this->executor_high, TreeConstructor::startTrunk_action(), this->treeConstructors[0], this->dataManager->getValue(arc->extremum));
    //         hpx::apply(this->executor_high, TreeConstructor::countSweeps_action(), this->treeConstructors[0], -1l);
    //         return;
    //     } else {
    //         arc->body->lock.unlock();
    //         hpx::apply(TreeConstructor::continueLocalSweep_action(), this->get_id(), v);
    //         return;
    //     }
    // }
}

bool TreeConstructor::fetchCreateArc(Arc*& arc, uint64_t v){
    Arc*& tmparc = this->arcMap[v];
    if (tmparc == nullptr){
        tmparc = new Arc(v, this->dataManager, &swept);
        arc = tmparc;
        return true;
    } else {
        arc = tmparc;
        return false;
    }
}

/* 在 RegionGrowth 之前初始化 Arc 的 queue 和 boundary */
void TreeConstructor::mergeBoundaries(Arc*& arc){
    for (uint32_t i = 0; i < arc->body->children.size(); ++i){
        mapLock.lock();
        Arc* childptr = arcMap[arc->body->children[i]];
        mapLock.unlock();
        if (childptr == nullptr)
            continue;
        for (uint32_t j = i+1; j < arc->body->children.size(); ++j){
            mapLock.lock();
            Arc* child2ptr = arcMap[arc->body->children[j]];
            mapLock.unlock();
            if (child2ptr == nullptr)
                continue;
            arc->body->queue.push(childptr->body->boundary.intersect(child2ptr->body->boundary));
        }

        arc->body->boundary.unite(childptr->body->boundary);
    }
}

/*
 * Sweep reaches vertex and checks if it can be swept by going through *all* its smaller neighbors and check if they *all* have already been swept by us
 */
bool TreeConstructor::touch(uint64_t c, uint64_t v){

    uint64_t neighbors[6];
    uint32_t numNeighbors = this->dataManager->getNeighbors(c, neighbors);

    for (uint64_t& n: neighbors) {
        if (n != INVALID_VERTEX) {
            if(this->dataManager->less(n, v)){
                if(!this->searchUF(this->swept[n], v)){
                    return false;
                }
            }
        }
    }
    return true;
}

bool TreeConstructor::searchUF(uint64_t start, uint64_t goal){
    // goal is actively running sweep

    if (start == INVALID_VERTEX)
        return false;

    uint64_t c = start;
    uint64_t next = this->UF[c];
    if (c == goal || next == goal)
        return true;
    if (next == INVALID_VERTEX)
        return false;

    // includes path compression
    while (true) {
        if (this->UF[next] == goal) {
            this->UF[c] = this->UF[next];
            return true;
        }

        if (this->UF[next] == INVALID_VERTEX)
            return false;

        this->UF[c] = this->UF[next];
        next = this->UF[next];
    }
}