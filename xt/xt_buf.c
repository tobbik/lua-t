//
//
#include <memory.h>               // memset
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>            // htonl

#include "l_xt.h"
#include "xt_buf.h"


// inline helper functions
/**
  * \brief  convert lon long (64bit) from network to host and vice versa
  * \param  uint64_t value 64bit integer
  * \return uint64_t endianess corrected integer
  */
static inline uint64_t htonll (uint64_t value)
{
	uint64_t high_part = htonl( (uint64_t)(value >> 32) );
	uint64_t low_part  = htonl( (uint64_t)(value & 0xFFFFFFFFLL) );
	return (((uint64_t)low_part) << 32) | high_part;
}

// --------------------------------- HELPERS from Lua 5.3 code
static int getendian( lua_State *luaVM, int pos )
{
	const char *endian = luaL_optstring( luaVM, pos,
	                           (IS_LITTLE_ENDIAN ? "l" : "b"));
	if (*endian == 'n')  /* native? */
		return (IS_LITTLE_ENDIAN ? 1 : 0);
	luaL_argcheck( luaVM, *endian == 'l' || *endian == 'b', pos,
	                 "endianness must be 'l'/'b'/'n'" );
	return (*endian == 'l');
}



/**
 * \brief     convert 8bit integer to BCD
 * \param     val  8bit integer
 * \return    8bit integer encoding of a 2 digit BCD number
 */
int l_byteToBcd(lua_State *luaVM)
{
	uint8_t val = luaL_checkint(luaVM, 1);
	lua_pushinteger(luaVM, (val/10*16) + (val%10) );
	return 1;
}

/**
 * \brief     convert 16bit integer to BCD
 * \param     val  16bit integer
 * \return    16bit integer encoding of a BCD coded number
 */
int l_shortToBcd( lua_State *luaVM )
{
	uint16_t val = luaL_checkint( luaVM, 1 );
	lua_pushinteger( luaVM,
			(val/1000   *4096 ) +
		 ( (val/100%10)* 256 ) +
		 ( (val/10%10) * 16 ) +
			(val%10)
	);
	return 1;
}


/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___ 
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | | 
// | |__| |_| | (_| |_____/ ___ \|  __/| | 
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////
/** -------------------------------------------------------------------------
 * \brief     creates the buffer from the Constructor
 * \param     luaVM  lua state
 * \lparam    CLASS table Time
 * \lparam    length of buffer
 * \lparam    string buffer content initialized            OPTIONAL
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
static int lxt_buf__Call (lua_State *luaVM)
{
	lua_remove( luaVM, 1 );    // remove the xt.Buffer Class table
	return lxt_buf_New( luaVM );
}


/** -------------------------------------------------------------------------
 * \brief     creates the buffer from the function call
 * \param     luaVM  lua state
 * \lparam    length of buffer
 *        ALTERNATIVE
 * \lparam    string buffer content initialized
 * \return    integer   how many elements are placed on the Lua stack
 *  -------------------------------------------------------------------------*/
