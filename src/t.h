/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t.h
 * \brief     general functions and globally known values
 *            data definitions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define UNUSED(...)   (void)(__VA_ARGS__)


#ifndef T_HELPERLIB_H
#define T_HELPERLIB_H

#define t_getAbsPos( L, pos)     \
  ((pos) < 0)                    \
   ? (lua_gettop( L ) + pos + 1) \
   : (pos)

// global helpers
int         t_getLoadedValue( lua_State *L, size_t len, int pos, ... );
int         t_push_error    ( lua_State *L, const char *fmt, ... );
int         t_typeerror     ( lua_State *L, int arg, const char *tname );
#endif //T_HELPERLIB_H


/******************************************************************************
 * Copyright (C) 2012-2020, Tobias Kieslich (tkieslich). All rights reseved.
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
