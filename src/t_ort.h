/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ort.h
 * \brief     data types for an ordered table
 *            Elements can be accessed by their name and their index. Basically
 *            an ordered hashmap.  It is implemented as intelligent mapper around
 *            a Lua table.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/// The userdata struct for T.OrderedTable
struct t_ort {
	int            tR;    ///<  LUA_REGISTRYINDEX reference for the table
};


// t_ort.c
// Constructors
int             luaopen_t_ort ( lua_State *L );
struct t_ort   *t_ort_check_ud ( lua_State *L, int pos, int check );
struct t_ort   *t_ort_create_ud( lua_State *L );

