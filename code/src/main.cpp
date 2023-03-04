#include <hpx/futures/future_fwd.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/program_options/options_description.hpp>
#include <hpx/runtime_distributed/find_localities.hpp>
#include <hpx/timing/high_resolution_timer.hpp>

#include "TreeConstructor.h"

int hpx_main(hpx::program_options::variables_map& vm){

    /* parse arguments */
    Options options;
    options.trunkskip = true;
    if(vm.count("no-trunkskip")){
        options.trunkskip = false;
    }

    std::string input;
    try {
        if (vm.count("hpx:positional")) {
            std::vector<std::string> positionals = vm["hpx:positional"].as<std::vector<std::string>>();

            if (positionals.size() >= 1)
                input = positionals.back();
        }
        if (input.empty())
            throw std::runtime_error("No input specified");

    } catch (const std::exception& e) {
        std::cout << "Parsing error: " << e.what() << std::endl
                  << std::endl;
        std::cout << "Usage: simple_ct [options] input" << std::endl
                  << std::endl;
        std::cout << "Check --h for details." << std::endl;
        return hpx::finalize();
    }

    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    /* initialize components */
    hpx::chrono::high_resolution_timer timer;

    std::vector<hpx::id_type> treeConstructors;
    for(hpx::id_type locality: localities){
        treeConstructors.push_back(hpx::new_<TreeConstructor>(locality).get());
    }

    /* init */
    timer.restart();
    std::vector<hpx::shared_future<void>> initFutures;
    for(hpx::id_type treeConstructor: treeConstructors){
        initFutures.push_back(hpx::async<TreeConstructor::init_action>(treeConstructor, treeConstructors, input, options));
    }
    hpx::lcos::wait_all(initFutures);
    std::cout << "Initialization: " << timer.elapsed() << " s\n"; 

    return hpx::finalize();
}

int main(int argc, char* argv[]){

    hpx::program_options::options_description descriptions("simple_ct [options] input");

    descriptions.add_options()
            ("no-trunkskip", "Perform explicit trunk computation instead of collecting dangling saddles");

    // HPX config
    std::vector<std::string> const cfg = {
        //        "hpx.stacks.small_size=0x80000"
        "hpx.stacks.use_guard_pages=0" 
    };

    // Run HPX
    hpx::init_params params;
    params.cfg = cfg;
    params.desc_cmdline = descriptions;
    return hpx::init(argc, argv, params);
}