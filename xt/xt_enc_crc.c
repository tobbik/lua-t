/**
 * \file   xt_enc_crc.c
 *         crc checksum for algorithms
 *         CRC-16
 *         CRC-CCIT kermit
 *         CRC-CCIT
 *         CRC-32
 */
#include <stdio.h>
#include <stdint.h>

#include "l_xt.h"
#include "xt_enc.h"
#include "xt_buf.h"

#define TLEN            256

/**
 * \brief      Polynom definitions for the algorithms
 */
#define POLY_16         0xA001
#define POLY_CCITT_KRM  0x8408
#define POLY_CCIT       4129
#define POLY_32         0xedb88320


enum xt_CRC_ALG {
	CRC_ALG_16,
	CRC_ALG_CCITT,
	CRC_ALG_CCITT_KRM,
	CRC_ALG_32
};


// calculates CRC16,CCITT KERMIT
void calc_16 (struct xt_enc_crc *crc, const char *data, size_t len)
{
	uint8_t  idx;
	size_t   i;
	for ( i = 0; i < len; ++i) {
		idx        = (uint8_t)   ( crc->crc16       ^ data[i]);
		crc->crc16 = (uint16_t)  ((crc->crc16 >> 8) ^ crc->t16[idx]);
	}
	crc->res = (int) crc->crc16;
}


void calc_ccitt (struct xt_enc_crc *crc, const char *data, size_t len)
{
	uint8_t  idx;
	size_t   i;
	for ( i = 0; i < len; ++i) {
		idx        = (uint8_t)  ((crc->crc16 >> 8) ^ (0xff & data[i]));
		crc->crc16 = (uint16_t) ((crc->crc16 << 8) ^ crc->t16[idx]);
	}
	crc->res = (int) crc->crc16;
}


void calc_32 (struct xt_enc_crc *crc, const char *data, size_t len) {
	uint8_t  idx;
	size_t   i;
	for(i = 0; i < len; ++i) {
			 idx        = (uint8_t)  ((crc->crc32 & 0xff) ^ data[i]);
			 crc->crc32 = (uint32_t) ((crc->crc32 >> 8)   ^ crc->t32[idx]);
	}
	crc->res = (int) ~(crc->crc32);
}


/*	 ___ _   _ ___ _____ 
	|_ _| \ | |_ _|_   _|
	 | ||  \| || |  | |  
	 | || |\  || |  | |  
	|___|_| \_|___| |_|*/

static void init_16 (struct xt_enc_crc *crc, uint16_t poly)
{
	uint16_t c, run, i;
	uint8_t  j;
	for(i = 0; i < TLEN; ++i) {
		c   = 0;
		run = i;
		for(j = 0; j < 8; ++j) {
			if (((c ^ run) & 0x0001) != 0) {
				c = (uint16_t) ((c >> 1) ^ poly);
			} else {
				c >>= 1;
			}
			run >>= 1;
		}
		crc->t16 [i] = c;
	crc->crc16  = 0;
	crc->init16 = 0;
	crc->calc   = calc_16;
	}
}


static void init_ccitt (struct xt_enc_crc *crc, uint16_t poly)
{
	uint16_t c,run,i;
	uint8_t  j;
	for (i = 0; i < TLEN; ++i) {
		c   = 0;
		run = (uint16_t) (i << 8);
		for (j = 0; j < 8; ++j) {
			if (((c ^ run) & 0x8000) != 0) {
				c = (uint16_t) ((c << 1) ^ poly);
			} else {
				c <<= 1;
			}
			run <<= 1;
		}
		crc->t16 [i] = c;
	}
	crc->crc16  = 0;
	crc->init16 = 0;
	crc->calc   = calc_ccitt;
}


static void init_32 (struct xt_enc_crc *crc, uint32_t poly)
{
	uint32_t temp,i;
	uint8_t  j;
	for(i = 0; i < TLEN; ++i) {
		temp = i;
		for (j = 8; j > 0; --j) {
			if ((temp & 1) == 1) {
				temp = (uint32_t) ((temp >> 1) ^ poly);
			} else {
				temp >>= 1;
			}
		}
		crc->t32 [i] = temp;
	}
	crc->crc32  = 0xffffffff;
	crc->init32 = 0xffffffff;
	crc->calc   = calc_32;
}

