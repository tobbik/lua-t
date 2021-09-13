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
#include <string.h>               // strchr, strncmp

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
	csv->ste       = T_CSV_FLDBEG;    ///< Current parse state
	csv->ste       = 0;               ///< Current parse state
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


static inline enum t_csv_ste
t_csv_pushvalue( lua_State *L, const char *val, int len, enum t_csv_ste ste, char rpl )
{
	char sub[2] = { rpl, 0 }, fnd[3] = { rpl, rpl, 0 };
	if (NULL == val)
		lua_pushnil( L );
	else
	{
		while ('\r' == val[len-1] && len > 0) len--;
		// push a \0 terminated string on stack that can be safely gsub()-ed
		// creates extra copy on stack that needs to be popped
		if (rpl)
		{
			lua_pushlstring( L, val, len );
			luaL_gsub( L, lua_tostring( L, -1 ), fnd, sub ); // pushes rinsed result onto stack
			lua_remove( L, -2 );
		}
		else
			lua_pushlstring( L, val, len );       // ,"", -> empty string
	}
	//t_stackDump(L);
	//printf(" P:[%s(%llu)]  ", lua_tostring(L,-1), lua_rawlen( L, -2 ) +1);
	lua_rawseti( L, -2, lua_rawlen( L, -2 ) +1 );  // push field to row
	return ste;
}

/**--------------------------------------------------------------------------
 * Eat through a CSV line and add fields to a Lua table on the stack
 * \param   L       Lua state.
 * \param   csv     t_csv struct userdata.
 * --------------------------------------------------------------------------*/
static void
t_csv_parse( lua_State *L, struct t_csv *csv, const char *run )
{
	const char *val_start = run;
	const char *val_stop  = run;
	int  has_dbl_qte =0, has_esc_chr=0;

	// CSV/TSV parsing finite state machine
	while (csv->ste < T_CSV_RECDNE)
	{
		//printf( " [%s]\t-[%c][%c][%c]-   ", t_csv_ste_nme[csv->ste], *run, *val_start, *val_stop );
		switch (csv->ste)
		{
			case T_CSV_DATNKD:
				// TODO: handle escape char
				if (csv->dlm == *run)                            // regular field end
					csv->ste = t_csv_pushvalue( L, val_start, val_stop - val_start +1, T_CSV_FLDBEG, '\0' );
				else if ('\n' == *run)                           // last field end
					csv->ste = t_csv_pushvalue( L, val_start, val_stop - val_start +1, T_CSV_RECDNE, '\0' );
				val_stop = (' ' == *run) ? val_stop : run;       // only progress when not space
				break;
			case T_CSV_DATQTE:
				val_stop = run;                            // don't include last enclosing quote
				if (csv->qte == *run)
					csv->ste = T_CSV_QTEONE;
				break;
			case T_CSV_QTEONE:
				if (csv->dlm == *run)
					csv->ste = t_csv_pushvalue( L, val_start, val_stop - val_start, T_CSV_FLDBEG, (has_dbl_qte) ? csv->qte : '\0' );
				else if ('\n' == *run)
					csv->ste = t_csv_pushvalue( L, val_start, val_stop - val_start, T_CSV_RECDNE, (has_dbl_qte) ? csv->qte : '\0' );
				else if (csv->qte == *run)
					csv->ste = T_CSV_QTETWO;
				else
					val_stop = (' ' == *run) ? val_stop : run;    // only progress when not space
				break;
			case T_CSV_QTETWO:
				has_dbl_qte = 1;
				if (csv->qte == *run)
					csv->ste    = T_CSV_QTEONE;
				else
					csv->ste    = T_CSV_DATQTE;
				val_stop = run;
				break;
			case T_CSV_FLDBEG:
				has_dbl_qte = has_esc_chr = 0;                // reset doublequote marker
				val_start = val_stop = csv->fld = run;
				if (csv->qte == *run)
					csv->ste     = T_CSV_QTE1ST;
				else if (' ' == *run)                         // Whitespace ... just keep eating
					;
				else if (csv->dlm == *run)                    // ...,,... empty field aka. NULL value
					csv->ste = t_csv_pushvalue( L, NULL, 0, T_CSV_FLDBEG, '\0' );    // pushing nil
				else
					csv->ste = T_CSV_DATNKD;
				break;
			case T_CSV_QTE1ST:
				val_start    = run;
				if (csv->qte == *run)
					csv->ste = T_CSV_QTE2ND;
				else
					csv->ste = T_CSV_DATQTE;
				break;
			case T_CSV_QTE2ND:
				if (csv->dlm == *run)                        // is ...,"",...
					csv->ste = t_csv_pushvalue( L, val_start, 0, T_CSV_FLDBEG, '\0' );   // pushing empty string
				else if (csv->qte == *run)
					csv->ste = T_CSV_QTETWO;
				break;
			default:
				break;
		}
		//printf("  [%c] [%c]   [%s]   __%s", *val_start, *val_stop, t_csv_ste_nme[csv->ste], run );
		if ('\0' == *run)
			break;
		run++;
	}
	return;
}

