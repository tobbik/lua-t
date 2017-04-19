/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp.h
 * \brief     data types for HTTP related data types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_ael.h"

#define T_HTP_CON_IDNT     "con"
#define T_HTP_SRV_IDNT     "srv"
#define T_HTP_STR_IDNT     "str"
#define T_HTP_WSK_IDNT     "wsk"
#define T_HTP_STR_PRX_IDNT "prx"

#define T_HTP_CON_NAME     "Connection"
#define T_HTP_SRV_NAME     "Server"
#define T_HTP_STR_NAME     "Stream"
#define T_HTP_WSK_NAME     "WebSocket"
#define T_HTP_STR_PRX_NAME "Proxy"

#define T_HTP_CON_TYPE     T_HTP_TYPE"."T_HTP_CON_NAME
#define T_HTP_SRV_TYPE     T_HTP_TYPE"."T_HTP_SRV_NAME
#define T_HTP_STR_TYPE     T_HTP_TYPE"."T_HTP_STR_NAME
#define T_HTP_WSK_TYPE     T_HTP_TYPE"."T_HTP_WSK_NAME
#define T_HTP_STR_PRX_TYPE T_HTP_TYPE"."T_HTP_STR_PRX_NAME

// _   _ _____ _____ ____
//| | | |_   _|_   _|  _ \   _ __   __ _ _ __ ___  ___ _ __
//| |_| | | |   | | | |_) | | '_ \ / _` | '__/ __|/ _ \ '__|
//|  _  | | |   | | |  __/  | |_) | (_| | |  \__ \  __/ |
//|_| |_| |_|   |_| |_|     | .__/ \__,_|_|  |___/\___|_|
//                          |_|

/// Recognized HTTP Methods
enum t_htp_mth {
	T_HTP_MTH_ILLEGAL,
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
enum t_htp_srm_s {
	T_HTP_STR_ZERO,       ///< Nothing done yet
	T_HTP_STR_FLINE,      ///< Parsing First Line
	T_HTP_STR_HEADER,     ///< Parsing Headers
	T_HTP_STR_UPGRADE,    ///< Is this an upgrading connection?
	T_HTP_STR_HEADDONE,   ///< Parsing Headers finished
	T_HTP_STR_BODY,       ///< Recieving body
	T_HTP_STR_RECEIVED,   ///< Request received
	T_HTP_STR_SEND,       ///< Send data from buffer
	T_HTP_STR_FINISH,     ///< The last chunk was written into the buffer
};


// Available HTTP versions
enum t_htp_ver {
	T_HTP_VER_09,
	T_HTP_VER_10,
	T_HTP_VER_11
};


// ____        _          ____  _                   _
//|  _ \  __ _| |_ __ _  / ___|| |_ _ __ _   _  ___| |_ _   _ _ __ ___  ___
//| | | |/ _` | __/ _` | \___ \| __| '__| | | |/ __| __| | | | '__/ _ \/ __|
//| |_| | (_| | || (_| |  ___) | |_| |  | |_| | (__| |_| |_| | | |  __/\__ \
//|____/ \__,_|\__\__,_| |____/ \__|_|   \__,_|\___|\__|\__,_|_|  \___||___/
/// The userdata struct for T.Http.Server
struct t_htp_srv {
	struct t_net_sck *sck;    ///< t_net_sck socket (must be tcp)
	struct t_ael     *ael;    ///< t_ael event loop
	int               sR;     ///< Lua registry reference for t.Net.TCP instance
	int               aR;     ///< Lua registry reference for t.Net.IP4 instance (struct sockaddr_in)
	int               lR;     ///< Lua registry reference for t.Loop instance
	int               rR;     ///< Lua registry reference to request handler function
	time_t            nw;     ///< Current time on the server
	char              fnw[30];///< Formatted Date time in HTTP format
};


/// The userdata struct for T.Http.Connection ( Server:accept() )
struct t_htp_con {
/////////////////////////////////////////////////////////////////////////////
	int               pR;     ///< Lua registry reference for proxy table
	int               sR;     ///< Lua registry reference to the stream table
	int               cnt;    ///< count requests (streams) handled in this con

	// onBody() handler; anytime a read-event is fired AFTER the header was
	// received this gets executed; Can be LUA_NOREF which discards incoming data
	int               bR;     ///< Lua registry reference to body handler function
	struct t_net_sck *sck;    ///< pointer to the actual socket
	struct t_htp_srv *srv;    ///< pointer to the HTTP-Server

	int               kpAlv;  ///< keepalive value in seconds -> 0==no Keepalive
	int               upgrade;///< shall the connection be upgraded?
	enum t_htp_ver    ver;    ///< HTTP version

	size_t            read;   ///< How many byte processed
	char              buf[ BUFSIZ ];   ///< reading buffer
	const char       *b;      ///< Current start of buffer to process

