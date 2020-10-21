/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck__format.c
 * \brief     Read and interpret packer information from the stack
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */
#include <string.h>     // memcpy

#include "t_pck_l.h"
#include "t_buf.h"
#include "t.h"          // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif


/* Lifted from Lua-5.3+ source
** Some sizes are better limited to fit in 'int', but must also fit in
** 'size_t'. (We assume that 'lua_Integer' cannot be smaller than 'int'.)
*/
#define MAX_SIZET ((size_t)(~(size_t)0))

#define MAXSIZE  \
   (sizeof(size_t) < sizeof(int) ? MAX_SIZET : (size_t)(INT_MAX))

// Function helpers
//

// ###########################################################################
//                                HELPERS adapted from Lua 5.3 Source

/** -------------------------------------------------------------------------
 * reads from string until input is not numeric any more.
 * \param   fmt    char**; format string
 * \param   dft    int   ; default value
 * \return  a      int   ; read numeric value
 *  -------------------------------------------------------------------------*/
static size_t
t_pck_parseSize( const char **fmt, size_t dft )
{
	if (! T_PCK_ISDIGIT( **fmt ))    // no number
		return dft;
	else
	{
		size_t a = 0;
		do
		{
			a = a*10 + *((*fmt)++) - '0';
		} while (T_PCK_ISDIGIT(**fmt) && a<(MAXSIZE - 9)/NB);
		return a;
	}
}


/** -------------------------------------------------------------------------
 * Read an integer from the format parser
 * raises an error if it is larger than the maximum size for integers.
 * \param   fmt    char**; format string
 * \param   dft    int   ; default value
 * \param   max    int   ; max value allowed for int
 * \return  size   int   ; read size
 *  -------------------------------------------------------------------------*/
static size_t
t_pck_getSizeFromFormat( lua_State *L, const char **fmt, int dft, size_t max )
{
	size_t sz = t_pck_parseSize( fmt, dft );
	if (sz > max || sz <= 0)
		return (size_t) luaL_error( L, "size (%d) out of limits [1 … %d]", sz, max );
	return sz;
}


/** -------------------------------------------------------------------------
 * Determines type of Packer from format string.
 * Returns the Packer, or NULL if unsuccessful.  Leaves created packer on the
 * stack.
 * \param   *L      Lua state.
 * \param   **f     const char** format string pointer. Function moves pointer.
 * \param   *e      int* pointer. Value holds current endianess.
 * \lreturn ud      t.Pack userdata instance.
 * \return  *p      struct t_pck* pointer.
 * TODO: Deal with bit sized Packers:
 *       - Detect if we are in Bit sized type(o%8 !=0)
 *       - Detect if fmt switched back to byte style and ERROR
 *  -------------------------------------------------------------------------*/
static struct t_pck
*t_pck_parseFmt( lua_State *L, const char **f, int *e )
{
	int           opt;
	struct t_pck *p = NULL;

#define G( sz, mx ) \
   t_pck_getSizeFromFormat( L, f, sz, mx )

#define C( typ, ltl, sgn, sz ) \
   t_pck_create_ud( L, T_PCK_##typ, (sz), \
     ( ((ltl) ? T_PCK_MOD_LITTLE : 0) | ((sgn) ? T_PCK_MOD_SIGNED : 0) ) );

	while (NULL == p)
	{
		opt = *((*f)++);
		//printf("'%c'   %02X\n", opt, opt);
		switch (opt)
		{
			// Integer types
			case 'b': p = C( INT, 1==*e, 1,                         NB ); break;
			case 'B': p = C( INT, 1==*e, 0,                         NB ); break;
			case 'h': p = C( INT, 1==*e, 1, sizeof( short )       * NB ); break;
			case 'H': p = C( INT, 1==*e, 0, sizeof( short )       * NB ); break;
			case 'l': p = C( INT, 1==*e, 1, sizeof( long )        * NB ); break;
			case 'L': p = C( INT, 1==*e, 0, sizeof( long )        * NB ); break;
			case 'j': p = C( INT, 1==*e, 1, sizeof( lua_Integer ) * NB ); break;
			case 'J': p = C( INT, 1==*e, 0, sizeof( lua_Integer ) * NB ); break;
			case 'T': p = C( INT, 1==*e, 0, sizeof( size_t )      * NB ); break;
			case 'i': p = C( INT, 1==*e, 1, G( SZINT, MXINT )     * NB ); break;
			case 'I': p = C( INT, 1==*e, 0, G( SZINT, MXINT )     * NB ); break;

			// Float types
			case 'f': p = C( FLT, 1==*e, 0, sizeof( float )       * NB ); break;
			case 'd': p = C( FLT, 1==*e, 0, sizeof( double )      * NB ); break;
			case 'n': p = C( FLT, 1==*e, 0, sizeof( lua_Number )  * NB ); break;

			// String type
			case 'c': p = C( RAW,     0, 0, G( 1, MAXSIZE )       * NB ); break;

			// Bit types
			case 'v': p = C( BOL,     0, 0, 1                          ); break;
			case 'r': p = C( INT,     0, 1, G( 1, MXBIT )              ); break;
			case 'R': p = C( INT,     0, 0, G( 1, MXBIT )              ); break;

			// modifier types
			case '<': *e = 1;                                             continue;
			case '>': *e = 0;                                             continue;

			// allow spaces as meaningless separators
			case ' ':                                                     continue;
			// that's the end of it
			case '\0':                                                    return NULL;
			default:
				luaL_error( L, "invalid format option '%c'", opt );
				return NULL;
		}
	}
#undef C
#undef G
	return p;
}


/**--------------------------------------------------------------------------
 * Decides if the element on pos is a packer kind of type.
 * It decides between the following options:
 *     - t.Pack type              : just return it
 *     - t.Pack.Index             : return referenced packer location identifier
 *     - fmt string of single item: fetch from cache or create
 *     - fmt string of multp items: let Sequence constructor handle and return result
 * \param   L      Lua state.
 * \param   pos    position on stack.
 * \param   int*   bit offset runner.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_getPacker( lua_State *L, int pos )
{
	struct t_pck *p = NULL; ///< packer
	int           l = _default_endian;
	int           n = 0;    ///< counter for packers created from fmt string
	int           t = lua_gettop( L );  ///< top of stack before operations
	const char   *fmt;

	pos = t_getAbsPos( L, pos );

	// T.Pack or T.Pack.Index at pos
	if (lua_isuserdata( L, pos ))
		p    = t_pck_idx_getPackFromStack( L, pos, NULL );
		//p    = t_pck_idx_getPackFromFieldOnStack( L, pos, NULL, 1 );
	else // format string at pos
	{
		fmt = luaL_checkstring( L, pos );
		p   = t_pck_parseFmt( L, &fmt, &l );
		while (NULL != p )
		{
			n++;
			p = t_pck_parseFmt( L, &fmt, &l );
		}
		if (n > 1)
			p =  t_pck_seq_create( L, t+1, lua_gettop( L ) );
		else
			p = t_pck_check_ud( L, -1, 1 );
		lua_replace( L, pos );
	}
	return p;
}