/**--------------------------------------------------------------------------
 * Attempts to read a single line of CSV content
 * \param   L       Lua state.
 * \lparam  ud      t_csv userdata.
 * \lparam  string  Single row of CSV content.
 * \lreturn boolean Was it complete? False mean more lines are needed.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_csv_parseLine( lua_State *L )
{
	//S: csv lne tbl
	struct t_csv  *csv = t_csv_check_ud( L, 1, 1 );
	const char    *lne = luaL_checkstring( L, 2 );
	luaL_argcheck( L, (LUA_TTABLE == lua_type( L, 3 )), 3, "Expected a Table" );

	csv->fld  = (char *) lne;
	t_csv_parse( L, csv, lne );
	lua_pushboolean( L, T_CSV_RECDNE == csv->ste );
	lua_pushstring( L, csv->fld );
	//csv->ste  = (T_CSV_RECDNE == csv->ste) ? T_CSV_FLDBEG : csv->ste;
	csv->ste  = T_CSV_FLDBEG;
	return 2;
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
	size_t       its = luaL_checkinteger( L, lua_upvalueindex( 3 ) ); // iter input
	size_t       cnt = luaL_checkinteger( L, lua_upvalueindex( 4 ) ); // token counter
	size_t       itd = 0, tkn = its;                                  // iter delim, token start
	//printf("$$$$$$$$$$$$$$$$$$$      ITS(TKN): %zu   LEN: %zu <%c>\n", its, lns, str[tkn]);
	if (its > lns)
		lua_pushnil( L );
	else
	{
		for (; its < lns; its++)
		{
			//printf("   ITS: %zu <%c><%c> ", its, str[its], dlm[itd]);
			itd = (str[its] == dlm[itd])  // string matches delimiter
				? itd+1                       // advance delimiter
				: (str[its] == *dlm)          // first character in delimiter matches
					? 1                           // advance delimiter to one
					: 0;                          // reset delimiter
			//printf(" -- %s\n", lnd == itd ? "Yea" : "Nah");
			if (itd == lnd)               // delimeter completely matched
				break;
		}
		//printf("      TKN: %s   %zu %ld \n", &(str[tkn]), its, its-tkn - ( (its==lns) ? 0 : lnd - 1 ));
		lua_pushlstring( L, &(str[tkn]), its-tkn - ( (its==lns) ? 0 : lnd - 1 ) );
		lua_pushinteger( L, its+1 );
		lua_replace( L, lua_upvalueindex( 3 ) );    // update string iterator position
		lua_pushinteger( L, cnt+1 );
		lua_replace( L, lua_upvalueindex( 4 ) );    // update field count
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
	lua_pushinteger( L, 0 );                // str iterator
	lua_pushinteger( L, 0 );                // token counter
	lua_pushcclosure( L, &t_csv_split, 4 );
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
	lua_pushfstring( L, T_CSV_TYPE"[%c:%c:%c:%s]: %p",
		csv->dlm,
		csv->qte,
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
		lua_pushlstring( L, &(csv->dlm), 1 );
	else if (0 == strncmp( key, "quotchar", keyLen ))
		lua_pushlstring( L, &(csv->qte), 1 );
	else if (0 == strncmp( key, "escapechar", keyLen ))
		lua_pushlstring( L, &(csv->esc), 1 );
	else if (0 == strncmp( key, "doublequoted", keyLen ))
		lua_pushboolean( L, csv->dbl );
	else if (0 == strncmp( key, "state", keyLen ))
		lua_pushstring( L, t_csv_ste_nme[ csv->ste ] );
	else if (0 == strncmp( key, "field", keyLen ))
		lua_pushstring( L, csv->fld );
	else if (0 == strncmp( key, "headers", keyLen ))
		lua_getiuservalue( L, 1, T_CSV_HDRIDX );
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
	const char   *key     = luaL_checklstring( L, 2, &keyLen ); //S: csv idx val
	//t_stackDump(L);

	if (0 == strncmp( key, "delimiter", keyLen ))
		csv->dlm = lua_tostring(  L, 3 )[ 0 ];
	else if (0 == strncmp( key, "quotchar", keyLen ))
		csv->qte = lua_tostring(  L, 3 )[ 0 ];
	else if (0 == strncmp( key, "escapechar", keyLen ))
		csv->esc = lua_tostring(  L, 3 )[ 0 ];
	else if (0 == strncmp( key, "doublequoted", keyLen ))
		csv->dbl = lua_toboolean( L, 3 );
	else if (0 == strncmp( key, "state", keyLen ) )
		csv->ste = luaL_checkoption( L, 3, NULL, t_csv_ste_nme );
	else if (0 == strncmp( key, "headers", keyLen ) )
		lua_setiuservalue( L, 1, T_CSV_HDRIDX );
	else
		luaL_argerror( L, 2, "Can't set this value in "T_CSV_TYPE );
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
	  {"new"           , lt_csv_New        }
	, {"split"         , lt_csv_Split      }
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

