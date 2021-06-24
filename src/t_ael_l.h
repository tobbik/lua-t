/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael.h
 * \brief     OOP wrapper for an asyncronous eventloop (T.Loop)
 *            data types and global functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *
[arch@tk-analytics src]$ ll *ael*
-rwxr-xr-x 1 arch users 57640 Oct 23 17:06 ael.so
-rw-r--r-- 1 arch users  6730 Oct 23 15:36 p_ael_epl.c
-rw-r--r-- 1 arch users 13952 Oct 23 17:03 p_ael_epl.o
-rw-r--r-- 1 arch users  5018 Oct 23 15:35 p_ael_sel.c
-rw-r--r-- 1 arch users 10736 Oct 23 11:41 p_ael_sel.o
-rw-r--r-- 1 arch users   365 Oct 11 13:19 t_ael.h
-rw-r--r-- 1 arch users 32560 Sep 11 14:22 t_ael.o
-rw-r--r-- 1 arch users 25977 Oct 23 17:06 t_ael_l.c
-rw-r--r-- 1 arch users  3877 Oct 23 16:52 t_ael_l.h
-rw-r--r-- 1 arch users 35088 Oct 23 17:06 t_ael_l.o
 */

// includes the Lua headers
#include "t_ael.h"
#include "t.h"             // t_typ*

enum t_ael_msk {
	// 00000000
	T_AEL_NO = 0x00,        ///< not set
	// 00000001
	T_AEL_RD = 0x01,        ///< Read  ready event on handle
	// 00000010
	T_AEL_WR = 0x02,        ///< Write ready event on handle
	// 00000011
	T_AEL_RW = 0x03,        ///< Read and Write on handle
};

// definition for file/socket descriptor node
struct t_ael_dnd {
	enum t_ael_msk    msk;   ///< mask, for unset, readable, writable
	int               rR;    ///< func/arg table reference for read  event in LUA_REGISTRYINDEX
	int               wR;    ///< func/arg table reference for write event in LUA_REGISTRYINDEX
	int               hR;    ///< handle   LUA_REGISTRYINDEX reference (T.Net.* or Lua file handle)
};

// definition for timer node
struct t_ael_tnd {
	int                fR;    ///< func/arg table reference in LUA_REGISTRYINDEX
	int                tR;    ///< T.Time  reference in LUA_REGISTRYINDEX
	struct timeval    *tv;    ///< time to elapse until fire (timval to work with)
	struct t_ael_tnd  *nxt;   ///< next pointer for linked list
};

// t_ael general implementation; API specifics live behind the *state pointer
struct t_ael {
	int                run;      ///< boolean indicator to start/stop the loop
	int                fdCount;  ///< how many descriptor observed
	//void              *state;    ///< polling API specific data
	int                sR;       ///< reference to polling API specific data
	int                dR;       ///< descriptor table reference
	struct t_ael_tnd  *tmHead;   ///< Head of timers linked list
};

// t_ael_l.c
struct t_ael     *t_ael_check_ud     ( lua_State *L, int pos, int check );
struct t_ael     *t_ael_create_ud    ( lua_State *L );
void              t_ael_executehandle( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk );
// t_ael_dnd.c
struct t_ael_dnd *t_ael_dnd_create_ud( lua_State *L );
struct t_ael_dnd *t_ael_dnd_check_ud ( lua_State *L, int pos, int check );
void              t_ael_dnd_setMaskAndFunction( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk, int fR );
void              t_ael_dnd_removeMaskAndFunction( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk );
int               luaopen_t_ael_dnd  ( lua_State *L );

// p_ael_(impl).c   (Implementation specific functions) INTERFACE
int  p_ael_create_ud_impl   ( lua_State *L );
int  p_ael_resize_impl      ( lua_State *L, struct t_ael *ael, size_t old_sz );
void p_ael_free_impl        ( lua_State *L, int ref );
int  p_ael_addhandle_impl   ( lua_State *L, struct t_ael *ael, struct t_ael_dnd *dnd, int fd, enum t_ael_msk msk );
int  p_ael_removehandle_impl( lua_State *L, struct t_ael *ael, struct t_ael_dnd *dnd, int fd, enum t_ael_msk msk );
void p_ael_addtimer_impl    ( struct t_ael *ael, struct timeval *tv );
int  p_ael_poll_impl        ( lua_State *L, struct t_ael *ael );

