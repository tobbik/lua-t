// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/**
 * \file      t_nry.h
 * \brief     models an Numarray
 *            This an example file as explained in the docs/Coding.rst
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/// The userdata struct for T.Numarray
struct t_nry
{
	size_t         len;
	lua_Integer    v[ 1 ];  ///< variable part, must be last in struct
};

// Constructors
int           luaopen_t_nry  ( lua_State *L );
struct t_nry *t_nry_check_ud ( lua_State *L, int pos, int check );
struct t_nry *t_nry_create_ud( lua_State *L, size_t n );
