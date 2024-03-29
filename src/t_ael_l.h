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
#include "t.h"                 // t_typ*

#include <sys/time.h>          // struct timeval

enum t_ael_msk {
	// 00000000
	T_AEL_NO = 0x00,            ///< not set
	// 00000001
	T_AEL_RD = 0x01,            ///< Read  ready event on handle
	// 00000010
	T_AEL_WR = 0x02,            ///< Write ready event on handle
	// 00000011
	T_AEL_RW = 0x03,            ///< Read and Write on handle
};

// definition for file/socket descriptor node
// It keeps a reference to the handle to make sure it won't be garbage collected
// if the calling code looses its reference.  So if it gets called by the loop
// it won't error out.
#define T_AEL_DSC_FRDIDX   1   ///< FUNCTION/ARGUMENTS READ INDEX
#define T_AEL_DSC_FWRIDX   2   ///< FUNCTION/ARGUMENTS WRITE INDEX
#define T_AEL_DSC_HDLIDX   3   ///< HANDLE INDEX
struct t_ael_dnd {
	enum t_ael_msk    msk;   ///< mask, for unset, readable, writable
};

// definition for timed task
// The t_ael_tsk collection is implemented as a "linked list" where the "next"
// task is a uservalue of the "previous" task.  In Lua 5.4 userdata can have
// multiple uservalues.  We use index1 as uservalue of the "next" task and
// index2 as uservalue for the function-table.  This has the benefit that if
// tasks go out of scope all references get automatically garbage collected.
#define T_AEL_TSK_NXTIDX   1   ///< NEXT TASK INDEX
#define T_AEL_TSK_FNCIDX   2   ///< FUNCTION/ARGUMENTS TABLE INDEX
struct t_ael_tsk {
	//int                fR;      ///< func/arg table reference in LUA_REGISTRYINDEX
	lua_Integer        tout;    ///< timeout in ms
};

// t_ael general implementation; API specifics live behind the *state pointer
#define T_AEL_STEIDX   1       ///< PLATFORM SPECIFIC STATE INDEX
#define T_AEL_DSCIDX   2       ///< DESCRIPTOR TABLE INDEX
#define T_AEL_TSKIDX   3       ///< TASK LINKED LIST HEAD INDEX
#define T_AEL_NOTIMEOUT   -1   ///< IF NO TIMER IS IN LIST
struct t_ael {
	int                run;      ///< boolean indicator to start/stop the loop
	int                fdCount;  ///< how many descriptor observed
	// for each call of poll it is necessary to reset the next time out
	// it is expensive to get the linked head, extract the time and pop it
	// keep a reference to the heads timeout value
	lua_Integer        tout;     ///< timeout of taskHead
};

// t_ael_l.c
struct t_ael     *t_ael_check_ud   ( lua_State *L, int pos, int check );
struct t_ael     *t_ael_create_ud  ( lua_State *L );
void              t_ael_doFunction( lua_State *L, int exc );

// t_ael_dnd.c
struct t_ael_dnd *t_ael_dnd_create_ud( lua_State *L );
struct t_ael_dnd *t_ael_dnd_check_ud ( lua_State *L, int pos, int check );
void              t_ael_dnd_execute( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk );
int               luaopen_t_ael_dnd  ( lua_State *L );

// t_ael_tsk.c
struct t_ael_tsk *t_ael_tsk_create_ud( lua_State *L, lua_Integer ms );
struct t_ael_tsk *t_ael_tsk_check_ud( lua_State *L, int pos, int check );
void              t_ael_tsk_insert( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tIns );
void              t_ael_tsk_remove( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tCnd );
void              t_ael_tsk_process( lua_State *L, struct t_ael *ael, lua_Integer et );
int               luaopen_t_ael_tsk  ( lua_State *L );

// p_ael_(impl).c   (Implementation specific functions) INTERFACE
void p_ael_create_ud_impl   ( lua_State *L );
void p_ael_free_impl        ( lua_State *L, int aelpos );
int  p_ael_addhandle_impl   ( lua_State *L, int aelpos, struct t_ael_dnd *dnd, int fd, enum t_ael_msk msk );
int  p_ael_removehandle_impl( lua_State *L, int aelpos, struct t_ael_dnd *dnd, int fd, enum t_ael_msk msk );
int  p_ael_poll_impl        ( lua_State *L, int timeout, int aelpos );

