/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t.h
 * \brief     general functions and globally known values
 *            data definitions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


/* includes the Lua headers */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "t_dbg.h"

#define MAX_PKT_BYTES     1500

#define PRINT_DEBUGS      0

#define UNUSED(...)   (void)(__VA_ARGS__)

// macro taken from Lua 5.3 source code
// number of bits in a character
#define NB                 CHAR_BIT


// http://stackoverflow.com/questions/2100331/c-macro-definition-to-determine-big-endian-or-little-endian-machine
// http://esr.ibiblio.org/?p=5095
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)
#define IS_LITTLE_ENDIAN (1 == *(unsigned char *)&(const int){1})
//#define IS_LITTLE_ENDIAN (*(uint16_t*)"\0\1">>8)
//#define IS_BIG_ENDIAN (*(uint16_t*)"\1\0">>8)

#define TSWAP(type,a,b) do{type SWAP_tmp=b; b=a; a=SWAP_tmp;}while(0)


// --------------------------- DATA TYPES --------------------------------------
// name:integer pairs for grouped constants such as AF_INET, SOCK_STREAM, etc.
struct t_typ {
	const char *name;
	const int   value;
};


// global helpers
void t_getTypeByName ( lua_State *L, int pos, const char *dft, const struct t_typ *types );
void t_getTypeByValue( lua_State *L, int pos, const int   dft, const struct t_typ *types );
int  t_push_error    ( lua_State *L, const char *fmt, ... );


/******************************************************************************
 * Copyright (C) 2012-2017, Tobias Kieslich (tkieslich). All rights reseved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *****************************************************************************/
