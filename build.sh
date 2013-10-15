#!/bin/bash

if [ -d out ]; then
	rm -rf out
fi
mkdir out

cp src/* ./out/
cp xt/l_xt* ./out/


# 
cp -f xt/{Makefile,linit.c,lualib.h} ./out/
cd out
# this accumulates the Lua provided upstream patches
patch < ../patches.patch
make MYCFLAGS=' -g' CC='clang' linux

cd ..

