//
//
#include <memory.h>               // memset
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>            // htonl

#include "l_xt.h"
#include "l_xt_buffer.h"


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
  * \brief  gets a numeric value in a buffer_stream segment according to mask and
  *         shift
  * \param  uint64_t the position in the buffer_stream (pointer)
  * \param  uint64_t out_mask  in relation to a 8 byte integer
  * \param  uint64_t out_shift to right end of 64 bit integer
  * \return uint64_t
  */
static inline uint64_t get_segment_value_bits (
		uint64_t  *valnum,
		uint64_t   out_mask,
		uint64_t   out_shift)
{
	return ((htonll (*valnum) & out_mask) >> out_shift);
}


/**
  * \brief  sets a numeric value in a qtc_pfield struct according to mask and
  *         shift
  * \param  lua_State
  * \param  struct qtc_pfield  the field to operate on
  * \param  uint64_t value the value to put into the pointer to the main buffer
  * \return int    that's what gets returned to Lua, meaning the one value on
  *                the stack
  */
static inline void set_segment_value_numeric (
		uint64_t  *valnum,
		uint64_t   out_mask,
		uint64_t   out_shift,
		uint64_t   value)
{
	//uint64_t cmpr;
	//cmpr = (64 == a->bits) ?  (uint64_t)0x1111111111111111 : ((uint64_t) 0x0000000000000001) << a->bits; // 2^bits
	//if ( value < cmpr ) {
	*valnum = htonll( ( htonll(*valnum) & ~out_mask) | (value << out_shift) );
	//}
	//else
	//	return luaL_error(luaVM,
	//	         "The value %d is too big for a %d bits wide field",
	//	         value, a->bits);
}