int lxt_buf_New( lua_State *luaVM )
{
	size_t                                   sz;
	struct xt_buf  __attribute__ ((unused)) *buf;

	if (lua_isnumber( luaVM, 1))
	{
		sz  = luaL_checkint( luaVM, 1 );
		buf = xt_buf_create_ud( luaVM, sz );
	}
	else if (lua_isstring( luaVM, 1 ))
	{
		luaL_checklstring( luaVM, 1, &sz);
		buf = xt_buf_create_ud( luaVM, sz );
		memcpy( (char*) &(buf->b[0]), luaL_checklstring( luaVM, 1, NULL ), sz );
	}
	else
	{
		xt_push_error( luaVM, "can't create xt.Buffer because of wrong argument type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * create a xt_buf and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct xt_buf*  pointer to the socket xt_buf
 * --------------------------------------------------------------------------*/
struct xt_buf *xt_buf_create_ud( lua_State *luaVM, int size )
{
	struct xt_buf  *b;
	size_t          sz;

	// size = sizof(...) -1 because the array has already one member
	sz = sizeof( struct xt_buf ) + (size - 1) * sizeof(unsigned char);
	b  = (struct xt_buf *) lua_newuserdata(luaVM, sz);
	memset(b->b, 0, size * sizeof(unsigned char));

	b->len = size;
	luaL_getmetatable(luaVM, "xt.Buffer");
	lua_setmetatable(luaVM, -2);
	return b;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an xt_buf struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return pointer to struct buf
 * --------------------------------------------------------------------------*/
struct xt_buf *xt_buf_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Buffer" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Buffer` expected" );
	return (struct xt_buf *) ud;
}


//////////////////////////////////////////////////////////////////////////////////////
/////////////// NEW IMPLEMENTATION

/**--------------------------------------------------------------------------
 * Read an integer of y bytes from a char buffer pointer
 * General helper function to read the value of an 64 bit integer from a char array
 * \param  *val        pointer to value.
 * \param   sz         how many bytes to read.
 * \param   islittle   treat input as little endian?
 * \param   buff       pointer to char array to read from.
 * --------------------------------------------------------------------------*/
static inline void xt_buf_readbytes( uint64_t *val, size_t sz, int islittle, unsigned char * buf )
{
	size_t         i;
	unsigned char *set = (unsigned char*) val;  ///< char array to read bytewise into val
#ifndef IS_LITTLE_ENDIAN
	size_t        sz_l = sizeof( *val );        ///< size of the value in bytes
#endif

#ifdef IS_LITTLE_ENDIAN
	for (i=0 ; i<sz; i++)
#else
	for (i=sz_l; i<sz_l - sz -2; i--)
#endif
	{
		if (islittle)      set[ i ] = buf[ sz-1-i ];
		else               set[ i ] = buf[ i ];
	}
}


/**--------------------------------------------------------------------------
 * Write an integer of y bytes to a char buffer pointer
 * General helper function to write the value of an 64 bit integer to a char array
 * \param  *val        pointer to value.
 * \param   sz         how many bytes to read.
 * \param   islittle   treat input as little endian?
 * \param   buff       pointer to char array to write to.
 * --------------------------------------------------------------------------*/
static inline void xt_buf_writebytes( uint64_t *val, size_t sz, int islittle, unsigned char * buf )
{
	size_t         i;
	unsigned char *set  = (unsigned char*) val;  ///< char array to read bytewise into val
#ifndef IS_LITTLE_ENDIAN
	size_t         sz_l = sizeof( *val );        ///< size of the value in bytes
#endif

#ifdef IS_LITTLE_ENDIAN
	for (i=0 ; i<sz; i++)
#else
	for (i=sz_l; i<sz_l - sz -2; i--)
#endif
	{
		if (islittle)      buf[ sz-1-i ] = set[ i ];
		else               buf[ i ]      = set[ i ];
	}
}


/**--------------------------------------------------------------------------
 * Read an integer of y bytes from the buffer at position x
 * \lparam  buf  userdata of type xt.Buffer (struct xt_buf)
 * \lparam  pos  position in bytes
 * \lparam  sz   size in bytes (1-8)
 * \lparam  end  endianess (l-little, b-big, n-native)
 * \lreturn val  lua_Integer
 *
 * \return integer 1 left on the stack
 * --------------------------------------------------------------------------*/
static int lxt_buf_readint( lua_State *luaVM )
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1 );
	int            pos = luaL_checkint( luaVM, 2 );   ///< starting byte  b->b[pos]
	int             sz = luaL_checkint( luaVM, 3 );   ///< how many bytes to read
	lua_Unsigned   val = 0;                           ///< value for the read access

	// TODO: properly calculate boundaries according #buf->b - sz etc.
	luaL_argcheck( luaVM, -1 <= pos && pos <= (int) buf->len, 2,
		                 "xt.Buffer position must be > 0 or < #buffer");
	luaL_argcheck( luaVM,  1<= sz && sz <= 8,       3,
		                 "size must be >=1 and <=8");

	xt_buf_readbytes( &val, sz, getendian( luaVM, 4 ), &(buf->b[pos]));

#ifdef PRINT_DEBUGS
	printf("%016llX    %lu     %d     %d\n", val, sizeof(lua_Unsigned), IS_LITTLE_ENDIAN, IS_BIG_ENDIAN);
#endif
	lua_pushinteger( luaVM, (lua_Integer) val );
	return 1;
}


