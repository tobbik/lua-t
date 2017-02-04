/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf.h
 * \brief     data types for buffer
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/// The userdata struct for T.Buffer
struct t_buf {
	size_t   len;   ///<  length of the entire buffer in bytes
	char     b[1];  ///<  pointer to the variable size buffer -> must be last in struct
};


// t_buf.c
// Constructors
int           luaopen_t_buf   ( lua_State *L );
struct t_buf *t_buf_check_ud  ( lua_State *L, int pos, int check );
struct t_buf *t_buf_create_ud ( lua_State *L, int size );

// helpers to check and verify input on stack
struct t_buf * t_buf_getbuffer( lua_State *L, int pB, int pP, int *pos );
