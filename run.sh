#!/usr/bin/env bash

# export HPX_VERSION="1.6.0"
# mkdir hpx-source
# curl -L https://github.com/STEllAR-GROUP/hpx/archive/refs/tags/${HPX_VERSION}.tar.gz | tar zx -C hpx-source --strip-components 1
# mkdir hpx-build
# pushd hpx-build
# cmake -G Ninja \
#       -D CMAKE_INSTALL_PREFIX=$HOME \
#       -D CMAKE_BUILD_TYPE=Release \
#       -D HPX_WITH_MALLOC=tcmalloc \
#       -D HPX_WITH_EXAMPLES=OFF \
#       -D HPX_WITH_TESTS=OFF \
#       -D HPX_WITH_PAPI=ON \
#       ../hpx-source

# cmake --build . --target install
# popd
# rm -rf hpx-build
# rm -rf hpx-source 

export LD_LIBRARY_PATH=$HOME/lib:$LD_LIBRARY_PATH

rm -r build
mkdir build
pushd build

cmake -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      ../code/src

cmake --build .

# rm -rf ../results/*

./simple_ct trunkskip ../data/bonsai_256x256x256_uint8.mhd

popd