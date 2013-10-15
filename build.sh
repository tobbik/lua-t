#!/bin/bash

if [ -d out ]; then
	rm -rf out
fi
mkdir out

cp src/* ./out/
cp library/library* library/l_net* library/l_buffer* ./out/


# 
cp -f library/{Makefile,linit.c,lualib.h} ./out/
cd out
# this accumulates the Lua provided upstream patches
patch < ../patches.patch
make MYCFLAGS=' -g' CC='clang' linux

cd ..

