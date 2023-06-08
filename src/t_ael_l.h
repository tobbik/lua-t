/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael.h
 * \brief     OOP wrapper for an asyncronous eventloop (T.Loop)
 *            data types and global functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *
 */

// includes the Lua headers
#include "t_ael.h"
#include "t.h"                 // t_typ*

#include <sys/time.h>          // struct timespec

enum t_ael_msk {
	// 00000000
	T_AEL_NO = 0x00,               ///< not set
	// 00000001
	T_AEL_RD = 0x01,               ///< Read  ready event on handle
	// 00000010
	T_AEL_WR = 0x02,               ///< Write ready event on handle
	// 00000011
	T_AEL_RW = 0x03,               ///< Read and Write on handle
};

extern const char* t_ael_msk_lst[ ]; // defined in t_ael_l.c

#define T_AEL_SLOTSIZE   1024 // how many events can be returned for ONE call to epoll_wait()
                              // NOT how many fd can be observed, which is limited by ulimit!

// definition for file/socket descriptor node
// It keeps a reference to the handle to make sure it won't be garbage collected
// if the calling code looses its reference.  So if it gets called by the loop
// it won't error out.
#define T_AEL_DSC_FRDIDX  1   ///< FUNCTION/ARGUMENTS READ INDEX
#define T_AEL_DSC_FWRIDX  2   ///< FUNCTION/ARGUMENTS WRITE INDEX
#define T_AEL_DSC_HDLIDX  3   ///< HANDLE INDEX
struct t_ael_dnd {
	enum t_ael_msk    msk;         ///< mask, for unset, readable, writable
};

// definition for timed task
// The t_ael_tsk collection is implemented as a "linked list" where the "next"
// task is a uservalue of the "previous" task.  In Lua 5.4 userdata can have
// multiple uservalues.  We use index1 as uservalue of the "next" task and
// index2 as uservalue for the function-table.  This has the benefit that if
// tasks go out of scope all references get automatically garbage collected.
#define T_AEL_TSK_NXTIDX  1   ///< NEXT TASK INDEX
#define T_AEL_TSK_FNCIDX  2   ///< FUNCTION/ARGUMENTS TABLE INDEX
struct t_ael_tsk {
	unsigned long long   tout;    ///< timeout in nanoseconds until execution
};

// t_ael general implementation; API specifics live behind the *state pointer
#define T_AEL_STEIDX      1   ///< INDEX FOR PLATFORM SPECIFIC STATE
#define T_AEL_DSCIDX      2   ///< INDEX FOR DESCRIPTOR TABLE
#define T_AEL_TSKIDX      3   ///< INDEX FOR TASK TIMER LINKED LIST HEAD
#define T_AEL_NOTIMEOUT   0   ///< TIMEOUT if no timer is in the list
struct t_ael {
	int                run;       ///< boolean indicator to start/stop the loop
	int                fdCount;   ///< how many descriptor observed
	// for each call of poll it is necessary to reset the next time out
	// it is expensive to get the linked head, extract the time and pop it
	// instead, keep track of the heads (aka. earliest timer) timeout value.
	// Set to T_AEL_NOTIMEOUT if no timers are in the list at all.
	unsigned long long   tout;    ///< timeout in nanoseconds of taskHead
};

// t_ael_l.c
struct t_ael     *t_ael_check_ud   ( lua_State *L, int pos, int check );
struct t_ael     *t_ael_create_ud  ( lua_State *L );
void              t_ael_doFunction ( lua_State *L, int exc );

// t_ael_dnd.c
struct t_ael_dnd *t_ael_dnd_create_ud( lua_State *L );
struct t_ael_dnd *t_ael_dnd_check_ud ( lua_State *L, int pos, int check );
void              t_ael_dnd_execute  ( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk );
int               luaopen_t_ael_dnd  ( lua_State *L );

// t_ael_tsk.c
struct t_ael_tsk *t_ael_tsk_create_ud( lua_State *L, unsigned long long ms );
struct t_ael_tsk *t_ael_tsk_check_ud ( lua_State *L, int pos, int check );
void              t_ael_tsk_insert   ( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tIns );
void              t_ael_tsk_remove   ( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tCnd );
void              t_ael_tsk_process  ( lua_State *L, struct t_ael *ael, unsigned long long et );
int               luaopen_t_ael_tsk  ( lua_State *L );

// t_ael_hlp.c
int t_ael_hlp_cloexec( int fd );

// p_ael_(impl).c   (Implementation specific functions) INTERFACE
void p_ael_create_ud_impl   ( lua_State *L );
void p_ael_free_impl        ( lua_State *L, int aelpos );
// TODO: dnd contains the existing mask and is passed only to calculate a
//       resulting mask if the implementation needs it. Example: a descriptor
//       has both Read and Write set but we are removing READ, in epoll we have
//       to re-set Write observation.  Can this be implemented with less
//       parameters??
int  p_ael_addhandle_impl   ( lua_State *L, int aelpos, struct t_ael_dnd *dnd, int fd, enum t_ael_msk msk );
int  p_ael_removehandle_impl( lua_State *L, int aelpos, struct t_ael_dnd *dnd, int fd, enum t_ael_msk msk );
int  p_ael_poll_impl        ( lua_State *L, struct timeval *timeout, int aelpos );


int  clock_gettime_realtime ( struct timespec *ts );
int  clock_gettime_monotonic( struct timespec *ts );


#define T_AEL_TIMESUB( a, b, result )                   \
	do {                                                 \
	  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;      \
	  (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;   \
	  if ((result)->tv_nsec < 0) {                       \
	    --(result)->tv_sec;                              \
	    (result)->tv_nsec += 1000000000;                 \
	  }                                                  \
	} while (0)

#define T_AEL_TIMEVAL2MS(tv)                            \
	((tv) ? ( ( (tv)->tv_sec * 1000000 )  +  ( (tv)->tv_nsec / 1000000 ) ) : 0)
