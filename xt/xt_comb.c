/**
 * \file     create a packer Combinator
 * \brief    combinators for packers to create structures etc
*/
#include <memory.h>               // memset

#include "l_xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * create an Combinator Struct/Sequence Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam
 *			... multiple of type table { name = xt.Packer }
 *			             or type       xt.Packer
 * \lreturn luatable representing a xt.pack.Struct
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_comb_Struct( lua_State *luaVM )
{
	int             i;      ///< iterator for going through the arguments
	size_t          sz=0;   ///< tally up the size of all elements in the Struct
	size_t          bc=0;   ///< count bitSize for bitSize fields
	struct xt_pack *p;
	int             retVal;

	lua_newtable( luaVM );
	lua_pushstring( luaVM, "_packer" );  // Stack: ...,Struct,"_packer"
	lua_newtable( luaVM ); // Stack: ..., Struct,"_packer",_packer
	for (i=1; i<lua_gettop( luaVM )-2; i++)
	{
		// if element is table -> assume key/value pair
		if (lua_istable( luaVM, i))
		{
			lua_pushnil( luaVM );
			if (lua_next( luaVM, i ))   // Stack: ...,Struct,"_packer",_packer,name,Pack
			{
				// check if Packer or Struct and pushes copy on stack
				if ((retVal = xt_comb_test_ud( luaVM, -1 )) != 0)
				{
					lua_pop( luaVM, 1 );
					return retVal;
				}
				p = lua_touserdata( luaVM, -1 );

				// Stack: ...,Struct,'_packer',_packer,name,Pack,Pack
				lua_insert( luaVM, -3 );    // Stack: ...,Struct,'_packer',_packer,Pack,name,Pack
				// make the copied Packer available as Struct[ 'name' ]
				// pops the copies of Packer and name  off the stack
				lua_rawset( luaVM, -4 );    // Stack: ...,Struct,'_packer',_packer,name,Pack
			}
			else
			{
				return xt_push_error( luaVM, "the table argument must contain one key/value pair.");
			}

			// make Packer available in numbered sequence of Struct[ #Struct ]
			// pops the original of the Packer off the Stack
			lua_rawseti( luaVM,  -2, lua_rawlen( luaVM, -2 ) +1 ); // Stack: ...,Struct,'_packer',_packer
		}
		else
		{
			// check if Packer or Struct and pushes copy on stack
			if ((retVal = xt_comb_test_ud( luaVM, i )) != 0)
			{
				lua_pop( luaVM, 1 );
				return retVal;
			}
			p = lua_touserdata( luaVM, -1 );
			// Stack: ...,Struct,'_packer',_packer, Pack
			// make Packer available in numbered sequence of Struct[ #Struct ]
			// pops the copy of the Packer off the Stack
			lua_rawseti( luaVM,  -2, lua_rawlen( luaVM, -2 ) +1 ); // Stack: ...,Struct,'_packer',_packer
		}
		// handle Bit type packers
		if (NULL != p && XT_PACK_BIT == p->type)
		{
			p->bofs = bc%8;
			if ((bc+p->blen)/8 > bc/8)
				sz += 1;
			bc = bc + p->blen;
		}
		if (NULL != p && XT_PACK_BIT != p->type)
		{
			sz = sz + p->sz;
			if (bc%8)
				xt_push_error( luaVM, "bitsized fields must always be grouped by byte size " );
			else
				bc = 0;
		}
	}
	luaL_getmetatable( luaVM, "xt.Packer.Combinator" ); // Stack: ...,Struct,xt.Packer.Combinator
	lua_setmetatable( luaVM, i ) ;

	// Stack: ...,Struct,'_packer',_packer
	lua_rawset( luaVM, i );              // Stack: ...,Struct
	lua_pushstring( luaVM, "_size" );    // Stack: ...,Struct,"_size"
	lua_pushinteger( luaVM, sz );        // Stack: ...,Struct,"_size",sz
	lua_rawset( luaVM, i );              // Stack: ...,Struct

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a Test
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \lparam  the Test table on the stack
 * \lreturn leaves the test table on the stack
 * --------------------------------------------------------------------------*/
void xt_comb_check_ud( lua_State *luaVM, int pos )
{
	luaL_checktype( luaVM, pos, LUA_TTABLE );
   if (lua_getmetatable( luaVM, pos ))                     // does it have a metatable
	{
		luaL_getmetatable( luaVM, "xt.Packer.Combinator" );  // get correct metatable 
		if (! lua_rawequal( luaVM, -1, -2 ))                 // not the same? 
			xt_push_error (luaVM, "wrong argument, `xt.Packer.Combinator` expected");
		lua_pop( luaVM, 2 );                                // pop the 2 metatables
	}
	else
		xt_push_error( luaVM, "wrong argument, `xt.Packer.Combinator` expected" );
}


/**--------------------------------------------------------------------------
 * \brief   test a value on the stack for being a Cobinator or Packer
 * \param   luaVM    The lua state.
 * \param   int      Boolean 0:success, 1:error
 * \lparam  the Test table on the stack
 * \lreturn leaves the tested element on the the stack
 * --------------------------------------------------------------------------*/
int xt_comb_test_ud( lua_State *luaVM, int pos )
{
	void *ud = lua_touserdata( luaVM, pos );
	if (lua_isuserdata( luaVM, pos ) && NULL != ud)
	{
		ud = luaL_checkudata( luaVM, pos, "xt.Packer" );
		lua_pushvalue( luaVM, pos );
		return 0;
	}
	
	if (lua_istable( luaVM, pos ) && lua_getmetatable( luaVM, pos )) // does it have a metatable
	{
		luaL_getmetatable( luaVM, "xt.Packer.Combinator" );  // get correct metatable 
		if (lua_rawequal( luaVM, -1, -2 ))                   // are metatables the same?
		{
			lua_pop( luaVM, 2 );                              // pop the 2 metatables
			lua_pushvalue( luaVM, pos );
			return 0;
		}
		lua_pop( luaVM, 1 );                                 // pop the xt.Packer.Combinator metatables
		luaL_getmetatable( luaVM, "xt.Packer.Array" );       // get correct metatable 
		if (lua_rawequal( luaVM, -1, -2 ))                   // are metatables the same?
		{
			lua_pop( luaVM, 2 );                              // pop the 2 metatables
			lua_pushvalue( luaVM, pos );
			return 0;
		}
	}
	lua_pop( luaVM, 2 );                                   // pop the 2 metatables
	return xt_push_error( luaVM, "`xt.Packer, xt.Packer.Combinator or xt.Packer.Array` expected" );
}


/**--------------------------------------------------------------------------
 * read a packer value
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string
 * \return  The # of itemspushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_comb__index( lua_State *luaVM )
{
	struct xt_pack    *p;

	// Stack: Struct, name, value
	xt_comb_check_ud( luaVM, -2 );
	luaL_checkstring( luaVM, -1 );
	// get the value from the _packer table
	lua_pushstring( luaVM, "_packer" );
	lua_rawget( luaVM, -3);    // Stack: Struct, name, _packer

	// Access the _packer table by key/value or numeric index
	if (0 == lua_tonumber( luaVM, -2 ))
	{
		lua_pushvalue( luaVM, -2 );    // push the name again
		lua_rawget( luaVM, -2 );       // Stack: Struct, name, _packer, (Packer or internal)
	}
	else
	{
		lua_rawgeti( luaVM, -1, lua_tonumber( luaVM, -2 )); // Stack: Struct, name, _packer, (Packer or internal)
	}

	if (lua_isnoneornil( luaVM, -1 ))
		return 1;

	p = xt_pack_check_ud( luaVM, -1 );

	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data structures" );

	return xt_pack_read( luaVM, p, p->b );
}


/**--------------------------------------------------------------------------
 * update a packer value
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string
 * \lparam  value LuaType
 * \return  The # of itemspushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_comb__newindex( lua_State *luaVM )
{
	struct xt_pack    *p;

	// Stack: Struct, name, value
	xt_comb_check_ud( luaVM, -3 );
	// Don't overwrite _internal fields
	if ('_' == *(luaL_checkstring( luaVM, -2 )))
		return xt_push_error( luaVM,
			"Can't overwrite or create internal Combinator fields" );

	// get the packer from the _packer table
	lua_pushstring( luaVM, "_packer" );   // Stack: Struct, name, value, '_packer'
	lua_rawget( luaVM, -4 );              // Stack: Struct, name, value,_packer

	// Access the _packer table by key/value or numeric index
	if (0 == lua_tonumber( luaVM, -2 ))
	{
		lua_pushvalue( luaVM, -3 );    // push the name again
		lua_rawget( luaVM, -2 );       // Stack: Struct, name, value, _packer, (Packer or internal)
	}
	else
	{
		lua_rawgeti( luaVM, -1, lua_tonumber( luaVM, -3 )); // Stack: Struct,name,value,_packer,(Packer or internal)
	}

	if (lua_isnoneornil( luaVM, -1 ))
		return xt_push_error( luaVM, "Combinator Fields can only be created on construction" );
		
	p = xt_pack_check_ud( luaVM, -1 );
	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data structures" );
	lua_pushvalue( luaVM, -3 );    // push value to end of stack where xt_pack_write expects it

	xt_pack_write( luaVM, p,  p->b );
	
	return 0;
}


/**--------------------------------------------------------------------------
 * Attach a buffer to a Packer Combinator.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Combinator.
 * \lparam struct xt_buf.
 * \lparam pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int lxt_comb__call( lua_State *luaVM )
{
	xt_comb_check_ud( luaVM, 1 );
	struct xt_buf  *b   = xt_buf_check_ud( luaVM, 2 );
	struct xt_pack *p;
	int             pos = luaL_checkint( luaVM, 3 );
	size_t          i;       ///< the iterator for all fields
	size_t          sz = 0;  ///< iterate over the size to assign proper buffer position 

	luaL_argcheck( luaVM, 0 <= pos && pos <= (int) b->len, 3,
	                    "xt.Buffer position must be > 0 or < #buffer" );
	// TODO: Buffer size must equal Struct size?
	lua_pushstring( luaVM, "_pos" );
	lua_insert( luaVM, -2);
	lua_rawset( luaVM, 1 );
	lua_pushstring( luaVM, "_buffer" );
	lua_insert( luaVM, -2);
	lua_rawset( luaVM, 1 );
	// get the _packer subtable
	lua_pushstring( luaVM, "_packer" );   // Stack: Struct, name, value, '_packer'
	lua_rawget( luaVM, 1 );               // Stack: Struct, name, value,_packer
	for (i=1; i < lua_rawlen( luaVM, 2 )+1 ; i++)
	{
		lua_rawgeti( luaVM, -1, i );
		p = xt_pack_check_ud( luaVM, -1 );
		p->b = &(b->b[ sz ]);
		sz += p->sz;
		lua_pop( luaVM, 1 );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Dissassociates buffers from Packers.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Combinator.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_comb__gc( lua_State *luaVM )
{
	xt_comb_check_ud( luaVM, 1 );
	struct xt_pack *p;
	size_t          i;       ///< the iterator for all fields

	// get the _packer subtable
	lua_pushstring( luaVM, "_packer" );   // Stack: Struct,'_packer'
	lua_rawget( luaVM, 1 );               // Stack: Struct,_packer
	for (i=1; i < lua_rawlen( luaVM, -1 )+1 ; i++)
	{
		lua_rawgeti( luaVM, -1, i );
		p = xt_pack_check_ud( luaVM, -1 );
		p->b = NULL;
		lua_pop( luaVM, 1 );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Length of Struct in bytes
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Combinator.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_comb__len( lua_State *luaVM )
{
	xt_comb_check_ud( luaVM, 1 );

	// get the _packer subtable
	lua_pushstring( luaVM, "_size" );   // Stack: Struct,'_packer'
	lua_rawget( luaVM, 1 );               // Stack: Struct,_packer
	return 1;
}



/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Packer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_comb( lua_State *luaVM )
{
	// xt.Pack instance metatable
	luaL_newmetatable( luaVM, "xt.Packer.Combinator" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_comb__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_comb__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_comb__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lxt_comb__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lxt_comb__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
