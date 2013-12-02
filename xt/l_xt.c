/**
 * \file   library.c
 *         a couple of helper files used throughout all library files
 */
#include <stdio.h>
#include <string.h>     // strerror
#include <errno.h>      // errno

#include "l_xt.h"
#include "l_xt_hndl.h"



/**
 * \brief  Prints a list of items on the lua stack.
 * \param  luaVM The Lua state.
 */
void stackDump (lua_State *luaVM) {
	int i;
	int top = lua_gettop(luaVM);
	printf("LUA STACK[%d]:\t", top);
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


/**
 * \brief  Return an error string to the LUA script.
 *         Basically luaL_error extended by errno support
 * \param  luaVM The Lua intepretter object.
 * \param  info  Error string.
 * \param  ...   variable arguments to fmt
 */
int xt_push_error(lua_State *luaVM, const char *fmt, ...)
{
	va_list argp;
	//lua_pushnil (luaVM);
	if (NULL==fmt) {
		if (0==errno) return luaL_error (luaVM, strerror(errno));
		else          return luaL_error (luaVM, "Unknown Error");
	}
	else {
		va_start(argp, fmt);
		luaL_where(luaVM, 1);
		lua_pushvfstring(luaVM, fmt, argp);
		va_end(argp);
		if (0==errno) lua_pushstring (luaVM, "\n");
		else          lua_pushfstring (luaVM, ": %s\n", strerror(errno) );
		lua_concat(luaVM, 3);
		return lua_error(luaVM);
	}
}


/**
 * \brief     Used to reverse the order of bytes for a 16 bit unsigned integer
 *
 * \param     value Unsigned 16 bit integer
 * \return    Integer with the opposite Endianness
 */
inline uint16_t Reverse2Bytes(uint16_t value)
{
	return (
		(value & 0xFFU) << 8 |
		(value & 0xFF00U) >> 8
	);
}


/**
 * \brief     Used to reverse the order of bytes for a 32 bit unsigned integer
 *
 * \param     value Unsigned 32 bit integer
 * \return    integer with the opposite Endianness
 */
 inline uint32_t Reverse4Bytes(uint32_t value)
{
	return (value & 0x000000FFU) << 24 |
			 (value & 0x0000FF00U) << 8  |
			 (value & 0x00FF0000U) >> 8  |
			 (value & 0xFF000000U) >> 24;
}


/**
 * \brief   Used to reverse the order of bytes for a 64 bit unsigned integer
 *
 * \param   value Unsigned 64 bit integer
 * \return  Integer with the opposite Endianness
 */
 inline uint64_t Reverse8Bytes(uint64_t value)
{
	return (value & 0x00000000000000FFUL) << 56 |
			 (value & 0x000000000000FF00UL) << 40 |
			 (value & 0x0000000000FF0000UL) << 24 |
			 (value & 0x00000000FF000000UL) << 8  |
			 (value & 0x000000FF00000000UL) >> 8  |
			 (value & 0x0000FF0000000000UL) >> 24 |
			 (value & 0x00FF000000000000UL) >> 40 |
			 (value & 0xFF00000000000000UL) >> 56;
}


/**
 * \brief    static table for the CRC16 calculation
 * \detail   based on polynimial 0xA001

static void crc16table (uint8_t hex) {
	const uint16_t crc16poly = 0xA001;
	uint16_t value;
	uint16_t temp,i;
	uint8_t  j;
	printf("static const uint16_t CRC16TABLE[] = {\n\t"); 
	for(i = 0; i < 256; ++i) {
		value = 0;
		temp = i;
		for(j = 0; j < 8; ++j) {
			if(((value ^ temp) & 0x0001) != 0) {
				value = (uint16_t) ((value >> 1) ^ crc16poly);
			}else {
				value >>= 1;
			}
			temp >>= 1;
		}
		if (hex) printf("0x%04X, ", value);
		else     printf("%5d, ",  value);
		if (0 == ((i+1)%8)) printf("\n\t");
	}
	printf("}\n"); 
}
* 
 */
static const uint16_t CRC16TABLE[]  = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241 ,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440 ,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40 ,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841 ,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40 ,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41 ,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641 ,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040 ,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240 ,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441 ,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41 ,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840 ,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41 ,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40 ,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640 ,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041 ,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240 ,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441 ,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41 ,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840 ,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41 ,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40 ,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640 ,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041 ,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241 ,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440 ,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40 ,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841 ,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40 ,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41 ,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641 ,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 
};

/**
 * \brief       calculate the CRC16 checksum
 * \detail      calculate over the payload and sticks it to the end of the buffer
 * \param       char buffer *data  data including the needed extra (2) bytes
 * \param       size_t size  how much of data is the payload
 * \return      the 16bit CRC16 checksum
 */
