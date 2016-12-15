/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_oht.h
 * \brief     data types for Sets.
 *            Elements of a Set are unordered (like a bag).  Each element can
 *            exist only once.  Implemented as a mapper around a Lua table.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/// The userdata struct for T.Set
struct t_set {
	size_t         len;   ///< How many elements in the set.  Using the keys for
	                      ///  speedy operation #table is meaning less.  Keeping
	                      ///  track manually.
	int            tR;    ///< LUA_REGISTRYINDEX reference for the table
};


// t_set.c
// Constructors
int             luaopen_t_set  ( lua_State *L );
struct t_set   *t_set_check_ud ( lua_State *L, int pos, int check );
struct t_set   *t_set_create_ud( lua_State *L, int pos, int mode );

