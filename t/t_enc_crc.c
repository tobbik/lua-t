/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_enc_crc.c
 * \brief     CRC checksum methods for the following types
 *            CRC-16
 *            CRC-CCIT kermit
 *            CRC-CCIT
 *            CRC-32
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>    // hton*()

#include "t.h"
#include "t_enc.h"
#include "t_buf.h"

#define TLEN            256

/**
 * \brief      Polynom definitions for the algorithms
 */
#define POLY_8          0xd5
#define POLY_16         0xA001
#define POLY_CCITT_KRM  0x8408
#define POLY_CCIT       4129
#define POLY_32         0xedb88320


enum t_CRC_ALG {
	CRC_ALG_8,
	CRC_ALG_16,
	CRC_ALG_CCITT,
	CRC_ALG_CCITT_KRM,
	CRC_ALG_32
};


int
calc_8( struct t_enc_crc *crc, const char *data, size_t len )
{
	size_t   i;
	for ( i = 0; i < len; ++i)
	{
		crc->crc8 = crc->t8 [crc->crc8 ^ data[i]];
	}

	return (int) (crc->crc8 ^ 0xFF);
}


// calculates CRC16,CCITT KERMIT
int
calc_16( struct t_enc_crc *crc, const char *data, size_t len )
{
	uint8_t  idx;
	size_t   i;
	for ( i = 0; i < len; ++i)
	{
		idx        = (uint8_t)   ( crc->crc16       ^ data[i]);
		crc->crc16 = (uint16_t)  ((crc->crc16 >> 8) ^ crc->t16[idx]);
	}
	if (crc->be) return (int) crc->crc16;
	else         return (int) htons(crc->crc16);
}


int
calc_ccitt( struct t_enc_crc *crc, const char *data, size_t len )
{
	uint8_t  idx;
	size_t   i;
	for ( i = 0; i < len; ++i)
	{
		idx        = (uint8_t)  ((crc->crc16 >> 8) ^ (0xff & data[i]));
		crc->crc16 = (uint16_t) ((crc->crc16 << 8) ^ crc->t16[idx]);
	}
	if (crc->be) return (int) crc->crc16;
	else         return (int) htons(crc->crc16);
}


int
calc_32( struct t_enc_crc *crc, const char *data, size_t len )
{
	uint8_t  idx;
	size_t   i;
	for(i = 0; i < len; ++i)
	{
			 idx        = (uint8_t)  ((crc->crc32 & 0xff) ^ data[i]);
			 crc->crc32 = (uint32_t) ((crc->crc32 >> 8)   ^ crc->t32[idx]);
	}
	if (crc->be) return (int)       ~crc->crc32;
	else         return (int) htonl (~crc->crc32);
}


/*	 ___ _   _ ___ _____
	|_ _| \ | |_ _|_   _|
	 | ||  \| || |  | |
	 | || |\  || |  | |
	|___|_| \_|___| |_|*/

static void
init_8( struct t_enc_crc *crc, uint8_t poly )
{
	int     i,j;
	uint8_t c;
	for (i=0; i<256; i++)
	{
		c = i;
		for (j=0; j<8; j++)
			c = (c << 1) ^ ((c & 0x80) ? poly : 0);
		crc->t8 [i] = c & 0xFF;
	}
	crc->crc8  = 0;
	crc->init8 = 0;
	crc->calc  = calc_8;
}


static void
init_16( struct t_enc_crc *crc, uint16_t poly )
{
	uint16_t c, run, i;
	uint8_t  j;
	for(i = 0; i < TLEN; ++i)
	{
		c   = 0;
		run = i;
		for(j = 0; j < 8; ++j)
		{
			if (((c ^ run) & 0x0001) != 0)
			{
				c = (uint16_t) ((c >> 1) ^ poly);
			}
			else
			{
				c >>= 1;
			}
			run >>= 1;
		}
		crc->t16 [i] = c;
	}
	crc->crc16  = 0;
	crc->init16 = 0;
	crc->calc   = calc_16;
}


