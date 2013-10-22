/* vim: ts=4 sw=4 st=4 sta tw=80 list
*/
/**
 * \file    l_xt_buf_seg.c
 * \brief   represents a chunk of data within a buffer
 * \detail  provides esay access to a segment of underlaying buffer
 *          provides acces by
 *            - bits
 *            - bytes
 *            - string based
 */
#include <memory.h>               // memset
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>            // htonl

#include "l_xt.h"
#include "l_xt_buf.h"


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
 * \brief     creates a buffer segment describing a single logical field in a Buffer
 * \param     lua state
 * \lparam    the Constructor instance
 * \lparam    a Buffer instance
 * \lparam    a Buffer.Segment.Type
 * \lparam    position in the Buffer
 * \return    integer   how many elements are placed on the Lua stack
*/
int c_new_buf_seg(lua_State *luaVM)
{
	enum xt_buf_seg_type  type;
	int                   offset;
	int                   size;
	struct xt_buf_seg    *seg;
	struct xt_buf        *buf = check_ud_buf(luaVM, 1);

	seg = create_ud_buf_seg(luaVM);

	type   =  (enum xt_buf_seg_type) luaL_checkint(luaVM, 2);
	size   =  luaL_checkint(luaVM, 3);
	offset =  luaL_checkint(luaVM, 4);

	printf("%d[%d],S: %d, O:%d[%d]\n", type, BUF_SEG_BIT,size,offset,offset/8);
	seg->type     = type;
	seg->ofs_bit  = offset;
	seg->ofs_byte = offset/8;

	if (lua_isnumber(luaVM, 5)) {
		switch ( size/8 ) {
			case 1:
				seg->write = write_8;
				seg->read  = read_8;
				seg->v8    = (uint8_t *) &(buf->b[seg->ofs_byte]);
				//*(seg->val8) = luaL_checkint(luaVM, 5);
				break;
			case 2:
				seg->write = write_16;
				seg->read  = read_16;
				seg->v16 = (uint16_t *) &(buf->b[seg->ofs_byte]);
				//*(seg->val16) = luaL_checkint(luaVM, 5);
				break;
			case 4:
				seg->write = write_32;
				seg->read  = read_32;
				seg->v32   = (uint32_t *) &(buf->b[seg->ofs_byte]);
				//*(seg->val32) = luaL_checkint(luaVM, 5);
				break;
			case 8:
				seg->write = write_64;
				seg->read  = read_64;
				seg->v64   = (uint64_t *) &(buf->b[seg->ofs_byte]);
				//*(seg->val64) = luaL_checkint(luaVM, 5);
				break;
			default:
				seg->shft  = 64 - (offset%8) - size;
				seg->mask  = ( 0xFFFFFFFFFFFFFFFF >> (64-size)) << seg->shft;
				seg->v64   = (uint64_t *) &(buf->b[seg->ofs_byte]);
				seg->write = write_bits;
				seg->read  = read_bits;
		}
	}
	else if (lua_isstring(luaVM, 5)) {
		seg->vS  = (char *) &(buf->b[ seg->ofs_byte ]);
#ifdef _WIN32
			size_t bytes = size/8;
			strncpy_s(seg->vS, bytes, luaL_checkstring(luaVM, 5), len);
#else
			strncpy(seg->vS, luaL_checkstring(luaVM, 5), size/8);
#endif
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create an xt_buf_seg and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_buf_seg*  pointer to the buffer segment
 * --------------------------------------------------------------------------*/
struct xt_buf_seg *create_ud_buf_seg ( lua_State  *luaVM)
{
	struct xt_buf_seg  *seg;

	seg = (struct xt_buf_seg*) lua_newuserdata(luaVM, sizeof(struct xt_buf_seg));
	memset(seg, 0, sizeof(struct xt_buf_seg));

	// set instance information
	luaL_getmetatable(luaVM, "L.Buffer.Segment");
	lua_setmetatable(luaVM, -2);
	return seg;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return pointer to struct xt_buf
 */
struct xt_buf_seg *check_ud_buf_seg (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.Buffer.Segment");
	luaL_argcheck(luaVM, ud != NULL, pos, "`Buffer.Segment` expected");
	return (struct xt_buf_seg *) ud;
}


/**
 * \brief  gets the value of an X bit wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 1 left on the stack
 */
static int read_bits (lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);

	lua_pushinteger(luaVM, ((htonll (*(seg->v64)) & seg->mask) >> seg->shft) );
	return 1;
}


/**
 * \brief  gets the value of a 1 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 1 left on the stack
 */
static int read_8 (lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);

	lua_pushinteger(luaVM, *(seg->v8) );
	return 1;
}


/**
 * \brief  gets the value of a 2 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 1 left on the stack
 */
static int read_16 (lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);

	lua_pushinteger(luaVM, htons( *(seg->v16) ) );
	return 1;
}


