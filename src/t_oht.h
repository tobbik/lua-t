/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_oht.h
 * \brief     data types for an ordered hash table
 *            Elements can be accessed by their name and their index.  Basically
 *            an ordered hashmap.  It is implemented as intelligent mapper around
 *            a Lua table.  The basic design handles the values as following:
 *            1   = 'a',
 *            2   = 'b',
 *            3   = 'c',
 *            4   = 'd',
 *            "a" = "value 1",
 *            "b" = "value 2",
 *            "c" = "value 3",
 *            "d" = "value 4",
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/// The userdata struct for T.OrderedHashTable
struct t_oht {
	int            tR;    ///<  LUA_REGISTRYINDEX reference for the table
};


// t_oht.c
// Constructors
int             luaopen_t_oht  ( lua_State *L );
struct t_oht   *t_oht_check_ud ( lua_State *L, int pos, int check );
struct t_oht   *t_oht_create_ud( lua_State *L );