/**--------------------------------------------------------------------------
 * Write an integer of y bytes into the buffer at position x.
 *       Any bits/bytes not used by value but covered in sz will be set to 0
 * \lparam  buf  userdata of type xt.Buffer (struct xt_buf)
 * \lparam  val  Integer value to be written into the buffer.
 * \lparam  pos  position in bytes.
 * \lparam  sz   size in bytes (1-8).
 * \lparam  end  endianess (l-little, b-big, n-native).
 *
 * \return integer 1 left on the stack
 * --------------------------------------------------------------------------*/
static int lxt_buf_writeint( lua_State *luaVM )
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1 );
	lua_Unsigned   val = (lua_Unsigned) luaL_checkinteger( luaVM, 2 );   ///< value to be written
	int            pos = luaL_checkint( luaVM, 3 );   ///< starting byte  b->b[pos]
	int             sz = luaL_checkint( luaVM, 4 );   ///< how many bytes to read

	// TODO: preperly calculate boundaries according #buf->b - sz etc.
	luaL_argcheck( luaVM, -1 <= pos && pos <= (int) buf->len, 3,
		                 "xt.Buffer position must be > 0 or < #buffer");
	luaL_argcheck( luaVM,  1<= sz && sz <= 8,       4,
		                 "size must be >=1 and <=8");

	xt_buf_writebytes( &val, sz, getendian( luaVM, 5 ), &(buf->b[pos]));

#ifdef PRINT_DEBUGS
	printf("%016llX    %lu     %d     %d\n", val, sizeof(lua_Unsigned), IS_LITTLE_ENDIAN, IS_BIG_ENDIAN);
#endif
	lua_pushinteger( luaVM, (lua_Integer) val );
	return 1;
}


/**--------------------------------------------------------------------------
 * Read an integer of y bits from the buffer at position x
 * \lparam  pos  position in bytes
 * \lparam  ofs  offset   in bits
 * \lparam  sz   size in bytes (1-8)
 * \lreturn val  lua_Integer
 *
 * \return integer 1 left on the stack
 * --------------------------------------------------------------------------*/
static int lxt_buf_readbit( lua_State *luaVM )
{
	struct xt_buf *buf = xt_buf_check_ud( luaVM, 1 );
	int            pos = luaL_checkint( luaVM, 2 );   ///< starting byte  b->b[pos]
	int            ofs = luaL_checkint( luaVM, 3 );   ///< starting byte  b->b[pos] + ofs bits
	int             sz = luaL_checkint( luaVM, 4 );   ///< how many bits  to read
	lua_Unsigned   val = 0;                           ///< value for the read access

	// TODO: properly calculate boundaries according #buf->b - sz etc.
	luaL_argcheck( luaVM,  0<= pos && pos <= (int) buf->len, 2,
		                 "xt.Buffer position must be > 0 or < #buffer");
	luaL_argcheck( luaVM,  0<= ofs && ofs <= 7,       3,
		                 "offset must be >=0 and <=7");
	luaL_argcheck( luaVM,  1<= sz  &&  sz <= 64,      4,
		                 "size must be >=1 and <=64");

	xt_buf_readbytes( &val, (sz+ofs)/8 +1, getendian( luaVM, 5 ), &(buf->b[pos]));
	lua_pushinteger( luaVM, (lua_Integer) ((val << (64- ((sz/8+1)*8) + ofs ) ) >> (64 - sz)) );

#ifdef PRINT_DEBUGS
	printf("Read Val:    %016llX\nShift Left:  %016llX\nShift right: %016llX\n%d      %d\n",
			val,
			(val << (64- ((sz/8+1)*8) + ofs ) ),
			(val << (64- ((sz/8+1)*8) + ofs ) ) >> (64 - sz),
			(64- ((sz/8+1)*8) + ofs ), (64-sz));
#endif
	//lua_pushinteger( luaVM, (lua_Integer) val );
	return 1;
}




///////////////////////////////////////// OLD IMPLEMENTATION /////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  len   in bits
 *
 * \return integer 1 left on the stack
 */
