/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf.h
 * \brief     data types and interfaces for T.Buffer and T.Buffer.Segment
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#define T_BUF_SEG_NAME  "Segment"

#define T_BUF_SEG_TYPE  T_BUF_TYPE"."T_BUF_SEG_NAME

/// The userdata struct for T.Buffer
struct t_buf {
	size_t   len;   ///<  length of the entire buffer in bytes
	char     b[1];  ///<  pointer to the variable size buffer -> must be last in struct
};

/// The userdata struct for T.Buffer.Segment
struct t_buf_seg {
	int      bR;    ///<  LUA_REGISTRYINDEX reference for t_buf
	char    *b;     ///<  pointer to t_buf->b[idx-1]
	size_t   idx;   ///<  offest from buffer start
	size_t   len;   ///<  length of segment
};


// t_buf.c
// Constructors
int           luaopen_t_buf   ( lua_State *L );
struct t_buf *t_buf_check_ud  ( lua_State *L, int pos, int check );
struct t_buf *t_buf_create_ud ( lua_State *L, size_t n );
// t_buf.c helpers
char         *t_buf_tolstring   ( lua_State *L, int pos, size_t *len, int *cw );
char         *t_buf_checklstring( lua_State *L, int pos, size_t *len, int *cw );
int           t_buf_getHexString( lua_State *L, char *b, size_t len );
int           t_buf_getBinString( lua_State *L, char *b, size_t len );
int           t_buf_compare( lua_State *L, char *bA, char *bB, size_t aLen, size_t bLen );
int          lt_buf_write( lua_State *L );
int          lt_buf_read ( lua_State *L );

// t_buf_seg.c
// Constructors
int               luaopen_t_buf_seg   ( lua_State *L );
struct t_buf_seg *t_buf_seg_check_ud  ( lua_State *L, int pos, int check );
struct t_buf_seg *t_buf_seg_create_ud ( lua_State *L, struct t_buf *buf, size_t idx, size_t len );

