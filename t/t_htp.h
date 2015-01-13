/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp.h
 * \brief     data types for HTTP related data types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_ael.h"

/// The userdata struct for T.Http.Server
struct t_htp_srv {
	struct t_sck *sck;    ///< t_sck socket (must be tcp)
	int           cR;     ///< Lua registry reference for connection table
	int           sR;     ///< Lua registry reference for t.Socket instance
	int           aR;     ///< Lua registry reference for t.Ip     instance (struct sockaddr_in)
	int           lR;     ///< Lua registry reference for t.Loop instance
	int           rR;     ///< Lua registry reference to request handler
	time_t        nw;     ///< Current time on the server
	char    fnow[30];     ///< Formatted Date time in HTTP format
};



// _   _ _____ _____ ____
//| | | |_   _|_   _|  _ \   _ __   __ _ _ __ ___  ___ _ __
//| |_| | | |   | | | |_) | | '_ \ / _` | '__/ __|/ _ \ '__|
//|  _  | | |   | | |  __/  | |_) | (_| | |  \__ \  __/ |
//|_| |_| |_|   |_| |_|     | .__/ \__,_|_|  |___/\___|_|
//                          |_|

/// Recognized HTTP Methods
enum t_htp_mth {
	T_HTP_MTH_CONNECT,
	T_HTP_MTH_CHECKOUT,
	T_HTP_MTH_COPY,
	T_HTP_MTH_DELETE,
	T_HTP_MTH_GET,
	T_HTP_MTH_HEAD,
	T_HTP_MTH_LOCK,
	T_HTP_MTH_MKACTIVITY,
	T_HTP_MTH_MKCALENDAR,
	T_HTP_MTH_MKCOL,
	T_HTP_MTH_MERGE,
	T_HTP_MTH_MSEARCH,
	T_HTP_MTH_MOVE,
	T_HTP_MTH_NOTIFY,
	T_HTP_MTH_OPTIONS,
	T_HTP_MTH_POST,
	T_HTP_MTH_PUT,
	T_HTP_MTH_PATCH,
	T_HTP_MTH_PURGE,
	T_HTP_MTH_PROPFIND,
	T_HTP_MTH_PROPPATCH,
	T_HTP_MTH_REPORT,
	T_HTP_MTH_SEARCH,
	T_HTP_MTH_SUBSCRIBE,
	T_HTP_MTH_TRACE,
	T_HTP_MTH_UNLOCK,
	T_HTP_MTH_UNSUBSCRIBE,
};


/// State of the HTTP message
enum t_htp_sta {
	T_HTP_STA_ZERO,         ///< Nothing done yet
	T_HTP_STA_URL,          ///< Parsing Url
	T_HTP_STA_VERSION,      ///< Parsing HTTP version
	T_HTP_STA_HEADER,       ///< Parsing Headers
	T_HTP_STA_BODY,         ///< Parsing Body
	T_HTP_STA_NOBODY,       ///< Header done, No body expected
	T_HTP_STA_BUFFER,       ///< Fill buffer with data
	T_HTP_STA_SEND,         ///< Send data from buffer
	T_HTP_STA_DONE,         ///< Finished
};

// Available HTTP versions
enum t_htp_ver {
	T_HTP_VER_09,
	T_HTP_VER_10,
	T_HTP_VER_11
};


/// The userdata struct for T.Http.Message ( Server:accept() )
struct t_htp_msg {
	int             pR;     ///< Lua registry reference for proxy table
	struct t_sck   *sck;    ///< pointer to the actual socket
	struct t_htp_srv *srv;    ///< pointer to the HTTP-Server

	int             status; ///< HTTP Status Code
	int             sz;     ///< HTTP Message Size
	int             kpAlv;  ///< keepalive value in seconds -> 0==no Keepalive
	enum t_htp_sta  pS;     ///< HTTP parser state
	enum t_htp_mth  mth;    ///< HTTP Method
	enum t_htp_ver  ver;    ///< HTTP version
	size_t          bRead;  ///< How many byte processed
	size_t          sent;   ///< How many byte sent out
	char           *bufr;   ///< Pointer to buffers current position
	char            buf[ BUFSIZ ];   ///< Initial Buffer
};


struct t_htp_req {
	struct t_htp_srv   *srv;  ///< reference to server
	
};


// t_htp.c
// Constructors
struct t_htp_srv   *t_htp_srv_check_ud ( lua_State *luaVM, int pos, int check );
struct t_htp_srv   *t_htp_srv_create_ud( lua_State *luaVM );
void                t_htp_srv_setnow( struct t_htp_srv *s, int force );



// library exporters
LUAMOD_API int luaopen_t_htp_msg( lua_State *luaVM );
LUAMOD_API int luaopen_t_htp_srv( lua_State *luaVM );


// Constructors
struct t_htp_msg   *t_htp_msg_check_ud ( lua_State *luaVM, int pos, int check );
struct t_htp_msg   *t_htp_msg_create_ud( lua_State *luaVM, struct t_htp_srv *srv );
// Message specific methods
int                 t_htp_msg_rcv ( lua_State *luaVM );
int                 t_htp_msg_rsp ( lua_State *luaVM );



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

