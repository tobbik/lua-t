/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_common.c
 * \brief     Functions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t.h"
#include "t_cmn.h"

//  _     ____         __  __
// | |_  | __ ) _   _ / _|/ _| ___ _ __
// | __| |  _ \| | | | |_| |_ / _ \ '__|
// | |_ _| |_) | |_| |  _|  _|  __/ |
//  \__(_)____/ \__,_|_| |_|  \___|_|

/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_buf struct and return it
 * \param  L    the Lua State
 * \param  pos  position on the stack
 *
 * \return struct t_buf* pointer to t_buf struct
 * --------------------------------------------------------------------------*/
struct t_buf
*t_buf_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_BUF_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_BUF_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_buf *) ud;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_buf_seg struct and return it
 * \param  L    the Lua State
 * \param  pos  position on the stack
 *
 * \return struct t_buf* pointer to t_buf struct
 * --------------------------------------------------------------------------*/
struct t_buf_seg
*t_buf_seg_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_BUF_SEG_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_BUF_SEG_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_buf_seg *) ud;
}


/**--------------------------------------------------------------------------
 * Get the char *buffer from either a T.Buffer or a T.Buffer.Segment or Lua string.
 * \param   L     Lua state.
 * \param   pos   position of userdata on stack.
 * \param  *len   size_t pointer that holds avilable length in char*.
 * \param  *cw    int pointer determining if char* is writable.
 * \lparam  buf  T.Buffer userdata instance.
 *      OR
 * \lparam  seg  T.Buffer.Segment userdata instance.
 *      OR
 * \lparam  b    Lua string.
 * \return  char*  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
char
*t_buf_tolstring( lua_State *L, int pos, size_t *len, int *cw )
{
	struct t_buf     *buf = t_buf_check_ud( L, pos, 0 );
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, pos, 0 );

	if (NULL != buf)
	{
		*len = buf->len;
		if (NULL!=cw) *cw  = 1;
		return (char*) &(buf->b[0]);
	}
	else if (NULL != seg)
	{
		*len = seg->len;
		if (NULL!=cw) *cw  = 1;
		return seg->b;
	}
	else if (lua_isstring( L, pos))
	{
		if (NULL!=cw) *cw = 0;
		return (char*) luaL_checklstring( L, pos, len );
	}
	else
		return NULL;
}


/**--------------------------------------------------------------------------
 * Get the char *buffer from either a T.Buffer or a T.Buffer.Segment or Lua string.
 * \param   L     Lua state.
 * \param   pos   position of userdata on stack.
 * \param  *len   size_t pointer that holds avilable length in char*.
 * \param  *cw    int pointer determining if char* is writable.
 * \lparam  buf  T.Buffer userdata instance.
 *      OR
 * \lparam  seg  T.Buffer.Segment userdata instance.
 *      OR
 * \lparam  b    Lua string.
 * \return  char*  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
char
*t_buf_checklstring( lua_State *L, int pos, size_t *len, int *cw )
{
	char *b = t_buf_tolstring( L, pos, len, cw );
	luaL_argcheck( L, (b != NULL), pos,
		 "`"T_BUF_SEG_TYPE"` or `"T_BUF_SEG_TYPE"` or string expected" );
	return b;
}



//  _     _   _      _
// | |_  | \ | | ___| |_
// | __| |  \| |/ _ \ __|
// | |_ _| |\  |  __/ |_
//  \__(_)_| \_|\___|\__|

/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net.
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \return  struct t_net_sck*  pointer to the struct t_net_sck.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_SCK_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_NET_SCK_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_net_sck *) ud;
}