static void
init_ccitt ( struct t_enc_crc *crc, uint16_t poly )
{
	uint16_t c,run,i;
	uint8_t  j;
	for (i = 0; i < TLEN; ++i)
	{
		c   = 0;
		run = (uint16_t) (i << 8);
		for (j = 0; j < 8; ++j)
		{
			if (((c ^ run) & 0x8000) != 0)
			{
				c = (uint16_t) ((c << 1) ^ poly);
			}
			else
			{
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


static void
init_32( struct t_enc_crc *crc, uint32_t poly )
{
	uint32_t temp,i;
	uint8_t  j;
	for(i = 0; i < TLEN; ++i)
	{
		temp = i;
		for (j = 8; j > 0; --j)
		{
			if ((temp & 1) == 1)
			{
				temp = (uint32_t) ((temp >> 1) ^ poly);
			}
			else
			{
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
lt_enc_crc__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lt_enc_crc_New( luaVM );
}


/**--------------------------------------------------------------------------
 * \brief   resets the CRC encoder to it's initial value.
 * \param   luaVM  The lua state.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_enc_crc_reset( lua_State *luaVM )
{
	struct t_enc_crc  *crc;
	crc = t_enc_crc_check_ud( luaVM, 1 );
	crc->crc32 = crc->init32;
	return 0;
}



/**--------------------------------------------------------------------------
 * construct a CRC encoder and return it.
 * \param   luaVM The lua state.
 * \lparam  key   key string (optional)
 * \lparam  kLen  length of key string (if key contains \0 bytes (optional))
 * \lreturn struct t.Crc userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
lt_enc_crc_New( lua_State *luaVM )
{
	struct t_enc_crc    *crc;
	int                  alg;

	crc = t_enc_crc_create_ud( luaVM );
	alg = luaL_checkinteger( luaVM, 1 );
	switch (alg)
	{
		case CRC_ALG_8:
			init_8( crc, POLY_8 );
			break;
		case CRC_ALG_16:
			init_16( crc, POLY_16 );
			break;
		case CRC_ALG_CCITT:
			init_ccitt( crc, POLY_CCIT );
			break;
		case CRC_ALG_CCITT_KRM:
			init_16( crc, POLY_CCITT_KRM );
			break;
		case CRC_ALG_32:
			init_32( crc, POLY_32 );
			break;
		default:
			t_push_error( luaVM, "Unknown CRC algorithm" );
	}
	crc->be = (lua_isboolean( luaVM, 2 )) ? lua_toboolean( luaVM, 2 ) : 1;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a crc encode userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct t_enc_crc*  pointer
 * --------------------------------------------------------------------------*/
struct t_enc_crc
*t_enc_crc_create_ud( lua_State *luaVM )
{
	struct t_enc_crc *crc;

	crc = (struct t_enc_crc *) lua_newuserdata( luaVM, sizeof( struct t_enc_crc ) );
	luaL_getmetatable( luaVM, "t.Encode.Crc" );
	lua_setmetatable( luaVM, -2 );
	return crc;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct t_enc_crc
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct t_enc_crc*
 * --------------------------------------------------------------------------*/
struct t_enc_crc
*t_enc_crc_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "t.Encode.Crc" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`t.Encode.Crc` expected" );
	return (struct t_enc_crc *) ud;
}


/** -------------------------------------------------------------------------
 * \brief   Calculate the CRC checksum over a string or t.Buffer.
 * \param   luaVM  The lua state.
 * \lparam  t_enc_crc userdata.
 * \lparam  data       luastring or t.Buffer.
 * \lparam  sta        start index in data.
 * \lparam  end        end index in data.
 * \lreturn crc        the CRC checksum.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int
lt_enc_crc_calc( lua_State *luaVM )
{
	struct t_enc_crc  *crc;
	struct t_buf      *buf;
	const char        *msg;
	size_t             len;
	int                sta;
	int                res;

	crc = t_enc_crc_check_ud( luaVM, 1 );
	sta = (lua_isnumber( luaVM, 3 )) ? luaL_checkinteger( luaVM, 3 )     : 0;
	// if string
	if (lua_isstring( luaVM, 2 ))
	{
		msg   = luaL_checklstring( luaVM, 2, &len ) + sta;
	}
	// if t_buffer
	else if (lua_isuserdata( luaVM, 2 ))
	{
		buf  = t_buf_check_ud( luaVM, 2, 1 );
		msg  = (const char *) &(buf->b[ sta ]);
		//msg  =  &(buf->b[ 0 ]);
		len  = buf->len;
	}
	else
		return t_push_error( luaVM,
			"ERROR t.Encode.Crc:calc(msg) takes msg argument" );

	len = (lua_isnumber( luaVM, 4 )) ? (size_t) luaL_checkinteger( luaVM, 4 )-sta : len - sta;

	res = crc->calc( crc, msg, len );
	lua_pushinteger( luaVM, res );
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg t_enc_crc_fm [] = {
	{"__call",      lt_enc_crc__Call},
	{NULL,          NULL}
};


/**
 * \brief      the CRC static class function library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg t_enc_crc_cf [] = {
	{"new",     lt_enc_crc_New},
	{NULL,      NULL}
};


/**
 * \brief      the CRC member functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_enc_crc_m [] =
{
	{"calc",    lt_enc_crc_calc},
	{"reset",   lt_enc_crc_reset},
	{NULL,      NULL}
};



/**--------------------------------------------------------------------------
 * \brief   pushes the t.Encode.CRC library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_enc_crc( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( luaVM, "t.Encode.Crc" );   // stack: functions meta
	luaL_newlib( luaVM, t_enc_crc_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Crc.new
	luaL_newlib( luaVM, t_enc_crc_cf );
	// set the constructor metatable Crc()
	luaL_newlib( luaVM, t_enc_crc_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}