static int lxt_buf_readbits (lua_State *luaVM) {
	int                   sz;    // how many bits to write
	int                   sz_nd; // how many bits to represent value
	int                   ofs;   // starting with the x bit
	uint8_t              *v8;
	uint16_t             *v16;
	uint32_t             *v32;
	uint64_t             *v64;
	uint8_t               m8;
	uint16_t              m16;
	uint32_t              m32;
	uint64_t              m64;
	struct xt_buf        *b = xt_buf_check_ud (luaVM, 1);

	ofs = luaL_checkint(luaVM, 2);
	sz  = luaL_checkint(luaVM, 3);
	sz_nd = (ofs%8) + sz;
	//printf("o:%d s:%d n:%d c:%d\n", ofs, sz, sz_nd, (sz_nd-1)/8);

	switch ( (sz_nd-1)/8 ) {
		case 0:
			m8  = ( 0xFF >> (8-sz)) << (8 - (ofs%8) - sz);
			v8  = (uint8_t *) &(b->b[ ofs/8 ]);
			lua_pushinteger( luaVM, (*v8 & m8) >> (8 -(ofs%8) -sz));
			break;
		case 1:
			m16 = ( 0xFFFF >> (16-sz)) << (16 - (ofs%8) - sz);
			v16 = (uint16_t *) &(b->b[ ofs/8 ]);
			lua_pushinteger(luaVM, (htons(*v16) & m16) >> (16 -(ofs%8) -sz));
			break;
		case 2:
		case 3:
			m32 = ( 0xFFFFFFFF >> (32-sz)) << (32 - (ofs%8) - sz);
			v32 = (uint32_t *) &(b->b[ ofs/8 ]);
			lua_pushinteger(luaVM, (htonl(*v32) & m32) >> (32 -(ofs%8) -sz));
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			m64 = ( 0xFFFFFFFF >> (64-sz)) << (64 - (ofs%8) - sz);
			v64 = (uint64_t *) &(b->b[ ofs/8 ]);
			lua_pushinteger(luaVM, (htonll(*v64) & m64) >> (64 -(ofs%8) -sz));
			break;
		default:
			//TODO: handle error
			return xt_push_error(luaVM, "Can't handle a %d bits wide field", sz);
	}
	return 1;
}


/**
 * \brief  gets a byte wide the value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int lxt_buf_read8 (lua_State *luaVM)
{
	uint8_t       *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b   = xt_buf_check_ud (luaVM, 1);

	v = (uint8_t *) &(b->b[ p ]);
	lua_pushinteger(luaVM, (int) *v);
	return 1;
}


/**
 * \brief  gets a short 2 byte wide value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int lxt_buf_read16 (lua_State *luaVM)
{
	uint16_t      *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v = (uint16_t *) &(b->b[ p ]);
	lua_pushinteger(luaVM, (int) htons (*v));
	return 1;
}


/**
 * \brief  gets a long 4 byte wide value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int lxt_buf_read32 (lua_State *luaVM)
{
	uint32_t      *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v = (uint32_t *) &(b->b[ p ]);
	lua_pushinteger(luaVM, (int) htonl (*v));
	return 1;
}


/**
 * \brief  gets a long long 8 byte wide value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int lxt_buf_read64 (lua_State *luaVM)
{
	uint64_t      *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v = (uint64_t *) &(b->b[ p ]);
	lua_pushinteger(luaVM, (int) htonll (*v));
	return 1;
}


/**
 * \brief  gets the string from a buffer of len x for pos y
 * \param  lua Virtual Machine
 * \lparam struct xt_buf 
 * \lparam int    length in bytes
 * \lparam int    offset in bytes
 * \return integer 1 left on the stack
 */
// TODO: boundary checks on buffer vs string length
static int lxt_buf_readstring (lua_State *luaVM)
{
	struct xt_buf *b   = xt_buf_check_ud (luaVM, 1);
	int            ofs;
	int            sz;
	ofs = (lua_isnumber(luaVM, 2)) ? (size_t) luaL_checkint( luaVM, 2 ) : 0;
	sz  = (lua_isnumber(luaVM, 3)) ? (size_t) luaL_checkint( luaVM, 3 ) : b->len-ofs;

	lua_pushlstring(luaVM,
			(const char*) &(b->b[ ofs ]),
			sz);
	return 1;
}


