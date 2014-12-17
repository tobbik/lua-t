/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp.h
 * \brief     data types for HTTP related data types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_elp.h"

/// The userdata struct for T.Http.Server
struct t_htpsrv {
	struct t_sck *sck;    ///< t_sck socket (must be tcp)
	int           sR;     ///< Lua registry reference for t.Socket instance
	int           lR;     ///< Lua registry reference for t.Loop instance
	int           rR;     ///< Lua registry reference to request handler
};


/// The userdata struct for T.Http.Connection ( Server:accept() )
struct t_htpcon {
	int           fd;     ///< the socket for direct access
	int           sR;     ///< Lua registry reference for t.Socket instance
	int           aR;     ///< Lua registry reference for t.Ip     instance (struct sockaddr_in)
	int           rR;     ///< Lua registry reference to request handler
};

struct t_htpreq {
	struct t_htpsrv   *srv;  ///< reference to server
	
};


// t_htp.c
// Constructors
struct t_htpsrv   *t_htpsrv_check_ud ( lua_State *luaVM, int pos, int check );
struct t_htpsrv   *t_htpsrv_create_ud( lua_State *luaVM );


// __        __   _    ____             _        _
// \ \      / /__| |__/ ___|  ___   ___| | _____| |_
//  \ \ /\ / / _ \ '_ \___ \ / _ \ / __| |/ / _ \ __|
//   \ V  V /  __/ |_) |__) | (_) | (__|   <  __/ |_
//    \_/\_/ \___|_.__/____/ \___/ \___|_|\_\___|\__|

/// parser helper for websocket header
//  http://www.johnmccutchan.com/2012/01/writing-your-own-websocket-server.html
struct t_wsk_msg_h {
	union {
		struct {
			unsigned int OP_CODE : 4;
			unsigned int RSV1    : 1;
			unsigned int RSV2    : 1;
			unsigned int RSV3    : 1;
			unsigned int FIN     : 1;
			unsigned int PAYLOAD : 7;
			unsigned int MASK    : 1;
		} bits;
		uint16_t short_header;
	};
};


/// data type tor websocket handling
struct t_wsk {
	int      sR;       /// Lua registry Reference for t_sck userdata
	int      spR;      /// Lua registry Reference for subprotocol string
	int      fd;       /// copy fd from t_sck for direct access
	struct t_sck *sck; /// reference to t_sck type
};


struct t_wsk  *t_wsk_create_ud( lua_State *luaVM );
struct t_wsk  *t_wsk_check_ud ( lua_State *luaVM, int pos, int check );
