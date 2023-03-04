#!/bin/bash

APP_PATH=build/simple_ct
APP_OPTIONS=data/bonsai_256x256x256_uint8.mhd

export LD_LIBRARY_PATH=$HOME/lib:$LD_LIBRARY_PATH

srun -p debug -N 1 -n 2 -c 10 $APP_PATH $APP_OPTIONS