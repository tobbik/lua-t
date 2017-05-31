/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf.c
 * \brief     t_buf_* functions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_buf.h"
#include "t.h"           // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif

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
	if (NULL == ud && check) t_typeerror( L , pos, T_BUF_TYPE );
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
	if (NULL == ud && check) t_typeerror( L , pos, T_BUF_SEG_TYPE );
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
		 "`"T_BUF_TYPE"` or `"T_BUF_SEG_TYPE"` or string expected" );
	return b;
}


/**--------------------------------------------------------------------------
 * Determine if value at pos can be converted into some sort of a char* buf.
 * \param   L     Lua state.
 * \param   pos   position of userdata on stack.
 * \param  *cw    int pointer determining if char* is writable.
 * \lparam  buf  T.Buffer userdata instance.
 *      OR
 * \lparam  seg  T.Buffer.Segment userdata instance.
 *      OR
 * \lparam  b    Lua string.
 * \return  char*  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_buf_isstring( lua_State *L, int pos, int *cw )
{
	struct t_buf     *buf = t_buf_check_ud( L, pos, 0 );
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, pos, 0 );

	if (NULL != buf || NULL != seg || lua_isstring( L , pos ))
	{
		*cw = (lua_isstring( L, pos )) ? 0 : 1;
		return 1;
	}
	else
		return 0;
}