// ----------------------------- Lua CRC wrapper functions
/**--------------------------------------------------------------------------
 * construct a CRC encoder and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table CRC
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int 
xt_enc_crc___call(lua_State *luaVM)
{
	lua_remove(luaVM, 1);
	return xt_enc_crc_new(luaVM);
}


/**--------------------------------------------------------------------------
 * construct a CRC encoder and return it.
 * \param   luaVM The lua state.
 * \lparam  key   key string (optional)
 * \lparam  kLen  length of key string (if key contains \0 bytes (optional))
 * \lreturn struct xt.Crc userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
xt_enc_crc_new(lua_State *luaVM)
{
	struct xt_enc_crc    *crc;
	int                   alg;

	crc = xt_enc_crc_create_ud (luaVM);
	alg = luaL_checkint(luaVM, 1);
	switch (alg) {
		case CRC_ALG_16:
			init_16 (crc, POLY_16);
			break;
		case CRC_ALG_CCITT:
			init_ccitt (crc, POLY_CCIT);
			break;
		case CRC_ALG_CCITT_KRM:
			init_16 (crc, POLY_CCITT_KRM);
			break;
		case CRC_ALG_32:
			init_32 (crc, POLY_32);
			break;
		default:
			xt_push_error(luaVM, "Unknown CRC algorithm");
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a crc encode userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_enc_crc*  pointer
 * --------------------------------------------------------------------------*/
struct xt_enc_crc
*xt_enc_crc_create_ud(lua_State *luaVM)
{
	struct xt_enc_crc *crc;

	crc = (struct xt_enc_crc *) lua_newuserdata (luaVM, sizeof(struct xt_enc_crc) );
	luaL_getmetatable(luaVM, "xt.Encode.Crc");
	lua_setmetatable(luaVM, -2);
	return crc;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct xt_enc_crc
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct xt_enc_crc*
 * --------------------------------------------------------------------------*/
struct xt_enc_crc
*xt_enc_crc_check_ud (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "xt.Encode.Crc");
	luaL_argcheck(luaVM, ud != NULL, pos, "`xt.Encode.Crc` expected");
	return (struct xt_enc_crc *) ud;
}


/** -------------------------------------------------------------------------
 * \brief   Calculate the CRC checksum over a string or xt.Buffer.
 * \param   luaVM  The lua state.
 * \lparam  xt_enc_crc userdata.
 * \lparam  data       luastring or xt.Buffer.
 * \lreturn crc        the CRC checksum.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int xt_enc_crc_calc(lua_State *luaVM)
{
	struct xt_enc_crc  *crc;
	struct xt_buf      *buf;
	const char         *msg;
	size_t              len;

	crc = xt_enc_crc_check_ud (luaVM, 1);
	if (lua_isstring(luaVM, 2)) {
		msg   = luaL_checklstring(luaVM, 2, &len);
	}
	else if (lua_isuserdata(luaVM, 2)) {
		buf  = xt_buf_check_ud (luaVM, 2);
		msg  = (const char *) &(buf->b[ 0 ]);
		//msg  =  &(buf->b[ 0 ]);
		len  = buf->len;
	}
	else
		return( xt_push_error(luaVM,
			"ERROR xt.Encoding.Crc:calc(msg) takes msg argument") );

	crc->calc (crc, msg, len);
	lua_pushinteger (luaVM, crc->res);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_enc_crc_fm [] = {
	{"__call",      xt_enc_crc___call},
	{NULL,          NULL}
};


/**
 * \brief      the CRC static class function library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg xt_enc_crc_cf [] = {
	{"new",     xt_enc_crc_new},
	{NULL,      NULL}
};


/**
 * \brief      the CRC member functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_enc_crc_m [] =
{
	{"calc",    xt_enc_crc_calc},
	{NULL,      NULL}
};



/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Encoding.CRC library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_xt_enc_crc (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable(luaVM, "xt.Encode.Crc");   // stack: functions meta
	luaL_newlib(luaVM, xt_enc_crc_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Crc.new
	luaL_newlib(luaVM, xt_enc_crc_cf);
	// set the constructor metatable Crc()
	luaL_newlib(luaVM, xt_enc_crc_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}
