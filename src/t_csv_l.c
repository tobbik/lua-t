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

/**--------------------------------------------------------------------------
 * Eat through a CSV line and add fields to a Lua table on the stack
 * \param   L       Lua state.
 * \lparam  table   T.Csv table instance.
 * \lparam  string  Single row of CSV content.
 * \lparam  string  Delimiter character.
 * \lparam  string  Quotation character.
 * \lparam  boolean Is it double quoted?
 * \lparam  string  Escape character.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
//enum t_csv_ste iter_fields( lua_State *L, char *lne, size_t len, char dlm, char qot, char esc, int dbl )
enum t_csv_ste
iter_fields( lua_State *L, char *lne, size_t len, char dlm, char qot )
{
	enum t_csv_ste ste = T_CSV_FLDSTART;
	char *s  = lne, *r = lne;          // start, runner
	int	cl = lua_rawlen( L, 4 );     // running index result table
	int	qc = 0;                      // count qoutes in a row

	// CSV/TSV parsing finite state machine
	while (T_CSV_ROWDONE != ste)
	{
		//printf( "[%s][%zu][%lu][%d]\t__%s\n", states[ste], r-lne, r-s, qc, r );
		switch (ste)
		{
			case T_CSV_FLDSTART:
				s = r;
				qc = 0;
				while (qot == *r)
				{
					r++;
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
					if (dlm == *r || 0 == *r)
					{
						lua_pushlstring( L, s, r-s );
						ste = T_CSV_FLDEND;
					}
				}
				break;
			case T_CSV_NOQOUTE:
				r = strchr( r, dlm );
				r = (! r)
					? lne+len  // no delimiter -> go to end of line
					: r;
				lua_pushlstring( L, s, r-s );
				ste = T_CSV_FLDEND;
				break;
			case T_CSV_INQUOTES:
				r = strchr( r, qot );
				if (! r)
				{
					return T_CSV_ROWTRUNCED;
					break;
				}
				// TODO: check for `doublequoted`
				if (qot == *(r+1)) // escaped quote
					r+=2;
				else
				{
					//lua_pushlstring( L, s, r-s );
					//luaL_gsub( L, lua_tostring( L, -1 ), "\"\"", "\"" ); // pushes rinsed result onto stack
					//lua_remove( L, -2 );

					// Dirty: very briefly replace the final quote with a \0 to allow
					// the use luaL_gsub
					*r = '\0';
					luaL_gsub( L, s, "\"\"", "\"" ); // pushes rinsed result onto stack
					*r = qot;

					r++;
					ste = T_CSV_FLDEND;
				}
				break;
			case T_CSV_FLDEND:
				lua_rawseti( L, 4, ++cl );  // adjust cl for Lua 1 based index
				if (r-lne < (long) len)
				{
					r++;
					ste = T_CSV_FLDSTART;
				}
				else
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
 * Attempts to read a single line of CSV content
 * \param   L       Lua state.
 * \lparam  table   T.Csv table instance.
 * \lparam  string  Single row of CSV content.
 * \lparam  string  Delimiter character.
 * \lparam  string  Quotation character.
 * \lparam  boolean Is it double quoted?
 * \lparam  string  Escape character.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_csv_parseLine( lua_State *L )
{
	//S: csv lne tbl dlm qot dbl esc
	//t_stackDump( L );
	struct t_csv  *csv = (struct t_csv *) lua_touserdata( L, 3 );
	const char   *lne = lua_tolstring( L, 2, &(csv->len) );
	const char    dlm = csv->dlm;
	const char    qot = csv->qot;
	const char    esc = csv->esc;
	const int     dbl = csv->dbl;
	//t_stackDump( L );
	//enum t_csv_ste p_state = iter_fields( L, (char *) lne, len, dlm, qot, esc, dbl );
	enum t_csv_ste ste = iter_fields( L, (char *) lne, csv->len, dlm, qot );
	lua_pushboolean( L, T_CSV_ROWDONE == ste );
	return 1;
}





/**--------------------------------------------------------------------------
 * Eat through a CSV line and add fields to a Lua table on the stack
 * \param   L       Lua state.
 * \lparam  table   T.Csv table instance.
 * \lparam  string  Single row of CSV content.
 * \lparam  string  Delimiter character.
 * \lparam  string  Quotation character.
 * \lparam  boolean Is it double quoted?
 * \lparam  string  Escape character.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
void
t_csv_parse( lua_State *L, struct t_csv *csv )
{
	int  qc  = 0;

	// CSV/TSV parsing finite state machine
	while (T_CSV_ROWDONE != csv->ste)
	{
		//printf( "[%s][%zu][%lu][%d]\t__%s\n", states[csv->ste], csv->run - csv->lne, csv->run - csv->fld, qc, csv->run );
		switch (csv->ste)
		{
			case T_CSV_FLDSTART:
				csv->fld = csv->run;
				qc = 0;
				while (csv->qot == *(csv->run))
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
					if (csv->dlm == *(csv->run) || 0 == *(csv->run))
					{
						lua_pushlstring( L, csv->fld, csv->run - csv->fld );
						csv->ste = T_CSV_FLDEND;
					}
				}
				break;
			case T_CSV_NOQOUTE:
				csv->run = strchr( csv->run, csv->dlm );
				csv->run = (! csv->run)
					? (char * ) csv->lne + csv->len  // no delimiter -> go to end of line
					: csv->run;
				lua_pushlstring( L, csv->fld, csv->run - csv->fld );
				csv->ste = T_CSV_FLDEND;
				break;
			case T_CSV_INQUOTES:
				csv->run = strchr( csv->run, csv->qot );
				if (! csv->run)
				{
					return;
					break;
				}
				// TODO: check for `doublequoted`
				if (csv->qot == *(csv->run + 1)) // escaped quote
					(csv->run) += 2;
				else
				{
					// Clean: push a \0 terminated string that can be safely gsub()
					// treated. creates extra copy that needs to be popped
					//lua_pushlstring( L, csv->fld, csv->run - csv->fld );
					//luaL_gsub( L, lua_tostring( L, -1 ), csv->dqot, csv->sqot ); // pushes rinsed result onto stack
					//lua_remove( L, -2 );

					// Dirty: very briefly replace the final quote with a \0 to allow
					// the use luaL_gsub
					*(csv->run) = '\0';
					luaL_gsub( L, csv->fld, csv->dqot, csv->sqot ); // pushes rinsed result onto stack
					*(csv->run) = csv->qot;

					(csv->run)++;
					csv->ste = T_CSV_FLDEND;
				}
				break;
			case T_CSV_FLDEND:
				lua_rawseti( L, 4, ++(csv->cnt) );  // adjust cl for Lua 1 based index
				if (csv->run - csv->lne < (long) csv->len)
				{
					(csv->run)++;
					csv->ste = T_CSV_FLDSTART;
				}
				else
					csv->ste = T_CSV_ROWDONE;
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
 * \lparam  table   T.Csv table instance.
 * \lparam  string  Single row of CSV content.
 * \lparam  string  Delimiter character.
 * \lparam  string  Quotation character.
 * \lparam  boolean Is it double quoted?
 * \lparam  string  Escape character.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_csv_pLine( lua_State *L )
{
	//S: csv lne tbl dlm qot dbl esc
	size_t         len;
	struct t_csv  *csv = (struct t_csv *) lua_touserdata( L, 3 );

	//t_stackDump( L );

	csv->lne  = lua_tolstring( L, 2, &(csv->len) );
	csv->fld  = (char *) csv->lne;
	csv->run  = (char *) csv->lne;
	csv->cnt  = 0;
	csv->ste  = T_CSV_FLDSTART;
	t_csv_parse( L, csv );
	return 0;
}


/**--------------------------------------------------------------------------
 * Build a parser/writer state
 * \param   L       Lua state.
 * \lparam  table   T.Csv table instance.
 * \lparam  string  Single row of CSV content.
 * \lparam  string  Delimiter character.
 * \lparam  string  Quotation character.
 * \lparam  boolean Is it double quoted?
 * \lparam  string  Escape character.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_csv_buildState( lua_State *L )
{
	//S: csv dlm qot esc dbl
	size_t        len;

	struct t_csv *csv = (struct t_csv *) lua_newuserdata( L, sizeof( struct t_csv ) );
	t_stackDump( L );
	csv->dlm          = lua_tostring(  L, 2 )[ 0 ];
	csv->qot          = lua_tostring(  L, 3 )[ 0 ];
	csv->esc          = lua_tostring(  L, 4 )[ 0 ];
	csv->dbl          = lua_toboolean( L, 5 );
	csv->dqot[0]      = csv->qot;
	csv->dqot[1]      = csv->qot;
	csv->dqot[2]      = 0;
	csv->sqot[0]      = csv->qot;
	csv->sqot[1]      = 0;
	t_stackDump( L );
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
	, { "p"            , lt_csv_pLine }
	, { "build"        , lt_csv_buildState }
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

