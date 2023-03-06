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
HPX_REGISTER_ACTION(TreeConstructor_type::wrapped_type::construct_action, treeConstructor_construct_action);

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

uint64_t TreeConstructor::construct(){
    hpx::chrono::high_resolution_timer timer;
    /* search local minima */
    std::vector<uint64_t> minimaList = this->dataManager->getLocalMinima();
    numMinima = minimaList.size();
    LogInfo() << minimaList.size();
    
    // TODO: process minimum
    // for(uint64_t m: minimaList){
        // hpx::apply(this->executor_start_sweeps, TreeConstructor::startSweep_action(), this->get_id(), v, true);
    // }

    if (numMinima == 0l)
        numMinima = std::numeric_limits<std::int64_t>::max();

    LogInfo() << "termination wait finish!";
    Log().tag(std::to_string(this->index)) << "num of minima: " << numMinima;


    // TODO
    return 0;
}