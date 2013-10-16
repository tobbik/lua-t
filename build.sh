#!/bin/bash

echo $0
OLDCWD=$(pwd)

cd $(dirname $0)
BASEDIR=$(pwd)

case "$1" in
	Lua)
		rm -rf out
		rm -rf compile/lua-5.2.2 && tar xzf sources/lua-5.2.2.tar.gz -C ./compile/
		cd compile/lua-5.2.2/src
		patch < ../../../patches/lua-5.2.2_upstream.patch || return 1
		cd ..
		make \
			MYCFLAGS=" -g -fPIC" \
			linux
		make \
			INSTALL_TOP="$BASEDIR/out" \
			install
		cd $BASEDIR
		;;
	Lxt)
		[ -d $BASEDIR/out ] && rm -rf $BASEDIR/out
		[ ! -d $BASEDIR/compile ] && mkdir $BASEDIR/compile
		tar xzf sources/lua-5.2.2.tar.gz -C $BASEDIR/compile/
		cd $BASEDIR/compile/lua-5.2.2/src
		cp -v $BASEDIR/xt/*.c $BASEDIR/xt/*.h $BASEDIR/xt/*.patch ./
		patch -p2 < ./build_static.patch || return 1
		patch < ../../../patches/lua-5.2.2_upstream.patch || return 1
		cd ..
		make \
			CC="gcc" \
			LD="clang" \
			MYCFLAGS=" -g -fPIC -DL_XT_ROOT=\"\\\"$BASEDIR/out/\\\"\"" \
			linux
		make \
			INSTALL_TOP="$BASEDIR/out" \
			install
		cd $BASEDIR
		;;
	xt-dyn)
		cd $BASEDIR/xt
		make clean
		make CC='clang' MYCFLAGS=' -g' INCS="$BASEDIR/compile/lua-5.2.2/src" local
		cp xt.so "$BASEDIR/out/lib/lua/5.2"
		;;
	*)
		echo "Usage: $0 {Lua|xt-dyn}"
esac

cd $OLDCWD
