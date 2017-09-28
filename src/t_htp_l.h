/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_l.h
 * \brief     Interfaces for HTTP related functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_htp.h"
#include "time.h"        // time_t

#include "t_net_l.h"
#include "t_ael_l.h"

/*  _   _ _____ _____ ____
 * | | | |_   _|_   _|  _ \   _ __   __ _ _ __ ___  ___ _ __
 * | |_| | | |   | | | |_) | | '_ \ / _` | '__/ __|/ _ \ '__|
 * |  _  | | |   | | |  __/  | |_) | (_| | |  \__ \  __/ |
 * |_| |_| |_|   |_| |_|     | .__/ \__,_|_|  |___/\___|_|
 *                           |_|                             */
// this needs to be in sync with the t.Http.Request.State table
enum t_htp_req_state {
	T_HTP_REQ_ZERO    = 0,
	T_HTP_REQ_METHOD  = 1,
	T_HTP_REQ_URI     = 2,
	T_HTP_REQ_VERSION = 3,
	T_HTP_REQ_HEADERS = 4,
	T_HTP_REQ_BODY    = 5,
	T_HTP_REQ_DONE    = 6,
};


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


// Available HTTP versions
enum t_htp_ver {
	T_HTP_VER_ILL,
	T_HTP_VER_09,
	T_HTP_VER_10,
	T_HTP_VER_11
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

LUAMOD_API int luaopen_t_htp_wsk( lua_State *L );
LUAMOD_API int luaopen_t_htp_req( lua_State *L );

/*  __        __   _    ____             _        _
 *  \ \      / /__| |__/ ___|  ___   ___| | _____| |_
 *   \ \ /\ / / _ \ '_ \___ \ / _ \ / __| |/ / _ \ __|
 *    \ V  V /  __/ |_) |__) | (_) | (__|   <  __/ |_
 *     \_/\_/ \___|_.__/____/ \___/ \___|_|\_\___|\__| */

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

