/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf_l.h
 * \brief     data types and interfaces for T.Buffer and T.Buffer.Segment
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_buf.h"
#include "t.h"


// t_buf.c
// Constructors
int             luaopen_t_buf     ( lua_State *L );
struct t_buf   *t_buf_create_ud   ( lua_State *L, size_t n );
// t_buf.c helpers
int            lt_buf__eq( lua_State *L );
int            lt_buf__len( lua_State *L );
int            lt_buf_toBinString( lua_State *L );
int            lt_buf_toHexString( lua_State *L );
int            lt_buf_write( lua_State *L );
int            lt_buf_read ( lua_State *L );
int            lt_buf_clear( lua_State *L );

// t_buf_seg.c
// Constructors
int               luaopen_t_buf_seg  ( lua_State *L );

