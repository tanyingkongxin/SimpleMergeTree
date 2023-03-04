#include "TreeConstructor.h"
#include <boost/algorithm/string.hpp>

#include "Log.h"
#include "RawManager.h"

HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<TreeConstructor> TreeConstructor_type;
HPX_REGISTER_COMPONENT(TreeConstructor_type, TreeConstructor);

// typedef TreeConstructor::init_action init_action;
// HPX_REGISTER_ACTION(init_action);

HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::init_action, treeConstructor_init_action);


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
    }
}