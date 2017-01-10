/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck_cmb.c
 * \brief     Helper functions for T.Pack read format strings.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t.h"
#include "t_pck.h"


// ###########################################################################
//                                HELPERS adapted from Lua 5.3 Source
/** -------------------------------------------------------------------------
 * See if int represents a character which is a digit.
 * \param     int c
 * \return    boolean 0:false - 1:true
 *  -------------------------------------------------------------------------*/
static int
is_digit( int c ) { return '0' <= c && c<='9'; }


/** -------------------------------------------------------------------------
 * reads from string until input is not numeric any more.
 * \param   char** format string
 * \param   int    default value
 * \return  int    read numeric value
 *  -------------------------------------------------------------------------*/
static int
gn( const char **fmt, int df )
{
	if (! is_digit(** fmt))    // no number
		return df;
	else
	{
		int a=0;
		do
		{
			a = a*10+ *((*fmt)++) - '0';
		} while (is_digit(**fmt) &&  a <(INT_MAX/10 - 10));
		return a;
	}
}


/** -------------------------------------------------------------------------
 * Read an integer from the format parser
 * raises an error if it is larger than the maximum size for integers.
 * \param  char* format string
 * \param  int   default value if no number is in the format string
 * \param  int   max value allowed for int
 * \return the converted integer value
 *  -------------------------------------------------------------------------*/
static int
gnl( lua_State *L, const char **fmt, int df, int max )
{
	int sz = gn( fmt, df );
	if (sz > max || sz <= 0)
		luaL_error( L, "size (%d) out of limits [1,%d]", sz, max );
	return sz;
}


/** -------------------------------------------------------------------------
 * Determines type of Packer from format string.
 * Returns the Packer, or NULL if unsuccessful.  Leaves created packer on the
 * stack.
 * \param   L      Lua state.
 * \param   char*  format string pointer. moved by this function.
 * \param   int*   e pointer to current endianess.
 * \param   int*   bo pointer to current bit offset within byte.
 * \lreturn ud     T.Pack userdata instance.
 * \return  struct t_pck* pointer.
 * TODO: Deal with bit sized Packers:
 *       - Detect if we are in Bit sized type(o%8 !=0)
 *       - Detect if fmt switched back to byte style and ERROR
 *  -------------------------------------------------------------------------*/
struct t_pck
*t_pck_fmt_read( lua_State *L, const char **f, int *e, size_t *bo )
{
	int           opt;
	int           m;
	size_t        s;
	enum t_pck_t  t;
	struct t_pck *p = NULL;

	while (NULL == p)
	{
		opt = *((*f)++);
		//printf("'%c'   %02X\n", opt, opt);
		switch (opt)
		{
			// Integer types
			case 'b': t = T_PCK_INT;  m = (1==*e);  s = 1;                                 break;
			case 'B': t = T_PCK_UNT;  m = (1==*e);  s = 1;                                 break;
			case 'h': t = T_PCK_INT;  m = (1==*e);  s = sizeof( short );                   break;
			case 'H': t = T_PCK_UNT;  m = (1==*e);  s = sizeof( short );                   break;
			case 'l': t = T_PCK_INT;  m = (1==*e);  s = sizeof( long );                    break;
			case 'L': t = T_PCK_UNT;  m = (1==*e);  s = sizeof( long );                    break;
			case 'j': t = T_PCK_INT;  m = (1==*e);  s = sizeof( lua_Integer );             break;
			case 'J': t = T_PCK_UNT;  m = (1==*e);  s = sizeof( lua_Integer );             break;
			case 'T': t = T_PCK_INT;  m = (1==*e);  s = sizeof( size_t );                  break;
			case 'i': t = T_PCK_INT;  m = (1==*e);  s = gnl( L, f, sizeof( int ), MXINT ); break;
			case 'I': t = T_PCK_UNT;  m = (1==*e);  s = gnl( L, f, sizeof( int ), MXINT ); break;

			// Float types
			case 'f': t = T_PCK_FLT;  m = (1==*e);  s = sizeof( float );                   break;
			case 'd': t = T_PCK_FLT;  m = (1==*e);  s = sizeof( double );                  break;
			case 'n': t = T_PCK_FLT;  m = (1==*e);  s = sizeof( lua_Number );              break;

			// String type
			case 'c': t = T_PCK_RAW;  m = 0;        s = gnl( L, f, 1, 0x1 << NB );         break;

			// Bit types
			case 'v':
				t = T_PCK_BOL;
				m = gnl(L, f, 1+(*bo%NB), NB) - 1;
				s = 1;
				break;
			case 'r':
				t = T_PCK_BTS;
				m = *bo % NB;
				s = gnl(L, f, 1, MXBIT );
				break;
			case 'R':
				t = T_PCK_BTU;
				m = *bo % NB;
				s = gnl(L, f, 1, MXBIT );
				break;

			// modifier types
			case '<': *e = 1; continue;                                                      break;
			case '>': *e = 0; continue;                                                      break;
			case '\0': return NULL;                                                          break;
			default:
				luaL_error( L, "invalid format option '%c'", opt );
				return NULL;
		}
		// TODO: check if 0==offset%8 if byte type, else error
		p    = t_pck_create_ud( L, t, s, m );
		// forward the Bit offset
		*bo += ((T_PCK_BTU==t || T_PCK_BTS==t || T_PCK_BOL == t) ? s : s*NB);
	}
	return p;
}