uint16_t get_crc16 (const unsigned char *data, size_t length)
{
	uint16_t    crc = 0;
	uint16_t    index;
	size_t      i;
	for(i = 0; i < length; ++i)
	{
		index = (uint8_t)  ( crc ^ data[i]);
		crc   = (uint16_t) ((crc >> 8) ^ CRC16TABLE [index]);
	}
	return crc;
}


/** -------------------------------------------------------------------------
 * \brief   Helper to take sockets from Lua tables to FD_SET
 *          Itertates over the table puls out the socket structs and adds the
 *          actual sockets to the fd_set
 * \param   luaVM  The lua state.
 * \param   int    position on stack where table is located
 * \param  *fd_set the set of sockets(fd) to be filled
 * \param  *int    the maximum socket(fd) value
 * \return  void.
 *-------------------------------------------------------------------------*/
void make_fdset(lua_State *luaVM, int stack_pos, fd_set *collection, int *max_hndl)
{
	struct xt_hndl  *hndl;

	FD_ZERO(collection);
	// empty table == nil
	if (lua_isnil(luaVM, stack_pos)) return;
	// only accept tables
	luaL_checktype(luaVM, stack_pos, LUA_TTABLE);

	// adding fh to FD_SETs
	lua_pushnil(luaVM);
	while (lua_next(luaVM, 1))
	{
		hndl = (struct xt_hndl*) lua_touserdata(luaVM, -1);
		FD_SET( hndl->fd, collection );
		*max_hndl = (hndl->fd > *max_hndl) ? hndl->fd : *max_hndl;
		lua_pop(luaVM, 1);   // remove the socket, keep key for next()
	}
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   luaVM  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int l_select_handle(lua_State *luaVM)
{
	fd_set           rfds, wfds;
	struct xt_hndl  *hndl;
	int              rnum, wnum, readsocks, i/*, rp*/;
	int              rset=1;//, wset=1;

	wnum = -1;
	rnum = -1;
	make_fdset(luaVM, 1, &rfds, &rnum);
	//make_fdset(luaVM, 2, &wfds, &wnum);
	FD_ZERO(&wfds);

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable(luaVM, 0, 0);     // create result table
	//TODO: check if readsocks hit 0 and break cuz we are done
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti(luaVM, 1, i);
		// in table this is when the last index is found
		if ( lua_isnil(luaVM, -1) )
		{
			lua_pop(luaVM, 1);
			break;
		}
		hndl = (struct xt_hndl*) lua_touserdata(luaVM, -1);
		if FD_ISSET( hndl->fd, &rfds)
		{
			lua_rawseti(luaVM, -2, rset++);
			readsocks--;
			if (0 == readsocks) {
				break;
			}
		}
		else {
			lua_pop(luaVM, 1);
		}
	}
	return (1);
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   luaVM  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int l_select_handle_k(lua_State *luaVM)
{
	fd_set           rfds, wfds;
	struct xt_hndl  *hndl;
	int              rnum, wnum, readsocks /*, rp*/;

	wnum = -1;
	rnum = -1;
	make_fdset(luaVM, 1, &rfds, &rnum);
	//make_fdset(luaVM, 2, &wfds, &wnum);
	FD_ZERO(&wfds);

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable(luaVM, 0, 0);     // create result table
	//lua_createtable(luaVM, 0, 0);
	lua_pushnil(luaVM);
	while (lua_next(luaVM, 1))
	{
		hndl = (struct xt_hndl*) lua_touserdata(luaVM, -1);
		if FD_ISSET( hndl->fd, &rfds)
		{
			//TODO: Find better way to preserve key for next iteration
			lua_pushvalue(luaVM, -2);
			lua_pushvalue(luaVM, -2);
			lua_rawset(luaVM, -5);
			if (0 == --readsocks) {
				lua_pop(luaVM, 2);
				break;
			}
		}
		lua_pop(luaVM, 1);
	}
	return (1);
}


/**
 * \brief      the (empty) xt library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_xt_lib [] =
{
	// xt-global methods
	{"select",      l_select_handle},
	{"selectK",     l_select_handle_k},
	{NULL,        NULL}
};


/**
 * \brief     Export the xt library to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_xt (lua_State *luaVM)
{
	luaL_newlib (luaVM, l_xt_lib);
	luaopen_xt_time(luaVM);
	lua_setfield(luaVM, -2, "Time");
	luaopen_socket(luaVM);
	lua_setfield(luaVM, -2, "Socket");
	luaopen_ipendpoint(luaVM);
	lua_setfield(luaVM, -2, "IpEndpoint");
	luaopen_xt_buf(luaVM);
	lua_setfield(luaVM, -2, "Buffer");
	luaopen_xt_enc(luaVM);
	lua_setfield(luaVM, -2, "Encode");
	luaopen_debug(luaVM);
	lua_setfield(luaVM, -2, "debug");
	return 1;
}

