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

#include "t_enc_l.h"
#include "t_buf.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

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
	if (crc->bE) return (int) crc->crc16;
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
	if (crc->bE) return (int) crc->crc16;
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
	if (crc->bE) return (int)       ~crc->crc32;
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
 * Construct a CRC encoder and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table CRC
 * \lparam  key    key string (optional)
 * \lparam  kLen   length of key string (if key contains \0 bytes (optional))
 * \lreturn ud     T.Encode.Crc userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_enc_crc__Call( lua_State *L )
{
	struct t_enc_crc    *crc;
	int                  alg;

	lua_remove( L, 1 );
	crc = t_enc_crc_create_ud( L );
	alg = luaL_checkinteger( L, 1 );
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
			luaL_error( L, "Unknown CRC algorithm" );
	}
	crc->bE = (lua_isboolean( L, 2 )) ? lua_toboolean( L, 2 ) : 1;

	return 1;
}


/**--------------------------------------------------------------------------
 * Resets the CRC encoder to it's initial value.
 * \param   L    The Lua state.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_enc_crc_reset( lua_State *L )
{
	struct t_enc_crc  *crc;
	crc = t_enc_crc_check_ud( L, 1 );
	crc->crc32 = crc->init32;
	return 0;
}


/**--------------------------------------------------------------------------
 * Create a crc encode userdata and push to LuaStack.
 * \param   L  The Lua state.
 * \return  struct t_enc_crc*  pointer
 * --------------------------------------------------------------------------*/
struct t_enc_crc
*t_enc_crc_create_ud( lua_State *L )
{
	struct t_enc_crc *crc;

	crc = (struct t_enc_crc *) lua_newuserdata( L, sizeof( struct t_enc_crc ) );
	luaL_getmetatable( L, T_ENC_CRC_TYPE );
	lua_setmetatable( L, -2 );
	return crc;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_enc_crc
 * \param   L        The Lua state.
 * \param   int      position on the stack
 * \return  struct t_enc_crc*
 * --------------------------------------------------------------------------*/
struct t_enc_crc
*t_enc_crc_check_ud( lua_State *L, int pos )
{
	void *ud = luaL_testudata( L, pos, T_ENC_CRC_TYPE );
	luaL_argcheck( L, ud != NULL, pos, "`"T_ENC_CRC_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_enc_crc *) ud;
}


/** -------------------------------------------------------------------------
 * Calculate the CRC checksum over a string or t.Buffer.
 * \param   L          The Lua state.
 * \lparam  ud         t_enc_crc userdata instance.
 * \lparam  data       luastring or t.Buffer.
 * \lparam  sta        start index in data.
 * \lparam  end        end index in data.
 * \lreturn crc        the CRC checksum.
 * \return  struct t_enc_crc*
 *-------------------------------------------------------------------------*/
static int
lt_enc_crc_calc( lua_State *L )
{
	struct t_enc_crc  *crc;
	const char        *msg;
	size_t             len;
	int                sta;
	int                res;

	crc = t_enc_crc_check_ud( L, 1 );
	sta = (lua_isnumber( L, 3 )) ? luaL_checkinteger( L, 3 )     : 0;
	msg = t_buf_checklstring( L, 2, &len, NULL );

	len = (lua_isnumber( L, 4 )) ? (size_t) luaL_checkinteger( L, 4 )-sta : len - sta;

	res = crc->calc( crc, msg, len );
	lua_pushinteger( L, res );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_crc_fm [] = {
	  { "__call",  lt_enc_crc__Call }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_crc_cf [] = {
	  { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_enc_crc_m [] =
{
	  { "calc",    lt_enc_crc_calc }
	, { "reset",   lt_enc_crc_reset }
	, { NULL,  NULL}
};



/**--------------------------------------------------------------------------
 * Pushes the T.Encode.CRC library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The Lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_enc_crc( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, T_ENC_CRC_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_enc_crc_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as Crc.new
	luaL_newlib( L, t_enc_crc_cf );
	// set the constructor metatable Crc()
	luaL_newlib( L, t_enc_crc_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

