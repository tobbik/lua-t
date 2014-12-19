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
struct t_htp_srv {
	struct t_sck *sck;    ///< t_sck socket (must be tcp)
	int           sR;     ///< Lua registry reference for t.Socket instance
	int           aR;     ///< Lua registry reference for t.Ip     instance (struct sockaddr_in)
	int           lR;     ///< Lua registry reference for t.Loop instance
	int           rR;     ///< Lua registry reference to request handler
};


/// The userdata struct for T.Http.Message ( Server:accept() )
struct t_htp_msg {
	int           fd;     ///< the socket for direct access
	int           sR;     ///< Lua registry reference for t.Socket instance
	int           aR;     ///< Lua registry reference for t.Ip     instance (struct sockaddr_in)
	int           rR;     ///< Lua registry reference to request handler
	int           hR;     ///< Lua registry reference to header table
	int           status; ///< HTTP Status Code
	int           sz;     ///< HTTP Message Size
};


struct t_htp_req {
	struct t_htp_srv   *srv;  ///< reference to server
	
};


// t_htp.c
// Constructors
struct t_htp_srv   *t_htp_srv_check_ud ( lua_State *luaVM, int pos, int check );
struct t_htp_srv   *t_htp_srv_create_ud( lua_State *luaVM );



// library exporters
LUAMOD_API int luaopen_t_htp_msg( lua_State *luaVM );
LUAMOD_API int luaopen_t_htp_srv( lua_State *luaVM );


// Message specific methods
int lt_htp_msg_read ( lua_State *luaVM );
int lt_htp_msg_write( lua_State *luaVM );


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
