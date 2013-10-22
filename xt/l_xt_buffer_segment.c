// vim: ts=4 sw=4 st=4 sta tw=80 list
//
#include <memory.h>               // memset
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>            // htonl

#include "l_xt.h"
#include "l_xt_buffer.h"


static int read_bits (lua_State *luaVM);
static int read_8 (lua_State *luaVM);
static int read_16 (lua_State *luaVM);
static int read_32 (lua_State *luaVM);
static int read_64 (lua_State *luaVM);
static int write_bits (lua_State *luaVM);
static int write_8 (lua_State *luaVM);
static int write_16 (lua_State *luaVM);
static int write_32 (lua_State *luaVM);
static int write_64 (lua_State *luaVM);
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
 * \brief     creates a buffer segment describing a single logical field in a Buffer.Stream
 * \param     lua state
 * \lparam    the Constructor instance
 * \lparam    a Buffer.Stream instance
 * \lparam    a Buffer.Segment.Type
 * \lparam    position in the Buffer.Stream
 * \return    integer   how many elements are placed on the Lua stack
*/
static int c_new_buffer_segment(lua_State *luaVM)
{
	enum buf_seg_type      type;
	int                    offset;
	int                    size;
	struct buffer_segment *seg;
	struct buffer_stream  *buffer = check_ud_buffer_stream(luaVM, 2);

	seg = create_ud_buffer_segment(luaVM);

	type   =  (enum buf_seg_type) luaL_checkint(luaVM, 3);
	size   =  luaL_checkint(luaVM, 4);
	offset =  luaL_checkint(luaVM, 5);

	printf("%d[%d],S: %d, O:%d[%d]\n", type,NUM,size,offset,offset/8);
	stackDump(luaVM);
	seg->type     = type;
	// seg->buffer   = buffer;
	seg->off_bit  = offset;
	seg->off_byte = offset/8;

	if (lua_isnumber(luaVM, 6)) {
		switch ( size/8 ) {
			case 1:
				seg->write = write_8;
				seg->read  = read_8;
				seg->val8  = (uint8_t *) &(buffer->buffer[seg->off_byte]);
				//*(seg->val8) = luaL_checkint(luaVM, 6);
				break;
			case 2:
				seg->write = write_16;
				seg->read  = read_16;
				seg->val16 = (uint16_t *) &(buffer->buffer[seg->off_byte]);
				//*(seg->val16) = luaL_checkint(luaVM, 6);
				break;
			case 4:
				seg->write = write_32;
				seg->read  = read_32;
				seg->val32 = (uint32_t *) &(buffer->buffer[seg->off_byte]);
				//*(seg->val32) = luaL_checkint(luaVM, 6);
				break;
			case 8:
				seg->write = write_64;
				seg->read  = read_64;
				seg->val64 = (uint64_t *) &(buffer->buffer[seg->off_byte]);
				//*(seg->val64) = luaL_checkint(luaVM, 6);
				break;
			default:
				seg->out_shft  = 64 - (offset%8) - size;
				seg->out_mask  = ( 0xFFFFFFFFFFFFFFFF >> (64-size)) << seg->out_shft;
				seg->val64 = (uint64_t *) &(buffer->buffer[seg->off_byte]);
				seg->write = write_bits;
				seg->read  = read_bits;
		}
	}
	else if (lua_isstring(luaVM, 6)) {
		seg->valS  = (char *) &(buffer->buffer[ seg->off_byte ]);
#ifdef _WIN32
			size_t bytes = size/8;
			strncpy_s(seg->valS, bytes, luaL_checkstring(luaVM, 6), len);
#else
			strncpy(seg->valS, luaL_checkstring(luaVM, 6), size/8);
#endif
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create a buffer_stream and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct buffer_stream*  pointer to the socket buffer_stream
 * --------------------------------------------------------------------------*/
struct buffer_segment *create_ud_buffer_segment( lua_State  *luaVM)
{
	struct buffer_segment  *seg;

	seg = (struct buffer_segment*) lua_newuserdata(luaVM, sizeof(struct buffer_segment));
	memset(seg, 0, sizeof(struct buffer_segment));

	// set instance information
	luaL_getmetatable(luaVM, "L.Buffer.Stream.Segment");
	lua_setmetatable(luaVM, -2);
	return seg;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return pointer to struct bufferStream
 */
struct buffer_segment *check_ud_buffer_segment (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.Buffer.Stream.Segment");
	luaL_argcheck(luaVM, ud != NULL, pos, "`buffer_stream` expected");
	return (struct buffer_segment *) ud;
}


/**
 * \brief  gets the value of an X bit wide segment
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 1 left on the stack
 */
static int read_bits (lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);

	lua_pushinteger(luaVM, ((htonll (*(seg->val64)) & seg->out_mask) >> seg->out_shft) );
	return 1;
}


/**
 * \brief  gets the value of a 1 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 1 left on the stack
 */
static int read_8 (lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);

	lua_pushinteger(luaVM, *(seg->val8) );
	return 1;
}


/**
 * \brief  gets the value of a 2 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 1 left on the stack
 */
static int read_16 (lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);

	lua_pushinteger(luaVM, htons( *(seg->val16) ) );
	return 1;
}


