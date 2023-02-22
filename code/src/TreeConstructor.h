#pragma once

#include <hpx/hpx.hpp>
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
    void init(const Options& options);
    // 每个能够被远程调用的成员函数都必须封装成为 component action
    HPX_DEFINE_COMPONENT_ACTION(TreeConstructor, init);
};

HPX_REGISTER_ACTION_DECLARATION(TreeConstructor::init_action, treeConstructor_init_action);
