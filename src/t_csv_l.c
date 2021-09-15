// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/**
 * \file      t_csv_l.c
 * \brief     OOP wrapper for a Csv reader/writer implementation
 *            Allows for reading/writing Csv/Tsv files of several dialects
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // strchr, strncmp

#include "t_csv_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Push the current partse value as a field into the rusult table
 * \param   L       Lua state.
 * \param   row     t_csv_row struct partsing atete for current row.
 * \param   ste     t_csv_ste enum to be set in row struct after pushing value
 * --------------------------------------------------------------------------*/
static inline void
t_csv_pushfield( lua_State *L, struct t_csv_row *row, enum t_csv_ste ste )
{
	if (0==row->len && T_CSV_FLDBEG==row->ste)
		lua_pushnil( L );
	else
	{
		while ('\r' == row->beg[row->len-1] && row->len > 0) (row->len)--;
		// push a \0 terminated string on stack that can be safely gsub()-ed
		if (row->hdb)
		{
			lua_pushlstring( L, row->beg, row->len );
			luaL_gsub( L, lua_tostring( L, -1 ), row->sdq, row->ssq ); // pushes rinsed result onto stack
			lua_replace( L, -2 );                                      // overwrite original with rinsed result
		}
		else
			lua_pushlstring( L, row->beg, row->len );
	}
#if PRINT_DEBUGS == 3
	printf(" (P:%s:%zu:%d]  ", lua_tostring(L,-1), row->len, row->cnt+1);
#endif
	lua_rawseti( L, -2, ++(row->cnt) );  // push field to row
	row->ste = ste;
}


/**--------------------------------------------------------------------------
 * Eat through a CSV line and add fields to a Lua table on the stack
 * \param   L       Lua state.
 * \param   csv     t_csv struct userdata.
 * --------------------------------------------------------------------------*/
static void
t_csv_parse( lua_State *L, struct t_csv_row *row )
{
	// CSV/TSV parsing finite state machine
	while (row->ste < T_CSV_RECDNE)
	{
#if PRINT_DEBUGS == 3
		printf( " [%s]\t-[%c][%c][%zu][%c]-   ", t_csv_ste_nme[row->ste], *(row->run), *(row->beg), row->len, row->beg[row->len] );
#endif
		switch (row->ste)
		{
			case T_CSV_DATNKD:
				// TODO: handle escape char
				if (row->dlm == *(row->run))                                 // regular field end
				{
					(row->len)++;
					t_csv_pushfield( L, row, T_CSV_FLDBEG );
				}
				else if ('\n' == *(row->run))                                // last field end
				{
					(row->len)++;
					t_csv_pushfield( L, row, T_CSV_RECDNE );
				}
				row->len = (' ' == *(row->run)) ? row->len : row->run - row->beg;       // only progress when not space
				break;
			case T_CSV_DATQTE:
				row->len = row->run - row->beg;                  // don't include last enclosing quote
				if (row->qte == *(row->run))
					row->ste = T_CSV_QTEONE;
				break;
			case T_CSV_QTEONE:
				if (row->dlm == *(row->run))
					t_csv_pushfield( L, row, T_CSV_FLDBEG );
				else if ('\n' == *(row->run))
					t_csv_pushfield( L, row, T_CSV_RECDNE );
				else if (row->qte == *(row->run))
					row->ste = T_CSV_QTETWO;
				else
					row->len = (' ' == *(row->run)) ? row->len : row->run - row->beg;       // only progress when not space
				break;
			case T_CSV_QTETWO:
				row->hdb = 1;
				if (row->qte == *(row->run))
					row->ste    = T_CSV_QTEONE;
				else
					row->ste    = T_CSV_DATQTE;
				row->len = row->run - row->beg;       // only progress when not space
				break;
			case T_CSV_FLDBEG:
				// resetting some values for the new field
				row->beg = row->fld = row->run;
				row->hdb = row->hec = row->len = 0;
				if (row->qte == *(row->run))
					row->ste     = T_CSV_QTE1ST;
				else if (' ' == *(row->run))                         // Whitespace ... just keep eating
					;
				else if (row->dlm == *(row->run))                    // ...,,... empty field aka. NULL value
					t_csv_pushfield( L, row, T_CSV_FLDBEG );
				else if ('\n' == *(row->run))                    // ...,,... empty field aka. NULL value
					t_csv_pushfield( L, row, T_CSV_RECDNE );
				else
					row->ste = T_CSV_DATNKD;
				break;
			case T_CSV_QTE1ST:
				row->beg    = row->run;
				if (row->qte == *(row->run))
					row->ste = T_CSV_QTE2ND;
				else
					row->ste = T_CSV_DATQTE;
				break;
			case T_CSV_QTE2ND:
				if (row->dlm == *(row->run))                        // is ...,"",...
					t_csv_pushfield( L, row, T_CSV_FLDBEG );
				else if ('\n' == *(row->run))                       // is ...,"",...
					t_csv_pushfield( L, row, T_CSV_RECDNE );
				else if (row->qte == *(row->run))
					row->ste = T_CSV_QTETWO;
				break;
			default:
				break;
		}
#if PRINT_DEBUGS == 3
		printf("  [%c][%zu][%c]   [%s]   __%s", *(row->beg), row->len, row->beg[row->len], t_csv_ste_nme[row->ste], row->run );
#endif
		if ('\0' == *(row->run))
			break;
		(row->run)++;
	}
	return;
}