/**
 * \brief    sets a value at a position in the stream
 *
 * \return integer 0 left on the stack
 */
static int lxt_buf_writebits(lua_State *luaVM) 
{
	int            sz;    ///> how many bits to write
	int            sz_nd; ///> how many bits needed to represent
	int            ofs;   ///> starting with the x bit
	int            nv;    ///> value to be set
	union {
		uint8_t       *v8;
		uint16_t      *v16;
		uint32_t      *v32;
		uint64_t      *v64;
	} v;
	union {
		uint8_t        m8;
		uint16_t       m16;
		uint32_t       m32;
		uint64_t       m64;
	} m;
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	ofs = luaL_checkint(luaVM, 2);
	sz  = luaL_checkint(luaVM, 3);
	nv  = luaL_checkint(luaVM, 4);
	sz_nd = (ofs%8) + sz;

	switch ( (sz_nd-1)/8 ) {
		case 0:
			m.m8     = ( 0xFF >> (8-sz)) << (8 - (ofs%8) - sz);
			v.v8     = (uint8_t *) &(b->b[ ofs/8 ]);
			*(v.v8)  = ( *(v.v8) & ~m.m8) | (nv << (8 -(ofs%8)- sz));
			break;
		case 1:
			m.m16    = ( 0xFFFF >> (16-sz)) << (16 - (ofs%8) - sz);
			v.v16    = (uint16_t *) &(b->b[ ofs/8 ]);
			*(v.v16) = htons( (htons(*(v.v16)) & ~m.m16) | (nv << (16 -(ofs%8)- sz)) );
			break;
		case 2:
		case 3:
			m.m32    = ( 0xFFFFFFFF >> (32-sz)) << (32 - (ofs%8) - sz);
			v.v32    = (uint32_t *) &(b->b[ ofs/8 ]);
			*(v.v32) = htonl( (htonl(*(v.v32)) & ~m.m32) | (nv << (32 -(ofs%8)- sz)) );
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			m.m64    = ( 0xFFFFFFFFFFFFFFFF >> (64-sz)) << (64 - (ofs%8) - sz);
			v.v64    = (uint64_t *) &(b->b[ ofs/8 ]);
			*(v.v64) = htonll( (htonll(*(v.v64)) & ~m.m64) | (nv << (64 -(ofs%8)- sz)) );
			break;
		default:
			//TODO: handle error
			return xt_push_error(luaVM, "Can't handle a %d bits wide field", sz);
	}
	return 0;
}


/**
 * \brief  sets a char 2 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int lxt_buf_write8(lua_State *luaVM)
{
	uint8_t       *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v  = (uint8_t *) &(b->b[ p ]);
	*v = (uint8_t) luaL_checknumber(luaVM, 3);
	return 0;
}


/**
 * \brief  sets a short 2 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int lxt_buf_write16(lua_State *luaVM)
{
	uint16_t      *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v  = (uint16_t *) &(b->b[ p ]);
	*v = htons( (uint16_t) luaL_checknumber(luaVM, 3) );
	return 0;
}


/**
 * \brief  sets a long 4 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int lxt_buf_write32 (lua_State *luaVM)
{
	uint32_t      *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v  = (uint32_t *) &(b->b[ p ]);
	*v = htonl( (uint32_t) luaL_checknumber(luaVM, 3) );
	return 0;
}


/**
 * \brief  sets a long long 8 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int lxt_buf_write64 (lua_State *luaVM)
{
	uint64_t      *v;
	int            p = luaL_checkint (luaVM,2); // starting byte  b->b[pos]
	struct xt_buf *b = xt_buf_check_ud (luaVM, 1);

	v  = (uint64_t *) &(b->b[ p ]);
	*v = htonll( (uint64_t) luaL_checknumber(luaVM, 3) );
	return 0;
}


/**
 * \brief  set a string in a buffer at pos y
 * \param  lua Virtual Machine
 * \lparam struct xt_buf 
 * \lparam string value to set
 * \lparam int    offset in bytes
 * \lparam length offset in bytes
 * \return integer 1 left on the stack
 */