	// output buffer handling with linked list (FiFo), this has significant
	// advantages in HTTP 2.0 because the stream identifiers are atomic to the
	// linked list chunks
	struct t_htp_buf *buf_head; ///< Head for the linked list
	struct t_htp_buf *buf_tail; ///< Tail for the linked list
};


/// userdata for a single request-response (HTTP stream)
struct t_htp_str {
	// Proxy contains lua readable items such as headers, length, status code etc
	int               pR;     ///< Lua registry reference for proxy table
	int               rqCl;   ///< request  content length
	int               rsCl;   ///< response content length
	int               rsBl;   ///< response buffer length (headers + rsCl)
	int               rsSl;   ///< response buffer sent length (if rsBl==rsSl; stream is done)
	int               bR;     ///< Lua registry reference to body handler function
	int               expect; ///< shall the connection return an expected thingy?
	enum t_htp_srm_s  state;  ///< HTTP Message state
	enum t_htp_mth    mth;    ///< HTTP Method for this request
	enum t_htp_ver    ver;    ///< HTTP version
	struct t_htp_con *con;    ///< pointer to the T.Http.Connection
	// in HTTP1.1 the connections counter will provide the id, in HTTP2.0
	// the ID gets provided in the protocol by the client
	int               cntId;  ///< id inherited from count in connection
};


/// userdata for HTTP connection output buffer chunk
struct t_htp_buf {
	int                bR;    ///< string reference within luaState
	int                sR;    ///< string reference within luaState
	size_t             bl;    ///< Outgoing Buffer Length (content+header)
	size_t             sl;    ///< Outgoing Sent
	char               last;  ///< Boolean to signify the last buffer for a stream
	struct t_htp_str  *str;   ///< stream this buffer is made for
	struct t_htp_buf  *prv;   ///< previous pointer for linked list
	struct t_htp_buf  *nxt;   ///< next pointer for linked list
};


//  __  __      _   _               _
// |  \/  | ___| |_| |__   ___   __| |___
// | |\/| |/ _ \ __| '_ \ / _ \ / _` / __|
// | |  | |  __/ |_| | | | (_) | (_| \__ \
// |_|  |_|\___|\__|_| |_|\___/ \__,_|___/
// t_htp.c
const char       *t_htp_pReqFirstLine( lua_State *L, struct t_htp_str *s, size_t n );
const char       *t_htp_pHeaderLine  ( lua_State *L, struct t_htp_str *s, size_t n );
const char       *t_htp_status       ( int status );


// t_htp_srv.c
// Constructors
struct t_htp_srv *t_htp_srv_check_ud ( lua_State *L, int pos, int check );
struct t_htp_srv *t_htp_srv_create_ud( lua_State *L );
void              t_htp_srv_setnow( struct t_htp_srv *s, int force );


// HTTP Connection specific methods
// Constructors
struct t_htp_con *t_htp_con_check_ud ( lua_State *L, int pos, int check );
struct t_htp_con *t_htp_con_create_ud( lua_State *L, struct t_htp_srv *srv );
// methods
int               t_htp_con_rcv    ( lua_State *L );
int               t_htp_con_rsp    ( lua_State *L );
void              t_htp_con_adjustbuffer( struct t_htp_con *c, size_t read, const char* rpos );

// HTTP Stream specific methods
// Constructors
struct t_htp_str *t_htp_str_check_ud ( lua_State *L, int pos, int check );
struct t_htp_str *t_htp_str_create_ud( lua_State *L, struct t_htp_con *con );
// methods
int               t_htp_str_rcv    ( lua_State *L, struct t_htp_str *s, size_t rcvd );
int               lt_htp_str__gc( lua_State *L );


// library exporters
LUAMOD_API int luaopen_t_htp_str( lua_State *L );
LUAMOD_API int luaopen_t_htp_con( lua_State *L );
LUAMOD_API int luaopen_t_htp_srv( lua_State *L );
LUAMOD_API int luaopen_t_htp_wsk( lua_State *L );


// __        __   _    ____             _        _
// \ \      / /__| |__/ ___|  ___   ___| | _____| |_
//  \ \ /\ / / _ \ '_ \___ \ / _ \ / __| |/ / _ \ __|
//   \ V  V /  __/ |_) |__) | (_) | (__|   <  __/ |_
//    \_/\_/ \___|_.__/____/ \___/ \___|_|\_\___|\__|

// check  https://github.com/arcapos

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
struct t_htp_wsk {
	int      sR;           /// Lua registry Reference for t_net userdata
	int      spR;          /// Lua registry Reference for subprotocol string
	int      fd;           /// copy fd from t_net_sck for direct access
	struct t_net_sck *sck; /// reference to t_net_sck type
};


struct t_htp_wsk  *t_htp_wsk_create_ud( lua_State *L );
struct t_htp_wsk  *t_htp_wsk_check_ud ( lua_State *L, int pos, int check );

