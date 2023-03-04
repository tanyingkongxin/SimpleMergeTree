#pragma once

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
    TreeConstructor(){}

    TreeConstructor(const TreeConstructor& ) = delete;
    TreeConstructor& operator=(const TreeConstructor& ) = delete;

    // ~TreeConstructor(){}

    void init(const std::vector<hpx::id_type>& treeConstructors, const std::string& input, const Options& options);
    // 每个能够被远程调用的成员函数都必须封装成为 component action
    HPX_DEFINE_COMPONENT_ACTION(TreeConstructor, init);

private:
    uint32_t index;
    Options options;
    std::vector<hpx::id_type> treeConstructors;

    DataManager* dataManager;
};

HPX_REGISTER_ACTION_DECLARATION(TreeConstructor::init_action, treeConstructor_init_action);
