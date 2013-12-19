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
static inline uint64_t htonll(uint64_t value)
{
	uint64_t high_part = htonl( (uint64_t)(value >> 32) );
	uint64_t low_part  = htonl( (uint64_t)(value & 0xFFFFFFFFLL) );
	return (((uint64_t)low_part) << 32) | high_part;
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
int l_shortToBcd(lua_State *luaVM)
{
	uint16_t val = luaL_checkint(luaVM, 1);
	lua_pushinteger(luaVM,
			(val/1000   *4096 ) +
		 ( (val/100%10)* 256 ) +
		 ( (val/10%10) * 16 ) +
			(val%10)
	);
	return 1;
}


/**
 * \brief     creates the buffer from the Constructor
 * \param     luaVM  lua state
 * \return    integer   how many elements are placed on the Lua stack
*/
static int xt_buf___call (lua_State *luaVM)
{
	lua_remove (luaVM, 1);
	return xt_buf_new (luaVM);
}


/**
 * \brief     creates the buffer from the .new() function
 * \param     luaVM  lua state
 * \return    integer   how many elements are placed on the Lua stack
*/
int xt_buf_new (lua_State *luaVM)
{
	size_t                                   sz;
	struct xt_buf  __attribute__ ((unused)) *b;

	sz  = luaL_checkint(luaVM, 1);
	b   = xt_buf_create_ud (luaVM, sz);
	if (lua_isstring(luaVM, 2)) {
		memcpy  ( (char*) &(b->b[0]), luaL_checklstring(luaVM, 2, NULL), sz);
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a xt_buf and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_buf*  pointer to the socket xt_buf
 * --------------------------------------------------------------------------*/
struct xt_buf *xt_buf_create_ud (lua_State *luaVM, int size)
{
	struct xt_buf  *b;
	size_t          sz;

	sz = sizeof(struct xt_buf) + (size - 1) * sizeof(unsigned char);
	b  = (struct xt_buf *) lua_newuserdata(luaVM, sz);
	memset(b->b, 0, size * sizeof(unsigned char));

	b->len = size;
	luaL_getmetatable(luaVM, "xt.Buffer");
	lua_setmetatable(luaVM, -2);
	return b;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  len   in bits
 *
 * \return pointer to struct buf
 */
struct xt_buf *xt_buf_check_ud (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata (luaVM, pos, "xt.Buffer");
	luaL_argcheck(luaVM, ud != NULL, pos, "`xt.Buffer` expected");
	return (struct xt_buf *) ud;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  len   in bits
 *
 * \return integer 1 left on the stack
 */
static int xt_buf_read_bits (lua_State *luaVM) {
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




	return 1;
}


/**
 * \brief  gets a byte wide the value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int xt_buf_read_8 (lua_State *luaVM) {
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
static int xt_buf_read_16 (lua_State *luaVM) {
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
static int xt_buf_read_32 (lua_State *luaVM) {
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
static int xt_buf_read_64 (lua_State *luaVM) {
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
static int xt_buf_read_string (lua_State *luaVM) {
	struct xt_buf *b   = xt_buf_check_ud (luaVM, 1);
	int            ofs;
	int            sz;
	ofs = (lua_isnumber(luaVM, 2)) ? luaL_checkint(luaVM, 2) : 0;
	sz  = (lua_isnumber(luaVM, 3)) ? luaL_checkint(luaVM, 3) : b->len-ofs;

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
static int xt_buf_write_bits(lua_State *luaVM) {
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
static int xt_buf_write_8(lua_State *luaVM) {
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
static int xt_buf_write_16(lua_State *luaVM) {
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
static int xt_buf_write_32(lua_State *luaVM) {
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
static int xt_buf_write_64(lua_State *luaVM) {
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
static int xt_buf_write_string (lua_State *luaVM) {
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
static int xt_buf_to_hex_string (lua_State *luaVM) {
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
static int xt_buf___len (lua_State *luaVM)
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
static int xt_buf___tostring (lua_State *luaVM) {
	struct xt_buf *bs = xt_buf_check_ud (luaVM, 1);
	lua_pushfstring(luaVM, "Buffer{%d}: %p", bs->len, bs);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_buf_fm [] = {
	{"__call",      xt_buf___call},
	{NULL,   NULL}
};


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_buf_cf [] = {
	{"new",           xt_buf_new},
	{NULL,            NULL}
};


/**
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_buf_m [] =
{
	{"readBits",      xt_buf_read_bits},
	{"writeBits",     xt_buf_write_bits},
	{"read8",         xt_buf_read_8},
	{"read16",        xt_buf_read_16},
	{"read32",        xt_buf_read_32},
	{"read64",        xt_buf_read_64},
	{"readString",    xt_buf_read_string},
	{"write8",        xt_buf_write_8},
	{"write16",       xt_buf_write_16},
	{"write32",       xt_buf_write_32},
	{"write64",       xt_buf_write_64},
	{"writeString",   xt_buf_write_string},
	{"length",        xt_buf___len},
	{"toString",      xt_buf___tostring},
	{"toHex",         xt_buf_to_hex_string},
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
	lua_pushcfunction(luaVM, xt_buf___len);
	lua_setfield(luaVM, -2, "__len");
	lua_pushcfunction(luaVM, xt_buf___tostring);
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

