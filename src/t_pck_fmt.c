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
 * \return int   converted integer value in tbit
 *  -------------------------------------------------------------------------*/
static int
gnl( lua_State *L, const char **fmt, int df, int max )
{
	int sz = gn( fmt, df );
	if (sz > max || sz <= 0)
		luaL_error( L, "size (%d) out of limits [1,%d]", sz, max );
	return sz * NB;
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
	struct t_pck *p = NULL;

#define CP( typ, end, sz ) \
	t_pck_create_ud( L, T_PCK_##typ, (sz), (end) );
	while (NULL == p)
	{
		opt = *((*f)++);
		//printf("'%c'   %02X\n", opt, opt);
		switch (opt)
		{
			// Integer types
			case 'b': p = CP( INT, 1==*e, NB                                ); break;
			case 'B': p = CP( UNT, 1==*e, NB                                ); break;
			case 'h': p = CP( INT, 1==*e, sizeof( short ) * NB              ); break;
			case 'H': p = CP( UNT, 1==*e, sizeof( short ) * NB              ); break;
			case 'l': p = CP( INT, 1==*e, sizeof( long ) * NB               ); break;
			case 'L': p = CP( UNT, 1==*e, sizeof( long ) * NB               ); break;
			case 'j': p = CP( INT, 1==*e, sizeof( lua_Integer ) * NB        ); break;
			case 'J': p = CP( UNT, 1==*e, sizeof( lua_Integer ) * NB        ); break;
			case 'T': p = CP( INT, 1==*e, sizeof( size_t ) * NB             ); break;
			case 'i': p = CP( INT, 1==*e, gnl( L, f, sizeof( int ), MXINT ) ); break;
			case 'I': p = CP( UNT, 1==*e, gnl( L, f, sizeof( int ), MXINT ) ); break;

			// Float typesCP
			case 'f': p = CP( FLT, 1==*e, sizeof( float ) * NB              ); break;
			case 'd': p = CP( FLT, 1==*e, sizeof( double ) * NB             ); break;
			case 'n': p = CP( FLT, 1==*e, sizeof( lua_Number ) * NB         ); break;

			// String typeCP
			case 'c': p = CP( RAW, 0    , gnl( L, f, 1, 0x1 << NB )         ); break;

			// Bit types
			case 'r': p = CP( BTS, 0    , gnl( L, f, 1, MXBIT )             ); break;
			case 'R': p = CP( BTU, 0    , gnl( L, f, 1, MXBIT )             ); break;
			case 'v': p = CP( BOL, 0    , gnl( L, f, 1+(*bo%NB), NB )       ); break;

			// modifier types
			case '<': *e = 1; continue;                                        break;
			case '>': *e = 0; continue;                                        break;
			case '\0': return NULL;                                            break;
			default:
				luaL_error( L, "invalid format option '%c'", opt );
				return NULL;
		}
	}
#undef CP
	return p;
}

