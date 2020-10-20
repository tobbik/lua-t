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


/** -------------------------------------------------------------------------
 * Constructor - creates the t.Csv instance.
 * \param   L          Lua state.
 * \lparam  CLASS      table Csv.
 * \lreturn instance   Userdata Csv.
 * \return  int        # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_csv_New( lua_State *L )
{
	struct t_csv    *csv = (struct t_csv *) lua_newuserdata( L, sizeof( struct t_csv ) );
	csv->ste       = T_CSV_FLDSTART;  ///< Current parse state
	csv->hR        = LUA_REFNIL;      ///< Reference to file handle in LUA_REGISTRYINDEX
	csv->cnt       = 0;               ///< running field count of current line
	csv->len       = 0;               ///< Length of current line load
	csv->lne       = NULL;            ///< Current line load
	csv->fld       = NULL;            ///< Current start of field
	luaL_getmetatable( L, T_CSV_TYPE );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_csv
 * \param   L      Lua state.
 * \param   int    position on the stack
 * \param   int    check(boolean): if true error out on fail
 * \return  struct t_csv*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_csv
*t_csv_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_CSV_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_CSV_TYPE );
	return (NULL==ud) ? NULL : (struct t_csv *) ud;
}


/**--------------------------------------------------------------------------
 * Eat through a CSV line and add fields to a Lua table on the stack
 * \param   L       Lua state.
 * \param   csv     t_csv struct userdata.
 * --------------------------------------------------------------------------*/
static void
t_csv_parse( lua_State *L, struct t_csv *csv )
{
	int  qc = 0;

	// CSV/TSV parsing finite state machine
	while (T_CSV_ROWDONE != csv->ste)
	{
		//printf( "[%s][%zu][%lu][%d]\t__%s\n", t_csv_ste_nme[csv->ste], csv->run - csv->lne, csv->run - csv->fld, qc, csv->run );
		switch (csv->ste)
		{
			case T_CSV_FLDSTART:
				csv->fld = csv->run;
				qc = 0;
				while (csv->qts[0] == *(csv->run))
				{
					(csv->run)++;
					qc++;
				}
				if ( qc%2 )
				{
					csv->ste = T_CSV_INQUOTES;
					(csv->fld)++; // eat the leading quote
				}
				else
				{
					csv->ste = T_CSV_NOQOUTE;
					if (csv->dlm[0] == *(csv->run) || 0 == *(csv->run))
					{
						lua_pushlstring( L, csv->fld, csv->run - csv->fld );
						csv->ste = T_CSV_FLDEND;
					}
				}
				break;
			case T_CSV_NOQOUTE:
				csv->run = strchr( csv->run, csv->dlm[0] );
				csv->run = (! csv->run)
					? (char * ) csv->lne + csv->len  // no delimiter -> go to end of line
					: csv->run;
				lua_pushlstring( L, csv->fld, csv->run - csv->fld );
				csv->ste = T_CSV_FLDEND;
				break;
			case T_CSV_INQUOTES:
				csv->run = strchr( csv->run, csv->qts[0] );
				if (! csv->run)
				{
					return;
					break;
				}
				// TODO: check for `doublequoted`
				if (csv->qts[0] == *(csv->run + 1)) // escaped quote
					(csv->run) += 2;
				else
				{
					// push a \0 terminated string on stack that can be safely gsub()-ed
					// creates extra copy on stack that needs to be popped
					lua_pushlstring( L, csv->fld, csv->run - csv->fld );
					luaL_gsub( L, lua_tostring( L, -1 ), csv->qtd, csv->qts ); // pushes rinsed result onto stack
					lua_remove( L, -2 );

					(csv->run)++;
					csv->ste = T_CSV_FLDEND;
				}
				break;
			case T_CSV_FLDEND:
				// t_stackDump(L);
				if (csv->run - csv->lne < (long) csv->len)
				{
					(csv->run)++;
					csv->ste = T_CSV_FLDSTART;
				}
				else
				{
					csv->ste = T_CSV_ROWDONE;
				}
				lua_rawseti( L, -2, ++(csv->cnt) );  // adjust cl for Lua 1 based index
				break;
			default:                           // handle as error -> abort
				csv->ste = T_CSV_ROWDONE;
				break;
		}
	}
	return;
}


