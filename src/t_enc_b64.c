/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_enc_b64.c
 * \brief     Base64 Encoding Decoding algorithm
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdio.h>

#include "t_enc_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

static const unsigned char enc_table[ 64 ] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static       unsigned char dec_table[ 256 ];

static const uint32_t      mod_table[ ]    = { 0, 2, 1 };


// ----------------------------- Native Base64 functions


/**--------------------------------------------------------------------------
 * B64 allocator for output.
 * Depending on en- or decoding returns empty string of right size.
 * \param  size of input
 * \param  encode, if 1 then return string of size need for encoded result
 * \return size_t
 * --------------------------------------------------------------------------*/
#define B64_RESULT_SIZE( LEN, DO_ENCODE ) \
   ((DO_ENCODE) ? 4 * ((LEN + 2) / 3) :  LEN / 4 * 3)


// TODO: improve to not having to test each character for length
static void
b64_encode( const char *inbuf, char *outbuf, size_t inbuf_len)
{
	uint32_t i, j;
	uint8_t  dec1, dec2, dec3;
	size_t   outbuf_len = B64_RESULT_SIZE( inbuf_len, 1 );

	for (i = 0, j = 0; i < inbuf_len;)
	{
		dec1 = i < inbuf_len ? (uint8_t) inbuf[ i++ ] : 0;
		dec2 = i < inbuf_len ? (uint8_t) inbuf[ i++ ] : 0;
		dec3 = i < inbuf_len ? (uint8_t) inbuf[ i++ ] : 0;

		outbuf[ j++ ] = enc_table[dec1 >> 2];
		outbuf[ j++ ] = enc_table[((dec1 & 3 ) << 4) | (dec2 >> 4)];
		outbuf[ j++ ] = enc_table[((dec2 & 15) << 2) | (dec3 >> 6)];
		outbuf[ j++ ] = enc_table[dec3 & 63];
	}

	for (i = 0; i < mod_table[ inbuf_len % 3 ]; i++)
		outbuf[ outbuf_len - 1 - i ] = '=';
}


// TODO: deal with line breaks and %4 length guarantee
static void
b64_decode( const char *inbuf, char *outbuf, size_t inbuf_len )
{
	uint32_t i, j;
	uint8_t  enc1, enc2, enc3, enc4;

	for (i = 0, j = 0; i < inbuf_len;)
	{
		if (inbuf [i] == '\n' || inbuf[i] == '\r')
		{
			i++;
			continue;
		}
		enc1 = (uint8_t) dec_table[ (int) inbuf[ i++ ] ];
		enc2 = (uint8_t) dec_table[ (int) inbuf[ i++ ] ];
		enc3 = (uint8_t) dec_table[ (int) inbuf[ i++ ] ];
		enc4 = (uint8_t) dec_table[ (int) inbuf[ i++ ] ];

		outbuf[ j++ ] = (unsigned char) (enc1        << 2) | (enc2 >> 4);
		outbuf[ j++ ] = (unsigned char) ((enc2 & 15) << 4) | (enc3 >> 2);
		outbuf[ j++ ] = (unsigned char) ((enc3 &  3) << 6) |  enc4;
	}
}

/**--------------------------------------------------------------------------
 * Expose Base64 encoding to Lua; wraps native function b64_encode above.
 * \param   L      Lua state.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
 // TODO: consider using a Lua_Buffer instead of allocating and freeing memory
static int
lt_enc_b64_encode( lua_State *L )
{
	luaL_Buffer         lB;
	size_t              bLen;                                     ///< length of body
	const char         *body   = luaL_checklstring( L, 1, &bLen );
	size_t              rLen   = B64_RESULT_SIZE( bLen, 1 );      ///< length of result
	char               *res    = luaL_buffinitsize( L, &lB, rLen );

	b64_encode( body, res, bLen );
	luaL_pushresultsize( &lB, rLen );

	return 1;
}


/**--------------------------------------------------------------------------
 * Expose Base64 decoding to Lua; wraps native function b64_decode above.
 * \param   L    Lua state.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
 // TODO: consider using a Lua_Buffer instead of allocating and freeing memory
static int
lt_enc_b64_decode( lua_State *L )
{
	luaL_Buffer         lB;
	size_t              bLen;                                     ///< length of body
	const char         *body   = luaL_checklstring( L, 1, &bLen );
	size_t              rLen   = B64_RESULT_SIZE( bLen, 0 );      ///< length of result
	char               *res    = luaL_buffinitsize( L, &lB, rLen );

	b64_decode( body, res, bLen );
	luaL_pushresultsize( &lB, rLen );

	return 1;
}

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_enc_b64_cf [] = {
	  { "encode"      ,  lt_enc_b64_encode }
	, { "decode"      ,  lt_enc_b64_decode }
	, { NULL          ,  NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the T.Encode.Base64 library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The Lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_enc_b64( lua_State *L )
{
	// initializes the decoder table
	uint8_t i;
	for (i=0; i<64; i++)
		dec_table[ enc_table[i] ] = i;
	// Push the class onto the stack
	// this is avalable as T.Encode.Base64.(en/de)code
	luaL_newlib( L, t_enc_b64_cf );
	return 1;
}

