/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_ws.h
 * \brief     data types and global functions for WebSockets
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

#include "xt_sck.h"
#include "xt_lp.h"

struct xt_ws_msg_h {
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
