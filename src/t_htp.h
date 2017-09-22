/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp.h
 * \brief     data types for HTTP related data types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


#define T_HTP_IDNT         "htp"
#define T_HTP_CON_IDNT     "con"
#define T_HTP_REQ_IDNT     "req"
#define T_HTP_SRV_IDNT     "srv"
#define T_HTP_STR_IDNT     "str"
#define T_HTP_WSK_IDNT     "wsk"
#define T_HTP_STR_PRX_IDNT "prx"

#define T_HTP_NAME         "Http"
#define T_HTP_CON_NAME     "Connection"
#define T_HTP_REQ_NAME     "Request"
#define T_HTP_SRV_NAME     "Server"
#define T_HTP_STR_NAME     "Stream"
#define T_HTP_WSK_NAME     "WebSocket"
#define T_HTP_STR_PRX_NAME "Proxy"

#define T_HTP_TYPE         "T."T_HTP_NAME
#define T_HTP_CON_TYPE     T_HTP_TYPE"."T_HTP_CON_NAME
#define T_HTP_REQ_TYPE     T_HTP_TYPE"."T_HTP_REQ_NAME
#define T_HTP_SRV_TYPE     T_HTP_TYPE"."T_HTP_SRV_NAME
#define T_HTP_STR_TYPE     T_HTP_TYPE"."T_HTP_STR_NAME
#define T_HTP_WSK_TYPE     T_HTP_TYPE"."T_HTP_WSK_NAME
#define T_HTP_STR_PRX_TYPE T_HTP_TYPE"."T_HTP_STR_PRX_NAME


