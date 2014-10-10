/**
 * \file     create a packer Combinator
 * \brief    combinators for packers to create structures etc
*/
#include <memory.h>               // memset

#include "l_xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * create an Combinator Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam
 *			... multiple of same type { name = xt.Pack.Int(2) }
 * \lreturn luatable representing a xt.pack.Struct
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_comb_Struct( lua_State *luaVM )
{
	int i;
	lua_newtable( luaVM );
	lua_pushstring( luaVM, "_packer" );  // Stack: ...,Struct,_packer, "_packer"
	lua_newtable( luaVM ); // Stack: ..., Struct, _packer
	for (i=1; i<lua_gettop( luaVM )-2; i++)
	{
		lua_pushnil( luaVM );
		while (lua_next( luaVM, i ))   // Stack: ...,Struct,'_packer',_packer,name,Pack
		{
			lua_pushvalue( luaVM, -2 ); // Stack: ...,Struct,'_packer',_packer,name,Pack,name
			lua_pushvalue( luaVM, -2 ); // Stack: ...,Struct,'_packer',_packer,name,Pack,name,Pack
			// make the copied Packer available as Struct[ 'name' ]
			// pops the copies of Packer and name  off the stack
			lua_rawset( luaVM, -5 );    // Stack: ...,Struct,'_packer',_packer,name,Pack
			// make Packer available in numbered sequence of Struct[ #Struct ]
			// pops the original of the Packer off the Stack
			lua_rawseti( luaVM,  -3, lua_rawlen( luaVM, -3 ) +1 ); // Stack: ...,Struct,'_packer',_packer,name
		}
	}
	luaL_getmetatable( luaVM, "xt.Packer.Combinator" );  // Stack: ...,Struct,xt.Packer.Combinator
	lua_setmetatable( luaVM, i ) ;

	// Stack: ...,Struct,'_packer',_packer
	lua_rawset (luaVM, i );              // Stack: ...,Struct

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
 * read a packer value
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string
 * \return  The # of itemspushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_comb__index( lua_State *luaVM )
{
	const char        *name;
	struct xt_pack    *p;

	// Stack: Struct, name, value
	xt_comb_check_ud( luaVM, -2 );
	name = luaL_checkstring( luaVM, -1 );
	stackDump(luaVM);
	// get the value from the _packer table
	lua_pushstring( luaVM, "_packer" );
	lua_rawget( luaVM, -3);    // Stack: Struct, name, _packer

	lua_pushvalue( luaVM, -2 );    // push the name again
	lua_rawget( luaVM, -2 );    // Stack: Struct, name, _packer, (Packer or internal)
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
	const char        *name;
	struct xt_pack    *p;

	// Stack: Struct, name, value
	xt_comb_check_ud( luaVM, -3 );
	name = luaL_checkstring( luaVM, -2 );
	stackDump(luaVM);
	// Don't overwrite _internal fields
	if ('_' == *name)
		return xt_push_error( luaVM,
			"Can't overwrite or create internal Combinator fields" );

	// get the packer from the _packer table
	lua_pushstring( luaVM, "_packer" );   // Stack: Struct, name, value, '_packer'
	lua_rawget( luaVM, -4 );              // Stack: Struct, name, value,_packer

	lua_pushvalue( luaVM, -3 );    // push the name again
	lua_rawget( luaVM, -2 );       // Stack: Struct, name, value, _packer, (Packer or internal)
	if (lua_isnoneornil( luaVM, -1 ))
		return xt_push_error( luaVM, "Combinator Fields can only be created on construction" );
		
	// grab the packer (if name is numeric or alpha doesn't matter)
	p = xt_pack_check_ud( luaVM, -1 );
	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data structures" );

	xt_pack_write( luaVM, p,  p->b );
	
	return 0;
}


/**--------------------------------------------------------------------------
 * Attach a buffer to a Packer Combinator.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Combinator.
 * \lparam struct xt_buf.
 * \lparam pos    position in xt_buf.
 * \return integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int lxt_comb_attach( lua_State *luaVM )
{
	xt_comb_check_ud( luaVM, 1 );
	struct xt_buf  *b   = xt_buf_check_ud( luaVM, 2 );
	int             pos = luaL_checkint( luaVM, 3 );

	luaL_argcheck( luaVM, 0 <= pos && pos <= (int) b->len, 3,
	                    "xt.Buffer position must be > 0 or < #buffer" );
	lua_setfield( luaVM, 1, "_pos" );
	lua_setfield( luaVM, 1, "_buffer" );
	return 0;
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
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
