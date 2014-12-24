/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_elp.h
 * \brief     OOP wrapper for an asyncronous eventloop (T.Loop)
 *            data types and global functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_sck.h"

enum t_elp_t {
	t_elp_READ,               ///< Reader event on socket
	t_elp_WRIT,               ///< Reader event on socket
};


struct t_elp_fd {
	enum t_elp_t       t;
	int                fd;    ///< descriptor
	int                fR;    ///< func/arg table reference in LUA_REGISTRYINDEX
	int                hR;    ///< handle   reference in LUA_REGISTRYINDEX (T.Socket or Lua file)
};


struct t_elp_tm {
	int                fR;    ///< func/arg table reference in LUA_REGISTRYINDEX
	int                tR;    ///< T.Time  reference in LUA_REGISTRYINDEX
	struct timeval    *tv;    ///< time to elapse until fire (timval to work with)
	struct t_elp_tm   *nxt;   ///< next pointer for linked list
};


/// t_elp implementation for select based loops
struct t_elp {
	fd_set             rfds;
	fd_set             wfds;
	fd_set             rfds_w;   ///<
	fd_set             wfds_w;   ///<
	int                run;      ///< boolean indicator to start/stop the loop
	int                mxfd;     ///< max fd
	size_t             fd_sz;    ///< how many fd to handle
	struct t_elp_tm   *tm_head;
	struct t_elp_fd  **fd_set;   ///< array with pointers to fd_events indexed by fd
};


// t_elp.c
struct t_elp *t_elp_check_ud ( lua_State *luaVM, int pos, int check );
struct t_elp *t_elp_create_ud( lua_State *luaVM, size_t sz );
int   lt_elp_addhandle       ( lua_State *luaVM );
int   lt_elp_removehandle    ( lua_State *luaVM );
int   lt_elp_showloop        ( lua_State *luaVM );

void t_elp_executetimer     ( lua_State *luaVM, struct t_elp *elp, struct timeval *rt );
void t_elp_executehandle    ( lua_State *luaVM, struct t_elp *elp, int fd );


// t_elp_(impl).c   (Implementation specific functions) INTERFACE
void t_elp_create_ud_impl   ( struct t_elp *elp );
void t_elp_addhandle_impl   ( struct t_elp *elp, int fd, int read );
void t_elp_removehandle_impl( struct t_elp *elp, int fd );
void t_elp_addtimer_impl    ( struct t_elp *elp, struct timeval *tv );
int  t_elp_poll_impl        ( lua_State *luaVM, struct t_elp *elp );



