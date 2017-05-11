/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_enc.h
 * \brief     t_enc_* types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// includes the Lua headers
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define T_ENC_IDNT "enc"
#define T_ENC_RC4_IDNT  "rc4"
#define T_ENC_CRC_IDNT  "crc"
#define T_ENC_B64_IDNT  "b64"

#define T_ENC_NAME "Encode"
#define T_ENC_RC4_NAME  "Rc4"
#define T_ENC_CRC_NAME  "Crc"
#define T_ENC_B64_NAME  "Base64"

#define T_ENC_TYPE "T."T_ENC_NAME
#define T_ENC_RC4_TYPE  T_ENC_TYPE"."T_ENC_RC4_NAME
#define T_ENC_CRC_TYPE  T_ENC_TYPE"."T_ENC_CRC_NAME
#define T_ENC_B64_TYPE  T_ENC_TYPE"."T_ENC_B64_NAME

