/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael.h
 * \brief     OOP wrapper for an asyncronous eventloop (T.Loop)
 *            data types and global functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_sck.h"

enum t_ael_t {
	// 00000000
	T_AEL_NO = 0x00,        ///< not set
	// 00000001
	T_AEL_RD = 0x01,        ///< Read  ready event on handle
	// 00000010
	T_AEL_WR = 0x02,        ///< Write ready event on handle
	// 00000011
	T_AEL_RW = 0x03,        ///< Read and Write on handle
};


struct t_ael_fd {
	enum t_ael_t       t;     ///< mask, for unset, readable, writable
	int                fd;    ///< descriptor
	int                rR;    ///< func/arg table reference for read  event in LUA_REGISTRYINDEX
	int                wR;    ///< func/arg table reference for write event in LUA_REGISTRYINDEX
	int                hR;    ///< handle   reference in LUA_REGISTRYINDEX (T.Socket or Lua file)
};


struct t_ael_tm {
	int                fR;    ///< func/arg table reference in LUA_REGISTRYINDEX
	int                tR;    ///< T.Time  reference in LUA_REGISTRYINDEX
	struct timeval    *tv;    ///< time to elapse until fire (timval to work with)
	struct t_ael_tm   *nxt;   ///< next pointer for linked list
};


/// t_ael implementation for select based loops
struct t_ael {
	fd_set             rfds;
	fd_set             wfds;
	fd_set             rfds_w;   ///<
	fd_set             wfds_w;   ///<
	int                run;      ///< boolean indicator to start/stop the loop
	int                max_fd;   ///< max fd
	size_t             fd_sz;    ///< how many fd to handle
	struct t_ael_tm   *tm_head;
	struct t_ael_fd  **fd_set;   ///< array with pointers to fd_events indexed by fd
};


// t_ael.c
struct t_ael *t_ael_check_ud ( lua_State *luaVM, int pos, int check );
struct t_ael *t_ael_create_ud( lua_State *luaVM, size_t sz );
int   lt_ael_addhandle       ( lua_State *luaVM );
int   lt_ael_removehandle    ( lua_State *luaVM );
int   lt_ael_showloop        ( lua_State *luaVM );

void t_ael_executetimer     ( lua_State *luaVM, struct t_ael *ael, struct timeval *rt );
void t_ael_executehandle    ( lua_State *luaVM, struct t_ael *ael, int fd, enum t_ael_t t );


// t_ael_(impl).c   (Implementation specific functions) INTERFACE
void t_ael_create_ud_impl   ( struct t_ael *ael );
void t_ael_addhandle_impl   ( struct t_ael *ael, int fd, enum t_ael_t t );
void t_ael_removehandle_impl( struct t_ael *ael, int fd, enum t_ael_t t );
void t_ael_addtimer_impl    ( struct t_ael *ael, struct timeval *tv );
int  t_ael_poll_impl        ( lua_State *luaVM, struct t_ael *ael );