/**
 * \brief     creates the buffer for the the network function
 * \detail    it creates the buffer used to stor all information and send them
 *            out to the network. In order to guarantee the binary operations
 *            for 64 bit integers it must be 8 bytes longer than what gets send
 *            out. By the same time the 8bytes padding in the ned provide space
 *            for two bytes used as placeholder for the CRC16 checksum if needed
 * \param     lua state
 * \return    integer   how many elements are placed on the Lua stack
*/
static int c_new_buffer_stream(lua_State *luaVM)
{
	int                    size;
	struct buffer_stream  __attribute__ ((unused)) *buffer;

	size           = luaL_checkint(luaVM, 2);
	buffer         = create_ud_buffer_stream(luaVM, size);

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a buffer_stream and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct buffer_stream*  pointer to the socket buffer_stream
 * --------------------------------------------------------------------------*/
struct buffer_stream *create_ud_buffer_stream(lua_State *luaVM, int size)
{
	struct buffer_stream  *buffer;
	size_t                 size_bytes;

	buffer = (struct buffer_stream*) lua_newuserdata(luaVM, sizeof(struct buffer_stream));
	size_bytes     = sizeof(struct buffer_stream) + (size - 1) * sizeof(unsigned char);
	buffer         = (struct buffer_stream *) lua_newuserdata(luaVM, size_bytes);
	memset(buffer->buffer, 0, size * sizeof(unsigned char));

	buffer->length = size;
	luaL_getmetatable(luaVM, "L.Buffer.Stream");
	lua_setmetatable(luaVM, -2);
	return buffer;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return pointer to struct bufferStream
 */
struct buffer_stream *check_ud_buffer_stream (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.Buffer.Stream");
	luaL_argcheck(luaVM, ud != NULL, pos, "`buffer_stream` expected");
	return (struct buffer_stream *) ud;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return integer 1 left on the stack
 */
static int l_read_number_bits (lua_State *luaVM) {
	int                   length;    // how many bits to write
	int                   offset;    // starting with the x bit
	uint64_t             *valnum;
	uint64_t              out_mask;
	uint64_t              out_shift;
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	offset = luaL_checkint(luaVM, 2);
	length = luaL_checkint(luaVM, 3);

	out_shift = 64 - (offset%8) - length;
	out_mask = ( 0xFFFFFFFFFFFFFFFF >> (64-length)) << out_shift;

	valnum = (uint64_t *) &(buffer->buffer[ offset/8 ]);
	lua_pushinteger(luaVM, get_segment_value_bits(valnum, out_mask, out_shift));
	return 1;
}

/**
 * \brief  gets a byte wide the value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int l_read_8 (lua_State *luaVM) {
	uint8_t              *valnum;
	int                   pos=luaL_checkint(luaVM,2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint8_t *) &(buffer->buffer[ pos ]);
	lua_pushinteger(luaVM, (int) *valnum);
	return 1;
}


/**
 * \brief  gets a short 2 byte wide value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int l_read_16 (lua_State *luaVM) {
	uint16_t             *valnum;
	int                   pos=luaL_checkint(luaVM,2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint16_t *) &(buffer->buffer[ pos ]);
	lua_pushinteger(luaVM, (int) htons (*valnum));
	return 1;
}


/**
 * \brief  gets a long 4 byte wide value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int l_read_32 (lua_State *luaVM) {
	uint32_t             *valnum;
	int                   pos=luaL_checkint(luaVM,2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint32_t *) &(buffer->buffer[ pos ]);
	lua_pushinteger(luaVM, (int) htonl (*valnum));
	return 1;
}


/**
 * \brief  gets a long long 8 byte wide value from stream
 * \lparam  position in bytes
 *
 * \return integer 1 left on the stack
 */
static int l_read_64 (lua_State *luaVM) {
	uint64_t             *valnum;
	int                   pos=luaL_checkint(luaVM,2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint64_t *) &(buffer->buffer[ pos ]);
	lua_pushinteger(luaVM, (int) htonll (*valnum));
	return 1;
}


/**
 * \brief    sets a value at a position in the stream
 *
 * \return integer 0 left on the stack
 */
static int l_write_number_bits(lua_State *luaVM) {
	int                   length;    // how many bits to write
	int                   offset;    // starting with the x bit
	uint64_t             *valnum;
	uint64_t              out_mask;
	uint64_t              out_shift;
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	offset = luaL_checkint(luaVM, 2);
	length = luaL_checkint(luaVM, 3);

	out_shift = 64 - (offset%8) - length;
	out_mask = ( 0xFFFFFFFFFFFFFFFF >> (64-length)) << out_shift;

	valnum = (uint64_t *) &(buffer->buffer[ offset/8 ]);

	set_segment_value_numeric(
		valnum, out_mask, out_shift,
		(uint64_t) luaL_checknumber(luaVM, 4));
	return 0;
}


/**
 * \brief  sets a char 2 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int l_write_8(lua_State *luaVM) {
	uint8_t              *valnum;
	int                   pos=luaL_checkint(luaVM, 2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum  = (uint8_t *) &(buffer->buffer[ pos ]);
	*valnum = (uint8_t) luaL_checknumber(luaVM, 3);

	return 0;
}


/**
 * \brief  sets a short 2 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int l_write_16(lua_State *luaVM) {
	uint16_t             *valnum;
	int                   pos=luaL_checkint(luaVM, 2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint16_t *) &(buffer->buffer[ pos ]);
	*valnum = htons( (uint16_t) luaL_checknumber(luaVM, 3) );

	return 0;
}


/**
 * \brief  sets a long 4 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int l_write_32(lua_State *luaVM) {
	uint32_t             *valnum;
	int                   pos=luaL_checkint(luaVM, 2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint32_t *) &(buffer->buffer[ pos ]);
	*valnum = htonl( (uint32_t) luaL_checknumber(luaVM, 3) );

	return 0;
}


/**
 * \brief  sets a long long 8 byte wide value in stream
 * \lparam  position in bytes
 * \lparam  value
 *
 * \return integer 0 left on the stack
 */
static int l_write_64(lua_State *luaVM) {
	uint64_t             *valnum;
	int                   pos=luaL_checkint(luaVM, 2);       // starting with byte pos in buffer[]
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	valnum = (uint64_t *) &(buffer->buffer[ pos ]);
	*valnum = htonll( (uint64_t) luaL_checknumber(luaVM, 3) );

	return 0;
}


/**
 * \brief    gets the content of the Stream in Hex
 * lreturn   string buffer representation in Hexadecimal
 *
 * \return integer 0 left on the stack
 */
static int l_get_hex_string(lua_State *luaVM) {
	int                   l,c;
	char                 *b;
	struct buffer_stream *buffer   = check_ud_buffer_stream(luaVM, 1);

	b=malloc(3 * buffer->length * sizeof( char ));
	memset(b, 0, 3 * buffer->length * sizeof( char ) );

	c = 0;
	for (l=0; l<buffer->length; l++) {
		c += snprintf(b+c, 4, "%02X ", buffer->buffer[l]);
	}
	lua_pushstring(luaVM, b);
	return 1;
}


/**
 * \brief     returns length of the buffer
 * \param     lua state
 * \return    integer   how many elements are placed on the Lua stack
*/
static int l_get_length(lua_State *luaVM)
{
	struct buffer_stream *buffer;

	buffer   = check_ud_buffer_stream(luaVM, 1);
	lua_pushinteger(luaVM, (int) buffer->length);

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   tostring representation of a buffer stream.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the buffer-Stream in user_data.
 * \lreturn string    formatted string representing buffer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_stream_tostring (lua_State *luaVM) {
	struct buffer_stream *bs = check_ud_buffer_stream(luaVM, 1);
	lua_pushfstring(luaVM, "Stream{%d}: %p", bs->length, bs);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_buffer_stream_fm [] = {
	{"__call",      c_new_buffer_stream},
	{NULL,   NULL}
};


/**
 * \brief      the buffer_stream library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_buffer_stream_m [] =
{
	{"readBits",     l_read_number_bits},
	{"writeBits",    l_write_number_bits},
	{"read8",        l_read_8},
	{"read16",       l_read_16},
	{"read32",       l_read_32},
	{"read64",       l_read_64},
	{"write8",       l_write_8},
	{"write16",      l_write_16},
	{"write32",      l_write_32},
	{"write64",      l_write_64},
	{"length",       l_get_length},
	{"toHex",        l_get_hex_string},
	{NULL,        NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the BufferBuffer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_buffer_stream (lua_State *luaVM)
{
	luaL_newmetatable(luaVM, "L.Buffer.Stream");   // stack: functions meta
	luaL_newlib(luaVM, l_buffer_stream_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, l_get_length);
	lua_setfield(luaVM, -2, "__len");
	lua_pushcfunction(luaVM, l_stream_tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack
	// empty IpEndpoint class = {}, this is the actual return of this function
	lua_createtable(luaVM, 0, 0);
	luaL_newlib(luaVM, l_buffer_stream_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

