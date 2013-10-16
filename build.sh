#!/bin/bash

echo $0
OLDCWD=$(pwd)

cd $(dirname $0)
BASEDIR=$(pwd)

case "$1" in
	Lua-dyn)
		rm -rf out
		rm -rf build/lua-5.2.2 && tar xzf sources/lua-5.2.2.tar.gz -C ./build/
		cd build/lua-5.2.2/src
		patch < ../../../patches/lua-5.2.2_upstream.patch || return 1
		cd ..
		patch -p1 < ../../patches/lua-5.2.2_so.patch || return 1
		make \
			MYCFLAGS=" -fPIC -DL_XT_ROOT=\"\\\"$BASEDIR/out/\\\"\"" \
			TO_LIB="liblua.a liblua.so liblua.so.5.2 liblua.so.5.2.1" \
			linux
		make \
			TO_LIB="liblua.a liblua.so liblua.so.5.2 liblua.so.5.2.1" \
			INSTALL_TOP="$BASEDIR/out" \
			install
		cd $BASEDIR
		;;
	Lua-sta)
		rm -rf out
		rm -rf build/lua-5.2.2 && tar xzf sources/lua-5.2.2.tar.gz -C ./build/
		cd build/lua-5.2.2/src
		patch < ../../../patches/lua-5.2.2_upstream.patch || return 1
		cd ..
		make \
			MYCFLAGS=" -fPIC -DL_XT_ROOT=\"\\\"$BASEDIR/out/\\\"\"" \
			TO_LIB="liblua.a liblua.so liblua.so.5.2 liblua.so.5.2.1" \
			linux
		make install \
			TO_LIB="liblua.a liblua.so liblua.so.5.2 liblua.so.5.2.1" \
			TOP="$BASEDIR/out"
		cd $BASEDIR
		;;
	xt)
		cd $BASEDIR/xt
		make clean
		make CC='clang' MYCFLAGS=' -g' INCS="$BASEDIR/out/include" local
		cp xt.so "$BASEDIR/out/lib/lua/5.2"
		;;
	*)
		echo "Usage: $0 {Lua-dyn|Lua-sta}"
esac

cd $OLDCWD

