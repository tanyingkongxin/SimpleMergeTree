#pragma once

#include "Arc.h"
#include "DataManager.h"
#include <hpx/serialization/access.hpp>

class Options{
public:
    bool trunkskip;

private:
    // Serialization support: provide an (empty) implementation for the
    // serialization as all arguments passed to actions have to support this.
    friend class hpx::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version){
        ar & trunkskip; 
    }

};

class TreeConstructor : public hpx::components::component_base<TreeConstructor> {
public:
    TreeConstructor():dataManager(nullptr), numMinima(0){}

    TreeConstructor(const TreeConstructor& ) = delete;
    TreeConstructor& operator=(const TreeConstructor& ) = delete;

    // ~TreeConstructor(){}

    void init(const std::vector<hpx::id_type>& treeConstructors, const std::string& input, const Options& options);
    // 每个能够被远程调用的成员函数都必须封装成为 component action
    HPX_DEFINE_COMPONENT_ACTION(TreeConstructor, init);

    uint64_t construct();
    HPX_DEFINE_COMPONENT_ACTION(TreeConstructor, construct);

    void startSweep(uint64_t v, bool leaf);
    HPX_DEFINE_COMPONENT_ACTION(TreeConstructor, startSweep);

    void continueLocalSweep(uint64_t v);
    HPX_DEFINE_COMPONENT_ACTION(TreeConstructor, continueLocalSweep);

    bool fetchCreateArc(Arc*& arc, uint64_t v);
    void mergeBoundaries(Arc*& arc);

    bool touch(uint64_t c, uint64_t v);
    bool searchUF(uint64_t start, uint64_t goal);

private:
    uint32_t index;
    Options options;
    std::vector<hpx::id_type> treeConstructors;

    DataManager* dataManager;
    int64_t numMinima;
    // the number of vertices (with ghost) in this locality
    uint64_t numVertices; 

   

    // the lock of arcMap
    hpx::lcos::local::mutex mapLock;

    //
    std::atomic<int64_t> sweeps;
    // Map for each vertex to ID of arc extremum
    DistVec<uint64_t> swept; // what vertex has been swept by which saddle/local minimum
    // Map for starting minima(or saddle) to Arc pointer
    DistVec<Arc*> arcMap;

    // Union-find-structure containing child-parent relations
    DistVec<uint64_t> UF;

};

HPX_REGISTER_ACTION_DECLARATION(TreeConstructor::init_action, treeConstructor_init_action);
HPX_REGISTER_ACTION_DECLARATION(TreeConstructor::construct_action, treeConstructor_construct_action);
HPX_REGISTER_ACTION_DECLARATION(TreeConstructor::startSweep_action, treeConstructor_startSweep_action);
HPX_REGISTER_ACTION_DECLARATION(TreeConstructor::continueLocalSweep_action, treeConstructor_continueLocalSweep_action);