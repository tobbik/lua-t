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
int             t_buf_getHexString( lua_State *L, char *b, size_t len );
int             t_buf_getBinString( lua_State *L, char *b, size_t len );
int             t_buf_compare( char *bA, char *bB, size_t aLen, size_t bLen );
int            lt_buf_write( lua_State *L );
int            lt_buf_read ( lua_State *L );

// t_buf_seg.c
// Constructors
int               luaopen_t_buf_seg  ( lua_State *L );
struct t_buf_seg *t_buf_seg_create_ud( lua_State *L, struct t_buf *buf, int idx, int len );