/**
 * \brief  gets the value of a 4 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 1 left on the stack
 */
static int read_32 (lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);

	lua_pushinteger(luaVM, htonl( *(seg->val32) ) );
	return 1;
}


/**
 * \brief  gets the value of a 8 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 1 left on the stack
 */
static int read_64 (lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);

	lua_pushinteger(luaVM, htonll( *(seg->val64) ) );
	return 1;
}


/**
 * \brief    sets an X bit wide value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_bits(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	int                    val   = luaL_checkint(luaVM, 2);

	*(seg->val64) = htonll( ( htonll(*(seg->val64)) & ~seg->out_mask) | (val << seg->out_shft) );
	return 0;
}


/**
 * \brief    sets a 1 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_8(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	int                    val   = luaL_checkint(luaVM, 2);

	*(seg->val8)  = val;
	return 0;
}


/**
 * \brief    sets a 2 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_16(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	int                    val   = luaL_checkint(luaVM, 2);

	*(seg->val16)  = htons( val );
	return 0;
}

/**
 * \brief    sets a 4 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_32(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	int                    val   = luaL_checkint(luaVM, 2);

	*(seg->val32)  = htonl( val );
	return 0;
}


/**
 * \brief    sets a 8 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_64(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	int                    val   = luaL_checkint(luaVM, 2);

	*(seg->val64)  = htonll( val );
	return 0;
}


/**
 * \brief  gets a value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 0 left on the stack
 */
static int l_read(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	return seg->read(luaVM);
}


/**
 * \brief  sets a value
 * \param  lua Virtual Machine
 * \lparam struct buffer_segment
 * \return integer 0 left on the stack
 */
static int l_write(lua_State *luaVM) {
	struct buffer_segment *seg   = check_ud_buffer_segment(luaVM, 1);
	return seg->write(luaVM);
}


/**--------------------------------------------------------------------------
 * \brief   tostring representation of a buffer stream.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the buffer-Stream in user_data.
 * \lreturn string    formatted string representing buffer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_buffer_segment_tostring (lua_State *luaVM) {
	struct buffer_segment *bs = check_ud_buffer_segment(luaVM, 1);
	lua_pushfstring(luaVM, "Stream_Segment{}: %p", bs);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_buffer_segment_fm [] = {
	{"__call",      c_new_buffer_segment},
	{NULL,   NULL}
};


/**
 * \brief      the buffer_stream library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_buffer_segment_m [] =
{
	{"read",     l_read},
	{"write",    l_write},
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
int luaopen_buffer_stream_segment (lua_State *luaVM)
{
	luaL_newmetatable(luaVM, "L.Buffer.Stream.Segment");   // stack: functions meta
	luaL_newlib(luaVM, l_buffer_segment_m);
	lua_setfield(luaVM, -2, "__index");
	//lua_pushcfunction(luaVM, l_buffer_segment_tostring);
	//lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack
	// empty IpEndpoint class = {}, this is the actual return of this function
	lua_createtable(luaVM, 0, 0);
	luaL_newlib(luaVM, l_buffer_segment_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