/**
 * \brief  gets the value of a 4 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 1 left on the stack
 */
static int read_32 (lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);

	lua_pushinteger(luaVM, htonl( *(seg->v32) ) );
	return 1;
}


/**
 * \brief  gets the value of a 8 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 1 left on the stack
 */
static int read_64 (lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);

	lua_pushinteger(luaVM, htonll( *(seg->v64) ) );
	return 1;
}


/**
 * \brief    sets an X bit wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_bits(lua_State *luaVM) {
	struct xt_buf_seg *seg = check_ud_buf_seg(luaVM, 1);
	*(seg->v64) = htonll(
		( htonll(*(seg->v64)) & ~seg->mask) |
		( luaL_checkint(luaVM,2) << seg->shft) );
	return 0;
}


/**
 * \brief    sets a 1 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_8(lua_State *luaVM) {
	struct xt_buf_seg *seg = check_ud_buf_seg(luaVM, 1);
	*(seg->v8) = luaL_checkint(luaVM, 2);
	return 0;
}


/**
 * \brief    sets a 2 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_16(lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);
	*(seg->v16)  = htons( luaL_checkint(luaVM, 2) );
	return 0;
}

/**
 * \brief    sets a 4 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_32(lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);
	*(seg->v32)  = htonl( luaL_checkint(luaVM, 2) );
	return 0;
}


/**
 * \brief    sets a 8 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_64(lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);
	*(seg->v64)  = htonll( luaL_checkint(luaVM, 2) );
	return 0;
}


/**
 * \brief  gets a value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 0 left on the stack
 */
static int l_read(lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);
	return seg->read(luaVM);
}


/**
 * \brief  sets a value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_seg
 * \return integer 0 left on the stack
 */
static int l_write(lua_State *luaVM) {
	struct xt_buf_seg *seg   = check_ud_buf_seg(luaVM, 1);
	return seg->write(luaVM);
}


/**--------------------------------------------------------------------------
 * \brief   tostring representation of a buffer segment.
 * \param   luaVM      The lua state.
 * \lparam  xt_buf_seg the buffer segment in user_data.
 * \lreturn string     formatted string representing buffer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_buf_seg_tostring (lua_State *luaVM) {
	struct xt_buf_seg *bs = check_ud_buf_seg(luaVM, 1);
	lua_pushfstring(luaVM, "Buffer.Segment{}: %p", bs);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_buf_seg_fm [] = {
	{"__call",      c_new_buf_seg},
	{NULL,   NULL}
};


/**
 * \brief      the buffer segment library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_buf_seg_m [] =
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
int luaopen_buf_seg (lua_State *luaVM)
{
	luaL_newmetatable(luaVM, "L.Buffer.Segment");   // stack: functions meta
	luaL_newlib(luaVM, l_buf_seg_m);
	lua_setfield(luaVM, -2, "__index");
	//lua_pushcfunction(luaVM, l_buf_seg_tostring);
	//lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack
	// empty IpEndpoint class = {}, this is the actual return of this function
	lua_createtable(luaVM, 0, 0);
	lua_pushinteger(luaVM, BUF_SEG_BIT);
	lua_setfield(luaVM, -2, "BIT");
	lua_pushinteger(luaVM, BUF_SEG_BYTE);
	lua_setfield(luaVM, -2, "BYTE");
	lua_pushinteger(luaVM, BUF_SEG_STR);
	lua_setfield(luaVM, -2, "STRING");
	luaL_newlib(luaVM, l_buf_seg_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