// TODO: handle length and zero over it first
static int lxt_buf_writestring (lua_State *luaVM)
{
	struct xt_buf *b   = xt_buf_check_ud (luaVM, 1);
	int            ofs;
	size_t         sz;
	ofs = (lua_isnumber(luaVM, 3)) ? luaL_checkint(luaVM, 3) : 0;
	if (lua_isnumber(luaVM, 4)) {
		sz  = luaL_checkint(luaVM, 4);
		memcpy  ( (char*) &(b->b[ ofs ]), luaL_checklstring(luaVM, 2, NULL), sz);
	}
	else {
		memcpy  ( (char*) &(b->b[ ofs ]), luaL_checklstring(luaVM, 2, &sz), sz);
	}
	return 0;
}


/**
 * \brief    gets the content of the Stream in Hex
 * lreturn   string buffer representation in Hexadecimal
 *
 * \return integer 0 left on the stack
 */
static int lxt_buf_tohexstring (lua_State *luaVM)
{
	int            l,c;
	char          *sbuf;
	struct xt_buf *b   = xt_buf_check_ud (luaVM, 1);

	sbuf = malloc(3 * b->len * sizeof( char ));
	memset(sbuf, 0, 3 * b->len * sizeof( char ) );

	c = 0;
	for (l=0; l < (int) b->len; l++) {
		c += snprintf(sbuf+c, 4, "%02X ", b->b[l]);
	}
	lua_pushstring(luaVM, sbuf);
	return 1;
}


/**
 * \brief     returns len of the buffer
 * \param     lua state
 * \return    integer   how many elements are placed on the Lua stack
*/
static int lxt_buf__len (lua_State *luaVM)
{
	struct xt_buf *b;

	b   = xt_buf_check_ud (luaVM, 1);
	lua_pushinteger(luaVM, (int) b->len);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   tostring representation of a buffer stream.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the buffer-Stream in user_data.
 * \lreturn string    formatted string representing buffer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_buf__tostring (lua_State *luaVM)
{
	struct xt_buf *bs = xt_buf_check_ud (luaVM, 1);
	lua_pushfstring( luaVM, "Buffer{%d}: %p", bs->len, bs );
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_buf_fm [] = {
	{"__call",        lxt_buf__Call},
	{NULL,            NULL}
};


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_buf_cf [] = {
	{"new",           lxt_buf_New},
	{NULL,            NULL}
};


/**
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_buf_m [] = {
	// new implementation
	{"readInt",       lxt_buf_readint},
	{"writeInt",      lxt_buf_writeint},
	{"readBit",       lxt_buf_readbit},
	// old implementation
	{"readBits",      lxt_buf_readbits},
	{"writeBits",     lxt_buf_writebits},
	{"read8",         lxt_buf_read8},
	{"read16",        lxt_buf_read16},
	{"read32",        lxt_buf_read32},
	{"read64",        lxt_buf_read64},
	{"readString",    lxt_buf_readstring},
	{"write8",        lxt_buf_write8},
	{"write16",       lxt_buf_write16},
	{"write32",       lxt_buf_write32},
	{"write64",       lxt_buf_write64},
	{"writeString",   lxt_buf_writestring},
	// univeral stuff
	{"toHex",         lxt_buf_tohexstring},
	{"length",        lxt_buf__len},
	{"toString",      lxt_buf__tostring},
	{"ByteField",     xt_buf_fld_new_byte},
	{"BitField",      xt_buf_fld_new_bits},
	{"StringField",   xt_buf_fld_new_string},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the BufferBuffer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_buf (lua_State *luaVM)
{
	// xt.Buffer instance metatable
	luaL_newmetatable(luaVM, "xt.Buffer");
	luaL_newlib(luaVM, xt_buf_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, lxt_buf__len);
	lua_setfield(luaVM, -2, "__len");
	lua_pushcfunction(luaVM, lxt_buf__tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack
	// xt.Buffer class
	luaL_newlib(luaVM, xt_buf_cf);
	luaopen_xt_buf_fld(luaVM);
	lua_setfield(luaVM, -2, "Field");
	luaL_newlib(luaVM, xt_buf_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

