#!/bin/bash

export LUA_PATH="${PWD}/out/share/lua/5.3/?.lua;;"
export LUA_CPATH="${PWD}/out/lib/lua/5.3/?.so;;"

export MYCFLAGS=" -m32 -O2 -mthumb -march=armv8-a -mcpu=cortex-a72 -mtune=cortex-a72.cortex-a53 -mfloat-abi=softfp"
