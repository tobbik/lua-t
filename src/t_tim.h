/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tim_cmn.h
 * \brief     t_tim_* types and unctions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef _WIN32
#include <time.h>
#else
#define __USE_MISC
#include <sys/time.h>    // gettimeofday()
#endif

#define T_TIM_IDNT "tim"
#define T_TIM_NAME "Time"
#define T_TIM_TYPE "T."T_TIM_NAME

// Constructors
// t_tim.c
struct timeval *t_tim_create_ud( lua_State *L, struct timeval *tvr );
struct timeval *t_tim_check_ud ( lua_State *L, int pos, int check );

void t_tim_since( struct timeval *tA );

// these might be needed if there is no reasonable sys/time.h implementation
#ifndef timercmp
#define timercmp( a, b, CMP )                      \
	(((a)->tv_sec  ==  (b)->tv_sec)   ?                  \
	 ((a)->tv_usec CMP (b)->tv_usec)  :                  \
	 ((a)->tv_sec  CMP (b)->tv_sec))
#endif

#ifndef timeradd
#define timeradd(a, b, result)                     \
	do {                                                 \
	  (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;      \
	  (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;   \
	  if ((result)->tv_usec >= 1000000) {                \
	    ++(result)->tv_sec;                              \
	    (result)->tv_usec -= 1000000;                    \
	  }                                                  \
	} while (0)
#endif

#ifndef timersub
#define timersub(a, b, result)                     \
	do {                                                 \
	  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;      \
	  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;   \
	  if ((result)->tv_usec < 0) {                       \
	    --(result)->tv_sec;                              \
	    (result)->tv_usec += 1000000;                    \
	  }                                                  \
	} while (0)
#endif