/**--------------------------------------------------------------------------
 * Attempts to read a single line of CSV content
 * \param   L       Lua state.
 * \lparam  ud      t_csv userdata..
 * \lparam  string  Single row of CSV content.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv_parseLine( lua_State *L )
{
	//S: csv lne tbl
	struct t_csv  *csv = t_csv_check_ud( L, 1, 1 );
	const char    *lne = luaL_checklstring( L, 2, &(csv->len) );

	//t_stackDump( L );

	printf( "[%s]%s\n", t_csv_ste_nme[csv->ste], lne );
	// T_CSV_INQUOTES is the only re-entrant state (handle multi-line fields)
	if (T_CSV_INQUOTES == csv->ste)
	{
		//printf( "[%lu   %lu]  %lld\n ", csv->fld - csv->lne, csv->run - csv->lne, csv->cnt );
		csv->fld  = (char *) lne + (csv->fld - csv->lne);
		csv->run  = csv->fld;
		//csv->run  = (char *) lne + (csv->run - csv->lne);
	}
	else
	{
		csv->fld  = (char *) lne;
		csv->run  = (char *) lne;
		csv->cnt  = lua_rawlen( L, -1 );
		csv->ste  = T_CSV_FLDSTART;
	}
	csv->lne     = lne;
	t_csv_parse( L, csv );
	lua_pushboolean( L, T_CSV_ROWDONE == csv->ste );
	return 1;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC LUA API========================
//
/**--------------------------------------------------------------------------
 * Return Tostring representation of a csv instance.
 * \param   L      Lua state.
 * \lparam  ud     T.Csv userdata instance.
 * \lreturn string Formatted string representing Csv.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv__tostring( lua_State *L )
{
	struct t_csv *csv = t_csv_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_CSV_TYPE"[%s:%s:%s:%s]: %p",
		(csv->dlm[0] == '\t') ? "<TAB>" : csv->dlm,
		csv->qts,
		csv->esc,
		(csv->dbl) ? "true" : "false",
		csv );
	return 1;
}


/**--------------------------------------------------------------------------
 * __index method for Csv.
 * \param   L      Lua state.
 * \lparam  ud     T.Csv userdata instance.
 * \lparam  string Access key value.
 * \lreturn value  based on what's behind __index.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv__index( lua_State *L )
{
	struct t_csv *csv     = t_csv_check_ud( L, 1, 1 );
	size_t        keyLen;
	const char   *key     = luaL_checklstring( L, 2, &keyLen );

	if (0 == strncmp( key, "delimiter", keyLen ))
		lua_pushlstring( L, csv->dlm, 1 );
	else if (0 == strncmp( key, "quotchar", keyLen ))
		lua_pushlstring( L, csv->qts, 1 );
	else if (0 == strncmp( key, "escapechar", keyLen ))
		lua_pushlstring( L, csv->esc, 1 );
	else if (0 == strncmp( key, "doublequoted", keyLen ))
		lua_pushboolean( L, csv->dbl );
	else if (0 == strncmp( key, "state", keyLen ))
		lua_pushstring( L, t_csv_ste_nme[ csv->ste ] );
	else if (0 == strncmp( key, "handle", keyLen ))
		lua_rawgeti( L, LUA_REGISTRYINDEX, csv->hR );
	else if (0 == strncmp( key, "line", keyLen ))
		lua_pushlstring( L, csv->lne, csv->len );
	else
	{
		lua_getmetatable( L, 1 );  //S: seg key _mt
		lua_pushvalue( L, 2 );     //S: seg key _mt key
		lua_gettable( L, -2 );     //S: seg key _mt key fnc
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * __index method for Csv.
 * \param   L      Lua state.
 * \lparam  ud     T.Csv userdata instance.
 * \lparam  string Access key value.
 * \lreturn value  based on what's behind __index.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv__newindex( lua_State *L )
{
	struct t_csv *csv     = t_csv_check_ud( L, 1, 1 );
	size_t        keyLen;
	const char   *key     = luaL_checklstring( L, 2, &keyLen );

	if (0 == strncmp( key, "delimiter", keyLen ))
	{
		csv->dlm[0] = lua_tostring(  L, 3 )[ 0 ];
		csv->dlm[1] = 0;
	}
	else if (0 == strncmp( key, "quotchar", keyLen ))
	{
		csv->qts[0] = lua_tostring(  L, 3 )[ 0 ];
		csv->qts[1] = 0;
		csv->qtd[0] = csv->qts[0];
		csv->qtd[1] = csv->qts[0];
		csv->qtd[2] = 0;
	}
	else if (0 == strncmp( key, "escapechar", keyLen ))
	{
		csv->esc[0]  = lua_tostring(  L, 3 )[ 0 ];
		csv->esc[1] = 0;
	}
	else if (0 == strncmp( key, "doublequoted", keyLen ))
		csv->dbl     = lua_toboolean( L, 3 );
	else if (0 == strncmp( key, "state", keyLen ) )
		csv->ste     = luaL_checkoption( L, 3, "tex", t_csv_ste_nme );
	else if (0 == strncmp( key, "handle", keyLen ) )
		// TODO: either don't allow to set or check it's actually a FILE*
		csv->hR      = luaL_ref( L, LUA_REGISTRYINDEX );
	else if (0 == strncmp( key, "line", keyLen ) )
		csv->lne     = luaL_checklstring( L, 3, &(csv->len) );
	else
		luaL_argerror( L, 2, "Can't set this value in "T_CSV_TYPE );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc method for Csv.
 * \param   L      Lua state.
 * \lparam  ud     T.Csv userdata instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv__gc( lua_State *L )
{
	struct t_csv *csv = t_csv_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, csv->hR );  //S: csv hdl
	luaL_unref( L, LUA_REGISTRYINDEX, csv->hR );
	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_csv_fm [] = {
	  { NULL           , NULL              }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_csv_cf [] = {
	  {"new"           , lt_csv_New        }
	, { NULL           , NULL              }
};

/**--------------------------------------------------------------------------
 * Instance metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_csv_m [] = {
	// metamethods
	  { "__tostring"   , lt_csv__tostring  }
	, { "__index"      , lt_csv__index     }
	, { "__newindex"   , lt_csv__newindex  }
	, { "__gc"         , lt_csv__gc        }
	, { "parseLine"    , lt_csv_parseLine  }
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
	lua_pop( L, 1 );  // balance the stack

	// T.Csv class
	luaL_newlib( L, t_csv_cf );
	luaL_newlib( L, t_csv_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

