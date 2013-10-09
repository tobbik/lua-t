//
//
#include <memory.h>               // memset
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>            // htonl

#include "library.h"
#include "l_byteBuffer.h"


// TODO: this shall be moved into a helper/debug file
void stackDump (lua_State *luaVM) {
	int i;
	int top = lua_gettop(luaVM);
	printf("Items on stack(%d):\t", top);
	for (i = 1; i <= top; i++) {     /* repeat for each level */
		int t = lua_type(luaVM, i);
		switch (t) {

			case LUA_TSTRING:    /* strings */
				printf("`%s'", lua_tostring(luaVM, i));
				break;

			case LUA_TBOOLEAN:   /* booleans */
				printf(lua_toboolean(luaVM, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:    /* numbers */
				printf("%g", lua_tonumber(luaVM, i));
				break;

			default:	            /* other values */
				printf("%s", lua_typename(luaVM, t));
				break;
		}
		printf("\t");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}


// inline helper helper functions
/**
  * \brief  convert lon long (64bit) from network to host and vice versa
  * \param  u_int8 value 64bit integer
  * \return u_int8 endianess corrected integer
  */
static inline u_int8 htonll(u_int8 value)
{
	u_int4 high_part = htonl( (u_int8)(value >> 32) );
	u_int4 low_part  = htonl( (u_int8)(value & 0xFFFFFFFFLL) );
	return (((u_int8)low_part) << 32) | high_part;
}


/**
  * \brief  gets a numeric value in a byteBuffer segment according to mask and
  *         shift
  * \param  u_int8 the position in the byteStream (pointer)
  * \param  u_int8 out_mask  in relation to a 8 byte integer
  * \param  u_int8 out_shift to right end of 64 bit integer
  * \return u_int8
  */
static inline u_int8 get_segment_value_numeric (
		u_int8  *valnum,
		u_int8   out_mask,
		u_int8   out_shift)
{
	return ((htonll (*valnum) & out_mask) >> out_shift);
}


/**
  * \brief  sets a numeric value in a qtc_pfield struct according to mask and
  *         shift
  * \param  lua_State
  * \param  struct qtc_pfield  the field to operate on
  * \param  u_int8 value the value to put into the pointer to the main buffer
  * \return int    that's what gets returned to Lua, meaning the one value on
  *                the stack
  */
static inline void set_segment_value_numeric (
		u_int8  *valnum,
		u_int8   out_mask,
		u_int8   out_shift,
		u_int8   value)
{
	//u_int8 cmpr;
	//cmpr = (64 == a->bits) ?  (u_int8)0x1111111111111111 : ((u_int8) 0x0000000000000001) << a->bits; // 2^bits
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
static int l_new_buffer(lua_State *luaVM)
{
	int                 size;
	size_t              size_bytes;
	struct byteBuffer  *buffer;

	size           = luaL_checkint(luaVM, 1);
	size_bytes     = sizeof(struct byteBuffer) + (size - 1) * sizeof(unsigned char);
	buffer         = (struct byteBuffer *) lua_newuserdata(luaVM, size_bytes);
	buffer->length = size_bytes;
	memset(buffer->buffer, 0, size * sizeof(unsigned char));
	
#if DEBUGHEX==1
	//printf("BUFFER ADDRESS: %p\n", buffer);
#endif

	return 1;
}

/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return pointer to struct byteBuffer
 */
static struct byteBuffer *byteBuffer_check (lua_State *luaVM) {
	void *ud = luaL_checkudata(luaVM, 1, "byteBuffer_instance");
	luaL_argcheck(luaVM, ud != NULL, 1, "`byteBuffer` expected");
	return (struct byteBuffer *) ud;
}


/**
 * \brief  gets the value of the element
 * \param  position in bytes
 * \param  offset   in bits
 * \param  length   in bits
 *
 * \return integer 0 left on the stack
 */
static int l_read_number (lua_State *luaVM) {
	int                length;
	int                pos;
	int                offset;
	u_int8            *valnum;
	u_int8             out_mask;
	u_int8             out_shift;
	struct byteBuffer *buffer   = byteBuffer_check(luaVM);

	// position in byteStream
	pos    = luaL_checkint(luaVM, 2);
	// length in bit
	length = luaL_checkint(luaVM, 3);
	// offset in bit
	offset = luaL_checkint(luaVM, 4);

	out_shift = 64 - (offset%8) - length;
	out_mask = ( 0xFFFFFFFFFFFFFFFF >> (64-length)) << out_shift;

	valnum = (u_int8 *) &(buffer->buffer[ pos ]);
	lua_pushinteger(luaVM, get_segment_value_numeric(valnum, out_mask, out_shift));
	return 1;
}

/**
 * \brief    sets a value  of the field
 * \detail   can set value, inc
 *
 * \return integer 0 left on the stack
 */
static int l_write_number (lua_State *luaVM) {
	int                length;
	int                pos;
	int                offset;
	u_int8            *valnum;
	u_int8             out_mask;
	u_int8             out_shift;
	struct byteBuffer *buffer   = byteBuffer_check(luaVM);

	// position in byteStream
	pos    = luaL_checkint(luaVM, 2);
	// length in bit
	length = luaL_checkint(luaVM, 3);
	// offset in bit
	offset = luaL_checkint(luaVM, 4);

	out_shift = 64 - (offset%8) - length;
	out_mask = ( 0xFFFFFFFFFFFFFFFF >> (64-length)) << out_shift;

	valnum = (u_int8 *) &(buffer->buffer[ pos ]);

	set_segment_value_numeric(
		valnum, out_mask, out_shift,
		(u_int8) luaL_checknumber(luaVM, 3));
	return 0;
}

/**
 * \brief     returns length of the buffer
 * \param     lua state
 * \return    integer   how many elements are placed on the Lua stack
*/
static int l_get_length(lua_State *luaVM)
{
	struct byteBuffer *buffer;

	buffer   = byteBuffer_check(luaVM);
	lua_pushinteger(luaVM, (int) buffer->length);

	return 1;
}


/**
 * \brief     convert 8bit integer to BCD
 * \param     val  8bit integer
 * \return    8bit integer encoding of a 2 digit BCD number
 */
int l_byteToBcd(lua_State *luaVM)
{
	u_int1 val = luaL_checkint(luaVM, 1);
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
	u_int2 val = luaL_checkint(luaVM, 1);
	lua_pushinteger(luaVM,
			(val/1000   *4096 ) +
		 ( (val/100%10)* 256 ) +
		 ( (val/10%10) * 16 ) +
			(val%10)
	);
	return 1;
}

static const luaL_Reg byteBuffer_lib [] =
{
	{"new",      l_new_buffer},
	{"readAt",   l_read_number},
	{"writeAt",  l_write_number},
	{"length",   l_get_length},
	{NULL,        NULL}
};

LUAMOD_API
int luaopen_byteBuffer (lua_State *luaVM)
{
	luaL_newlib (luaVM, byteBuffer_lib);
	return 1;
}



///**
// * \brief    the metatble for the Module
// */
//static const struct luaL_Reg byteBuffer_fm [] = {
//	{"__call",      l_new_buffer},
//	{NULL,          NULL}
//};
//
////static const struct luaL_Reg byteBuffer_m [] = {
////	{"__tostring",  l_grt_string},
////	{"__index",     l_get_index},
////	{"__newindex",  l_set_index},
////	{"__len",       l_get_size},
////	{NULL,          NULL}
////};
//
//LUAMOD_API
//int luaopen_qtc_pfield(lua_State *luaVM) {
//	luaL_newmetatable(luaVM, "byteBuffer_instance");   // stack: functions meta
//	//luaL_setfuncs(luaVM, byteBuffer_m, 0);
//	lua_pushcfunction(luaVM, l_get_index);
//	lua_setfield(luaVM, -2, "__index");
//	lua_pushcfunction(luaVM, l_set_index);
//	lua_setfield(luaVM, -2, "__newindex");
//	lua_pushcfunction(luaVM, l_get_length);
//	lua_setfield(luaVM, -2, "__len");
//	lua_pushcfunction(luaVM, l_get_string);
//	lua_setfield(luaVM, -2, "__tostring");
//	lua_pop(luaVM, 1);
//	// empty byteBuffer class = {}
//	lua_createtable(luaVM, 0, 0);
//	// metatable = {}
//	luaL_newmetatable(luaVM, "byteBuffer_class");
//	luaL_setfuncs(luaVM, byteBuffer_fm, 0);
//	lua_setmetatable(luaVM, -2);
//	return 1;
//}
