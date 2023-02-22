#!/bin/bash

APP_PATH=build/simple_ct
APP_OPTIONS=test_input_path

export LD_LIBRARY_PATH=$HOME/lib:$LD_LIBRARY_PATH

srun -p debug -N 1 -n 2 -c 10 $APP_PATH $APP_OPTIONS