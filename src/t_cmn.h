/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_common.h
 * \brief     Functions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#define T_NET_SCK_IDNT  "sck"
#define T_BUF_SEG_IDNT  "seg"

#define T_NET_SCK_NAME  "Socket"
#define T_BUF_SEG_NAME  "Segment"

#define T_NET_SCK_TYPE  T_NET_TYPE"."T_NET_SCK_NAME
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


struct t_buf *t_buf_check_ud  ( lua_State *L, int pos, int check );
char         *t_buf_tolstring   ( lua_State *L, int pos, size_t *len, int *cw );
char         *t_buf_checklstring( lua_State *L, int pos, size_t *len, int *cw );
struct t_buf_seg *t_buf_seg_check_ud  ( lua_State *L, int pos, int check );


// ----------------------------- DATA TYPES -----------------------------------
struct t_net_sck {
	int   fd;    ///< socket handle
};

struct t_net_sck *t_net_sck_check_ud ( lua_State *L, int pos, int check );
