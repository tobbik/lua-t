/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_csv_l.c
 * \brief     OOP wrapper for a Csv reader/writer implementation
 *            Allows for reading/writing Csv/Tsv files of several dialects
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset,memcpy

#include "t_csv_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

// the handle_field callback has the following arguments
// handle_field( column number, field value, charlen of field_value, is last in row )
enum t_csv_ste iter_fields( lua_State *L, char *line )
{
	enum t_csv_ste ste = T_CSV_FLDSTART;
	char *s = line, *e = line;           // start, end
	int	cl = -1;                       // gets instantly set to 0
	int	qc = 0;                        // count qoutes in a row

	// CSV/TSV parsing finite state machine
	while (T_CSV_ROWDONE != ste)
	{
		printf( "[%s][%lu]\t__%s\n", states[ste], e-s, e);
		switch (ste)
		{
			case T_CSV_FLDSTART:
				e = s;
				cl++;
				qc = 0;
				while ('"' == *e)
				{
					e++;
					qc++;
				}
				if ( qc%2 )
				{
					ste = T_CSV_INQUOTES;
					s++; // eat the leading quote
				}
				else
				{
					ste = T_CSV_NOQOUTE;
					if ('\t' == *e)
						ste = T_CSV_FLDEND;
					if ('\n'==*e)
						ste = T_CSV_ROWEND;
				}
				break;
			case T_CSV_NOQOUTE:
				e = strchr( e, '\t' );
				if (! e)
				{
					e = strchr( s, 0 );
					ste = T_CSV_ROWEND;
				}
				else
					ste = T_CSV_FLDEND;
				break;
			case T_CSV_INQUOTES:
				e = strchr( e, '"' );
				//if (! e)
				//{
				//	ste = T_CSV_ROWTRUNCED;
				//	break;
				//}
				if ('"' == *(e+1)) // escaped quote
					e+=2;
				else
				{
					e++;
					ste = ('\n' == *e) ? T_CSV_ROWEND : T_CSV_FLDEND;
					e--;// skip the last quote
				}
				break;
			case T_CSV_FLDEND:
				lua_pushlstring( L, s, e-s );
				lua_rawseti( L, 3, cl+1 );  // adjust cl for Lua 1 based index
				s = e+1;
				e = s;
				ste = T_CSV_FLDSTART;
				break;
			case T_CSV_ROWEND:
				lua_pushlstring( L, s, e-s );
				lua_rawseti( L, 3, cl+1 );  // adjust cl for Lua 1 based index
				ste = T_CSV_ROWDONE;
				break;
			default:                           // handle as error -> abort
				ste = T_CSV_ROWDONE;
				break;
		}
	}
	return ste;
}


/**--------------------------------------------------------------------------
 * Gets the content of the buffer as a hexadecimal string.
 * \param   L       Lua state.
 * \lparam  ud      T.Buffer userdata instance.
 * \lreturn string  T.Buffer representation as hexadecimal string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_csv_parseLine( lua_State *L )
{
	t_stackDump(L);
	size_t  len;
	const char   *line = lua_tolstring( L, 2, &len );
	t_stackDump(L);
	// printf("LEN: %lu\n", len);
	lua_createtable( L, 0, 0 );
	enum t_csv_ste p_state = iter_fields( L, (char *) line );
	UNUSED( p_state );
	return 1;
}

/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_csv_fm [] = {
	  { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_csv_cf [] = {
	  { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_csv_m [] = {
	// metamethods
	  { "parse"        , lt_csv_parseLine }
	, { NULL           , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes this library onto the stack.
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_csv( lua_State *L )
{
	// T.Csv instance metatable
	luaL_newmetatable( L, T_CSV_TYPE );
	luaL_setfuncs( L, t_csv_m, 0 );

	// T.Csv class
	luaL_newlib( L, t_csv_cf );
	luaL_newlib( L, t_csv_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

