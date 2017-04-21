/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael.h
 * \brief     OOP wrapper for an asyncronous eventloop (T.Loop)
 *            data types and global functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#define T_AEL_IDNT "ael"
#define T_AEL_NAME "Loop"
#define T_AEL_TYPE "T."T_AEL_NAME

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
struct t_ael_fd {
	enum t_ael_msk     msk;   ///< mask, for unset, readable, writable
	int                fd;    ///< descriptor
	int                rR;    ///< func/arg table reference for read  event in LUA_REGISTRYINDEX
	int                wR;    ///< func/arg table reference for write event in LUA_REGISTRYINDEX
	int                hR;    ///< handle   reference in LUA_REGISTRYINDEX (T.Net.* or Lua file handle)
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
	int                fdMax;    ///< max fd
	size_t             fdCount;  ///< how many fd to handle
	void              *state;    ///< polling API specific data
	struct t_ael_tnd  *tmHead;   ///< Head of timers linked list
	struct t_ael_fd  **fdSet;    ///< array with pointers to fd_events indexed by fd
};


// t_ael.c
struct t_ael *t_ael_check_ud ( lua_State *L, int pos, int check );
struct t_ael *t_ael_create_ud( lua_State *L, size_t sz );
int   lt_ael_addhandle       ( lua_State *L );
int   lt_ael_removehandle    ( lua_State *L );
int   lt_ael_showloop        ( lua_State *L );

void t_ael_executeHeadTimer ( lua_State *L, struct t_ael_tnd **tHead, struct timeval *rt );
void t_ael_executehandle    ( lua_State *L, struct t_ael_fd *fd, enum t_ael_msk msk );

// t_ael_(impl).c   (Implementation specific functions) INTERFACE
int  t_ael_create_ud_impl   ( lua_State *L, struct t_ael *ael );
void t_ael_free_impl        ( struct t_ael *ael );
int  t_ael_addhandle_impl   ( lua_State *L, struct t_ael *ael, int fd, enum t_ael_msk msk );
int  t_ael_removehandle_impl( lua_State *L, struct t_ael *ael, int fd, enum t_ael_msk msk );
void t_ael_addtimer_impl    ( struct t_ael *ael, struct timeval *tv );
int  t_ael_poll_impl        ( lua_State *L, struct t_ael *ael );


static const struct t_typ t_ael_directionList[ ] = {
	{ "T_AEL_RD"           , T_AEL_RD        },
	{ "read"               , T_AEL_RD        },
	{ "rd"                 , T_AEL_RD        },
	{ "r"                  , T_AEL_RD        },
	{ "inc"                , T_AEL_RD        },
	{ "incoming"           , T_AEL_RD        },

	{ "T_AEL_WR"           , T_AEL_WR        },
	{ "write"              , T_AEL_WR        },
	{ "wr"                 , T_AEL_WR        },
	{ "w"                  , T_AEL_WR        },
	{ "out"                , T_AEL_WR        },
	{ "outgoing"           , T_AEL_WR        },

	{ "T_AEL_RW"           , T_AEL_RW        },
	{ "readwrite"          , T_AEL_RW        },
	{ "rdwr"               , T_AEL_RW        },
	{ "rw"                 , T_AEL_RW        },
	{ "both"               , T_AEL_RW        },
	{ "either"             , T_AEL_RW        },

	{ NULL                , 0                }
};
