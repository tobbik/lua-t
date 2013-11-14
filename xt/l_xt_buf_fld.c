/* vim: ts=4 sw=4 st=4 sta tw=80 list
*/
/**
 * \file    l_xt_buf_fld.c
 * \brief   represents a chunk of data within a buffer
 * \detail  provides esay access to a field of underlaying buffer
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


static int read_8 (lua_State *luaVM);
static int read_16 (lua_State *luaVM);
static int read_32 (lua_State *luaVM);
static int read_64 (lua_State *luaVM);
static int read_bits_8 (lua_State *luaVM);
static int read_bits_16 (lua_State *luaVM);
static int read_bits_32 (lua_State *luaVM);
static int read_bits_64 (lua_State *luaVM);
static int read_string (lua_State *luaVM);
static int write_8 (lua_State *luaVM);
static int write_16 (lua_State *luaVM);
static int write_32 (lua_State *luaVM);
static int write_64 (lua_State *luaVM);
static int write_bits_8 (lua_State *luaVM);
static int write_bits_16 (lua_State *luaVM);
static int write_bits_32 (lua_State *luaVM);
static int write_bits_64 (lua_State *luaVM);
static int write_string (lua_State *luaVM);

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
 * \brief     creates a buffer field describing a single logical field in a Buffer
 * \param     lua state
 * \lparam    the Constructor instance
 * \lparam    a Buffer instance
 * \lparam    position in the Buffer
 * \return    integer   how many elements are placed on the Lua stack
*/
// TODO: ARGCHECK size>0 < buflength-offset
int c_new_buf_fld_bits(lua_State *luaVM)
{
	int                ofs;   ///< offset in bits from buffer start
	int                sz;    ///< size of field in bits
	int                sz_nd; ///< bytes needed to represent field
	struct xt_buf_fld *f;
	struct xt_buf     *buf = check_ud_buf(luaVM, 1);

	sz    =  luaL_checkint(luaVM, 2);
	ofs   =  luaL_checkint(luaVM, 3);
	sz_nd = (ofs%8) + sz;

	f = create_ud_buf_fld(luaVM);
	f->type     = BUF_FLD_BIT;
	f->ofs      = ofs;
	f->sz       = sz;

	switch ( (sz_nd-1)/8 ) {
		case 0:
			f->shft  = 8 - (ofs%8) - sz;
			f->m.m8  = ( 0xFF >> (8-sz)) << f->shft;
			f->write = write_bits_8;
			f->read  = read_bits_8;
			f->v.v8  = (uint8_t *) &(buf->b[f->ofs/8]);
			break;
		case 1:
			f->shft  = 16 - (ofs%8) - sz;
			f->m.m16 = ( 0xFFFF >> (16-sz)) << f->shft;
			f->write = write_bits_16;
			f->read  = read_bits_16;
			f->v.v16 = (uint16_t *) &(buf->b[f->ofs/8]);
			break;
		case 2:
		case 3:
			f->shft  = 32 - (ofs%8) - sz;
			f->m.m32 = ( 0xFFFFFFFF >> (32-sz)) << f->shft;
			f->write = write_bits_32;
			f->read  = read_bits_32;
			f->v.v32 = (uint32_t *) &(buf->b[f->ofs/8]);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			f->shft  = 64 - (ofs%8) - sz;
			f->m.m64 = ( 0xFFFFFFFF >> (64-sz)) << f->shft;
			f->write = write_bits_64;
			f->read  = read_bits_64;
			f->v.v64 = (uint64_t *) &(buf->b[f->ofs/8]);
			break;
		default:
			//TODO: handle error
			pusherror(luaVM, "Can't handle size of bit field");
	}
	if (lua_isnumber(luaVM, 4)) {
	// TODO handle setting the value
	}
	return 1;
}


/**
 * \brief     creates a buffer field describing a single logical field in a Buffer
 * \param     lua state
 * \lparam    the Constructor instance
 * \lparam    a Buffer instance
 * \lparam    position in the Buffer
 * \return    integer   how many elements are placed on the Lua stack
*/
int c_new_buf_fld_byte(lua_State *luaVM)
{
	int                ofs; ///< offset to buffer start in byte
	int                sz;  ///< size in byte
	struct xt_buf_fld *f;
	struct xt_buf     *buf = check_ud_buf(luaVM, 1);

	sz  =  luaL_checkint(luaVM, 2);
	ofs =  luaL_checkint(luaVM, 3);

	f = create_ud_buf_fld(luaVM);
	f->type     = BUF_FLD_BYTE;
	f->ofs      = ofs*8;
	f->sz       = sz*8;

	switch ( sz ) {
		case 1:
			f->write = write_8;
			f->read  = read_8;
			f->v.v8  = (uint8_t *) &(buf->b[f->ofs/8]);
			break;
		case 2:
			f->write = write_16;
			f->read  = read_16;
			f->v.v16 = (uint16_t *) &(buf->b[f->ofs/8]);
			break;
		case 4:
			f->write = write_32;
			f->read  = read_32;
			f->v.v32 = (uint32_t *) &(buf->b[f->ofs/8]);
			break;
		case 8:
			f->write = write_64;
			f->read  = read_64;
			f->v.v64 = (uint64_t *) &(buf->b[f->ofs/8]);
			break;
		default:
			// TODO: can't handle others than 1,2,4,8 byte
			pusherror(luaVM, "bytes must be 1,2,4 or 8");
	}
	if (lua_isnumber(luaVM, 4)) {
	// TODO handle setting the value
	}
	return 1;
}


