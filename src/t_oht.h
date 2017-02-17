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

// t_oht.c
// Constructors
int   luaopen_t_oht  ( lua_State *L );
int   t_oht_check    ( lua_State *L, int pos, int check );
int   t_oht_create   ( lua_State *L );

void  t_oht_getElement( lua_State *L, int pos );
void  t_oht_addElement( lua_State *L, int pos );
void  t_oht_deleteElement( lua_State *L, int pos );
void  t_oht_insertElement( lua_State *L, int pos );
void  t_oht_readArguments( lua_State *L, int sp, int ep );
void  t_oht_getTable( lua_State *L, int type );