/**--------------------------------------------------------------------------
 * Attempts to read a single line of CSV content
 * \param   L       Lua state.
 * \lparam  string  string containing delimiter character.
 * \lparam  string  string containing quotation character.
 * \lparam  string  string containing escape character.
 * \lparam  bool    is this parsing using double quotation characters?
 * \lparam  string  Single row of CSV content.
 * \lparam  table   Table to push the results into.
 * \lparam  int     Index from where to start pushing fields into.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \lreturn string  String starting at the beginning of last processed field.
 * \lreturn int     Number of fields parsed in this function call.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv_ParseLine( lua_State *L )
{
	//S: dlm qte esc dbl lne tbl cnt
	const char   dlm = luaL_checkstring( L, 1 )[ 0 ];
	const char   qte = luaL_checkstring( L, 2 )[ 0 ];
	const char   esc = luaL_checkstring( L, 3 )[ 0 ];
	const int    dbl = lua_toboolean( L, 4 );
	const char  *lne = luaL_checkstring( L, 5 );
	luaL_argcheck( L, (LUA_TTABLE == lua_type( L, 6 )), 6, "Expected a Table" );

	struct t_csv_row row = {
		.ste = T_CSV_FLDBEG,
		.dlm = dlm,
		.qte = qte,
		.esc = esc,
		.dbl = dbl,
		.run = lne,
		.beg = lne,
		.len = 0,
		.fld = lne,
		.hdb = 0,
		.hec = 0,
		.cnt = luaL_optinteger( L, 7, 0 ),
		.ssq = { qte, 0 },
		.sdq = { qte, qte, 0 },
		.sec = { esc, 0 },
	};
	lua_pop( L, 1 );               // pop the field count
	t_csv_parse( L, &row );
	lua_pushboolean( L, T_CSV_RECDNE == row.ste );
	lua_pushstring( L, row.fld );
	lua_pushinteger( L, row.cnt );
	return 3;
}


/**--------------------------------------------------------------------------
 * Closure to split a string by a delimiter
 * \param   L       Lua state.
 * \uparam  string  Original text string.
 * \uparam  string  Delimiter string.
 * \uparam  int     Index inside uparam text string.
 * \lreturn string  Last token that was split of from text string.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
t_csv_split( lua_State *L )
{
	size_t       lns,lnd; // length input, length delim
	const char  *str = luaL_checklstring( L, lua_upvalueindex( 1 ), &lns );
	const char  *dlm = luaL_checklstring( L, lua_upvalueindex( 2 ), &lnd );
	const int    dst = lua_toboolean( L, lua_upvalueindex( 3 ) );     // don't strip delimiter?
	size_t       its = luaL_checkinteger( L, lua_upvalueindex( 4 ) ); // iter input
	size_t       cnt = luaL_checkinteger( L, lua_upvalueindex( 5 ) ); // token counter
	size_t       itd = 0, tkn = its;                                  // iter delim, token start
	if (its > lns)
		lua_pushnil( L );
	else
	{
		for (; its < lns; its++)
		{
			//printf("   ITS: %zu <%c><%c> ", its, str[its], dlm[itd]);
			itd = (str[its] == dlm[itd])  // string matches delimiter
				? itd+1                    // advance delimiter
				: (str[its] == *dlm)       // first character in delimiter matches
					? 1                     // advance delimiter to one
					: 0;                    // reset delimiter
			//printf(" -- %s\n", lnd == itd ? "Yea" : "Nah");
			if (itd == lnd)               // delimeter completely matched
				break;
		}
		//printf("      TKN: %s  %zu %ld \n", &(str[tkn]), its, its-tkn  - ((its<lns) ? ((dst==1) ? -1 : lnd-1) : 0) );
		lua_pushlstring( L, &(str[tkn]), its-tkn -
			((its<lns)            // Not end of input text?
				? ((dst==1)        // strip delimiter?
					? -1            // Include delimiter
					: lnd-1)        // Exclude delimiter
				: 0)               // last field
		);
		lua_pushinteger( L, its+1 );
		lua_replace( L, lua_upvalueindex( 4 ) );    // update string iterator position
		lua_pushinteger( L, cnt+1 );
		lua_replace( L, lua_upvalueindex( 5 ) );    // update field count
		lua_pushinteger( L, cnt+1 );
	}
	return (its>lns) ? 1 : 2;
}

/**--------------------------------------------------------------------------
 * Function to split a string by a delimiter
 * \param   L       Lua state.
 * \lparam  string  Text string to split.
 * \lparam  string  Delimiter string.
 * \lreturn func    Iterator function that returns the split tokens one by one.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv_Split( lua_State *L )
{
	const char __attribute__ ((unused)) *str = luaL_checkstring( L, 1 );
	const char __attribute__ ((unused)) *dlm = luaL_checkstring( L, 2 );
	if (2==lua_gettop( L ))
		lua_pushboolean( L, lua_toboolean( L, 3 ) ); // don't strip delimiter
	lua_pushinteger( L, 0 );                // str iterator
	lua_pushinteger( L, 0 );                // token counter
	lua_pushcclosure( L, &t_csv_split, 5 );
	return 1;
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
	  {"split"         , lt_csv_Split      }
	, {"parseLine"     , lt_csv_ParseLine  }
	, { NULL           , NULL              }
};

/**--------------------------------------------------------------------------
 * Instance metamethods library definition
 * --------------------------------------------------------------------------*/
//not needed

/**--------------------------------------------------------------------------
 * Pushes this library onto the stack.
 *          - creates Metatable with functions
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_csv( lua_State *L )
{
	// T.Csv class
	luaL_newlib( L, t_csv_cf );
	luaL_newlib( L, t_csv_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