/**
 * \brief     creates a buffer field describing a single logical field in a Buffer
 * \param     lua state
 * \lparam    the Constructor instance
 * \lparam    a Buffer instance
 * \lparam    position in the Buffer
 * \return    integer   how many elements are placed on the Lua stack
*/
int c_new_buf_fld_string(lua_State *luaVM)
{
	int                ofs; ///< offset to buffer start in byte
	int                sz;  ///< size in byte
	struct xt_buf_fld *f;
	struct xt_buf     *buf = check_ud_buf(luaVM, 1);

	sz  =  luaL_checkint(luaVM, 2);
	ofs =  luaL_checkint(luaVM, 3);

	f = create_ud_buf_fld(luaVM);
	f->type     = BUF_FLD_STR;
	f->ofs      = ofs*8;
	f->sz       = sz*8;
	f->v.vS     = (char *) &(buf->b[ f->ofs/8 ]);
	f->write = write_string;
	f->read  = read_string;

	if (lua_isstring(luaVM, 4)) {
#ifdef _WIN32
			size_t bytes = sz/8;
			strncpy_s(f->v.vS, bytes, luaL_checkstring(luaVM, 4), len);
#else
			strncpy(f->v.vS, luaL_checkstring(luaVM, 4), sz);
#endif
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create an xt_buf_fld and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_buf_fld*  pointer to the buffer field
 * --------------------------------------------------------------------------*/
struct xt_buf_fld *create_ud_buf_fld ( lua_State  *luaVM)
{
	struct xt_buf_fld  *f;

	f = (struct xt_buf_fld*) lua_newuserdata(luaVM, sizeof(struct xt_buf_fld));
	memset(f, 0, sizeof(struct xt_buf_fld));

	// set instance information
	luaL_getmetatable(luaVM, "L.Buffer.Field");
	lua_setmetatable(luaVM, -2);
	return f;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return pointer to struct xt_buf
 */
struct xt_buf_fld *check_ud_buf_fld (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.Buffer.Field");
	luaL_argcheck(luaVM, ud != NULL, pos, "`Buffer.Field` expected");
	return (struct xt_buf_fld *) ud;
}


/**
 * \brief  gets the value of an X bit wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_bits_8 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, ((*(f->v.v8) & f->m.m8) >> f->shft) );
	return 1;
}


/**
 * \brief  gets the value of an X bit wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_bits_16 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, ((htons (*(f->v.v16)) & f->m.m16) >> f->shft) );
	return 1;
}


/**
 * \brief  gets the value of an X bit wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_bits_32 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, ((htonl (*(f->v.v32)) & f->m.m32) >> f->shft) );
	return 1;
}


/**
 * \brief  gets the value of an X bit wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_bits_64 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, ((htonll (*(f->v.v64)) & f->m.m64) >> f->shft) );
	return 1;
}


/**
 * \brief  gets the value of a 1 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_8 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, *(f->v.v8) );
	return 1;
}


/**
 * \brief  gets the value of a 2 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_16 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, htons( *(f->v.v16) ) );
	return 1;
}


/**
 * \brief  gets the value of a 4 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_32 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, htonl( *(f->v.v32) ) );
	return 1;
}


/**
 * \brief  gets the value of a 8 byte wide segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_64 (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushinteger(luaVM, htonll( *(f->v.v64) ) );
	return 1;
}


/**
 * \brief  gets the value of a string segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int read_string (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);

	lua_pushlstring(luaVM, f->v.vS, f->sz/8 );
	return 1;
}


/**
 * \brief  sets an max 8 bit wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_bits_8(lua_State *luaVM) {
	struct xt_buf_fld *f = check_ud_buf_fld(luaVM, 1);
	int                v   = luaL_checkint(luaVM, 2);
	luaL_argcheck(luaVM, 0 <= v && v <=255 , 2, "value out of range");
	*(f->v.v8) =  ( *(f->v.v8) & ~f->m.m8) | ( ((uint8_t)v) << f->shft);
	return 0;
}


/**
 * \brief  sets an max 16 bit wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_bits_16(lua_State *luaVM) {
	struct xt_buf_fld *f = check_ud_buf_fld(luaVM, 1);
	int                v   = luaL_checkint(luaVM, 2);
	luaL_argcheck(luaVM, 0 <= v && v <= 65536, 2, "value out of range");
	*(f->v.v16) = htons(
		( htons(*(f->v.v16)) & ~f->m.m16) |
		( v << f->shft));
	return 0;
}


/**
 * \brief  sets an max 32 bit wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_bits_32(lua_State *luaVM) {
	struct xt_buf_fld *f = check_ud_buf_fld(luaVM, 1);
	int                v   = luaL_checkint(luaVM, 2);
	luaL_argcheck(luaVM, 0 <= v && v <=2147483647 , 2, "value out of range");
	*(f->v.v32) = htonl(
		( htonl(*(f->v.v32)) & ~f->m.m32) |
		( v << f->shft));
	return 0;
}


/**
 * \brief  sets an max 64 bit wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_bits_64(lua_State *luaVM) {
	struct xt_buf_fld *f = check_ud_buf_fld(luaVM, 1);
	int                v   = luaL_checkint(luaVM, 2);
	luaL_argcheck(luaVM, 0 <= v && v <=2147483647 , 2, "value out of range");
	*(f->v.v64) = htonll(
		( htonll(*(f->v.v64)) & ~f->m.m64) |
		( v << f->shft));
	return 0;
}


/**
 * \brief    sets a 1 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_8(lua_State *luaVM) {
	struct xt_buf_fld *f = check_ud_buf_fld(luaVM, 1);
	*(f->v.v8) = luaL_checkint(luaVM, 2);
	return 0;
}


/**
 * \brief    sets a 2 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_16(lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);
	*(f->v.v16)  = htons( luaL_checkint(luaVM, 2) );
	return 0;
}


/**
 * \brief    sets a 4 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_32(lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);
	*(f->v.v32)  = htonl( luaL_checkint(luaVM, 2) );
	return 0;
}


/**
 * \brief    sets a 8 byte wide value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \lparam int    val
 * \return integer 0 left on the stack
 */
static int write_64(lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);
	*(f->v.v64)  = htonll( luaL_checkint(luaVM, 2) );
	return 0;
}


/**
 * \brief  sets the value of a string segment
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 1 left on the stack
 */
static int write_string (lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);
	size_t l;
	//TODO make sure string is not longer than f->sz*8
#ifdef _WIN32
	strncpy_s(f->v.vS, l, luaL_checklstring(luaVM, 2, &l), l);
#else
	strncpy(f->v.vS, luaL_checklstring(luaVM, 2, &l), l);
#endif
	return 0;
}


/**
 * \brief  gets a value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 0 left on the stack
 */
static int l_read(lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);
	return f->read(luaVM);
}


