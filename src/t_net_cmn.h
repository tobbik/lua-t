/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_cmn.h
 * \brief     t_net_* types and unctions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#define T_NET_SCK_IDNT  "sck"

#define T_NET_SCK_NAME  "Socket"

#define T_NET_SCK_TYPE  T_NET_TYPE"."T_NET_SCK_NAME

/// The userdata struct for T.Net.Socket
struct t_net_sck {
	int   fd;    ///< socket handle
};

/// Functions to check t.Net.Socket type
struct t_net_sck *t_net_sck_check_ud ( lua_State *L, int pos, int check );
