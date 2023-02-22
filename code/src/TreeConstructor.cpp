#include "TreeConstructor.h"

HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<TreeConstructor> TreeConstructor_type;
HPX_REGISTER_COMPONENT(TreeConstructor_type, TreeConstructor);

// typedef TreeConstructor::init_action init_action;
// HPX_REGISTER_ACTION(init_action);

HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::init_action, treeConstructor_init_action);

void TreeConstructor::init(const Options& options){
    // output all options
    std::cout << "trunkskip: " << (options.trunkskip ? "true" : "false") << "\n";

}