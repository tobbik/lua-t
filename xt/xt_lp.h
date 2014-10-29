/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_lp.h
 * \brief     OOP wrapper for an asyncronous eventloop (xt.Loop)
 *            data types and global functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

#include "xt_sck.h"


enum xt_lp_t {
	XT_LP_READ,               ///< Reader event on socket
	XT_LP_WRIT,               ///< Reader event on socket
	XT_LP_TIME,
};


struct xt_lp_fd {
	enum xt_lp_t        t;
	int                 fd;    ///< descriptor
	int                 fR;    ///< function reference in LUA_REGISTRYINDEX
};


struct xt_lp_tm {
	enum xt_lp_t        t;
	//int                 id;
	struct timeval      tw;    ///< time to elapse until fire (timval to work with)
	struct timeval     *to;    ///< keep reference to the original timeval
	int                 fR;    ///< function reference in LUA_REGISTRYINDEX
	struct xt_lp_tm    *nxt;   ///< next pointer for linked list
};

//#ifdef XT_LOOP_SELECT
/// xt_lp implementation for select based loops
struct xt_lp {
	fd_set            rfds;
	fd_set            wfds;
	fd_set            rfds_w;   ///<
	fd_set            wfds_w;   ///<
	int               run;      ///< boolean indicator to start/stop the loop
	int               mxfd;     ///< max fd
	size_t            fd_sz;    ///< how many fd to handle
	struct xt_lp_tm  *tm_head;
	struct xt_lp_fd **fd_set;   ///< array with pointers to fd_events indexed by fd
};
//#endif


// xt_lp.c
struct xt_lp *xt_lp_check_ud ( lua_State *luaVM, int pos );
struct xt_lp *xt_lp_create_ud( lua_State *luaVM, size_t sz );
int           lxt_lp_New     ( lua_State *luaVM );



