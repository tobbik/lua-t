#!/bin/bash

if [ -d out ]; then
	rm -rf out
fi
mkdir out && cd out

cp ../src/* ./

# this accumulates the Lua provided upstream patches
patch < ../patches.patch
make linux
