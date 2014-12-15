/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_wsk.h
 * \brief     data types and global functions for WebSockets
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_elp.h"

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
