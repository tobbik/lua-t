/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_cmn.h
 * \brief     t_net_* types and unctions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define T_NET_IDNT          "net"
#define T_NET_ADR_IDNT      "adr"
#define T_NET_IFC_IDNT      "ifc"
#define T_NET_SCK_IDNT      "sck"
#define T_NET_SCK_PTC_IDNT  "ptc"
#define T_NET_SCK_SHT_IDNT  "sht"

#define T_NET_NAME          "Net"
#define T_NET_ADR_NAME      "Address"
#define T_NET_IFC_NAME      "Interface"
#define T_NET_SCK_NAME      "Socket"
#define T_NET_SCK_PTC_NAME  "Protocol"
#define T_NET_SCK_SHT_NAME  "Shutdown"

#define T_NET_TYPE          "T."T_NET_NAME
#define T_NET_ADR_TYPE      T_NET_TYPE"."T_NET_ADR_NAME
#define T_NET_IFC_TYPE      T_NET_TYPE"."T_NET_IFC_NAME
#define T_NET_SCK_TYPE      T_NET_TYPE"."T_NET_SCK_NAME
#define T_NET_SCK_PTC_TYPE  T_NET_TYPE"."T_NET_SCK_NAME"."T_NET_SCK_PTC_NAME
#define T_NET_SCK_SHT_TYPE  T_NET_TYPE"."T_NET_SCK_NAME"."T_NET_SCK_SHT_NAME

/// The userdata struct for T.Net.Socket
struct t_net_sck {
	int   fd;    ///< socket handle
};

/// Functions to check t.Net.Socket type
struct t_net_sck *t_net_sck_check_ud      ( lua_State *L, int pos, int check );