/**
 * \brief  sets a value
 * \param  lua Virtual Machine
 * \lparam struct xt_buf_fld
 * \return integer 0 left on the stack
 */
static int l_write(lua_State *luaVM) {
	struct xt_buf_fld *f   = check_ud_buf_fld(luaVM, 1);
	return f->write(luaVM);
}


/**--------------------------------------------------------------------------
 * \brief   tostring representation of a buffer field.
 * \param   luaVM      The lua state.
 * \lparam  xt_buf_fld the buffer field in user_data.
 * \lreturn string     formatted string representing buffer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_buf_fld_tostring (lua_State *luaVM) {
	struct xt_buf_fld *f = check_ud_buf_fld(luaVM, 1);

	f->read(luaVM);
	if (lua_isstring(luaVM, 2))
		lua_pushfstring(luaVM, "Buffer.Field{%s}: %p", lua_tostring(luaVM, 2), f);
	else if (lua_isnumber(luaVM, 2))
		lua_pushfstring(luaVM, "Buffer.Field{%d}: %p", luaL_checkint(luaVM, 2), f);
	else
		lua_pushfstring(luaVM, "Buffer.Field{}: %p", f);
	return 1;
}


/**
 * \brief      the buffer field library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_buf_fld_m [] =
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
int luaopen_buf_fld (lua_State *luaVM)
{
	luaL_newmetatable(luaVM, "L.Buffer.Field");   // stack: functions meta
	luaL_newlib(luaVM, l_buf_fld_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, l_buf_fld_tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack
	// empty IpEndpoint class = {}, this is the actual return of this function
	lua_createtable(luaVM, 0, 0);
	lua_pushinteger(luaVM, BUF_FLD_BIT);
	lua_setfield(luaVM, -2, "BIT");
	lua_pushinteger(luaVM, BUF_FLD_BYTE);
	lua_setfield(luaVM, -2, "BYTE");
	lua_pushinteger(luaVM, BUF_FLD_STR);
	lua_setfield(luaVM, -2, "STRING");
	return 1;
}

