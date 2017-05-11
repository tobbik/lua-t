/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tim.h
 * \brief     data types for timeval (T.Time) related data types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_tim.h"

// Constructors
// t_tim.c
struct timeval *t_tim_create_ud( lua_State *L, long ms );

