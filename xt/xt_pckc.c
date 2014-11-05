/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_pckc.c
 * \brief     create a packer Struct/Array
 *            Combinators for packers to create structures and arrays. This is
 *            interleved with xt.Pack.Reader functionality (xt_packr) becuase a
 *            lot of logic can be shared
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */


#include "xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * Create a  xt.Pack.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  xt.Pack.
 * \lreturn userdata of xt.Pack.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pckc_Struct( lua_State *luaVM )
{
	size_t             i;      ///< iterator for going through the arguments
	size_t             bc=0;   ///< count bitSize for bit type packers
	struct xt_pck     *p;      ///< temporary packer/struct for iteration
	struct xt_pck     *cp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	cp     = (struct xt_pck *) lua_newuserdata( luaVM, sizeof( struct xt_pck ) );
	cp->n  = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the Struct userdata)
	cp->sz = 0;
	cp->t  = XT_PCK_STRUCT;

	// create index table
	lua_createtable( luaVM, cp->n, cp->n ); // Stack: ..., Struct,idx
	// create offset table
	lua_createtable( luaVM, cp->n, cp->n ); // Stack: ..., Struct,idx,ofs

	for (i=1; i<=cp->n; i++)
	{
		// are we creating a named element?
		if (lua_istable( luaVM, i))
		{
			// Stack gymnastic:
			lua_pushnil( luaVM );
			if (!lua_next( luaVM, i ))         // Stack: ...,Struct,idx,ofs,name,Pack
				return xt_push_error( luaVM, "the table argument must contain one key/value pair.");
			// TODO: check if name is already used!
		}
		else
		{
			lua_pushfstring( luaVM, "%d", i ); // Stack: ...,Struct,idx,ofs,"i"
			lua_pushvalue( luaVM, i );         // Stack: ...,Struct,idx,ofs,"i",Pack
		}
		// populate idx and the ofs table
		lua_insert( luaVM, -2);           // Stack: ...,Struct,idx,ofs,Pack,name             swap pack/name
		lua_pushvalue( luaVM, -1);        // Stack: ...,Struct,idx,ofs,Pack,name,name
		lua_rawseti( luaVM, -5, i );      // Stack: ...,Struct,idx,ofs,Pack,name             idx[i]=name
		lua_pushinteger( luaVM, cp->sz);  // Stack: ...,Struct,idx,ofs,Pack,name,offset
		lua_rawseti( luaVM, -4, i );      // Stack: ...,Struct,idx,ofs,Pack,name             ofs[i]=offset
		lua_pushvalue( luaVM, -1);        // Stack: ...,Struct,idx,ofs,Pack,name,name
		lua_pushinteger( luaVM, i);       // Stack: ...,Struct,idx,ofs,Pack,name,name,index
		lua_rawset( luaVM, -5 );          // Stack: ...,Struct,idx,ofs,Pack,name             ofs[name]=i
		lua_pushvalue( luaVM, -2);        // Stack: ...,Struct,idx,ofs,Pack,name,Pack
		lua_rawset( luaVM, -5 );          // Stack: ...,Struct,idx,ofs,Pack                  idx[name]=Pack
		// Stack: ...,Struct,idx,Pack/Struct
		p = xt_pckc_check_ud( luaVM, -1, 1 );    // allow xt.Pack or xt.Pack.Struct
		// handle Bit type packers
		if (XT_PCK_BIT == p->t)
		{
			p->oB = bc%8;
			if ((bc + p->lB)/8 > bc/8)
					  cp->sz += 1;
			bc = bc + p->lB;
		}
		else
		{
			if (bc%8)
					  xt_push_error( luaVM, "bitsized fields must always be grouped by byte size" );
			else   
					  bc = 0;
			cp->sz += p->sz;
		}
		lua_pop( luaVM, 1 );   // pop packer from stack for next round
	}
	cp->oR = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register offset table
	cp->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "xt.Pack.Struct" ); // Stack: ...,xt.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  xt.Pack.Array Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  type identifier  xt.Pack, xt.Pack.Struct, xt.Pack.Array .
 * \lparam  len              Number of elements in the array.
 * \lreturn userdata of xt.Pack.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pckc_Array( lua_State *luaVM )
{
	struct xt_pck     *p;      ///< packer
	struct xt_pck     *ap;     ///< array userdata to be created

	p = xt_pckc_check_ud( luaVM, -2, 1 );    // allow x.Pack or xt.Pack.Struct
	ap     = (struct xt_pck *) lua_newuserdata( luaVM, sizeof( struct xt_pck ) );
	ap->n  = luaL_checkint( luaVM, -2 );       // how many elements in the array
	ap->sz = ap->n * sizeof( p->sz );
	ap->t  = XT_PCK_ARRAY;

	luaL_getmetatable( luaVM, "xt.Pack.Struct" );
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being an xt.Pack OR * xt.Pack.Struct/Array
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \param   int      check -> treats as check -> erros if fail
 * \lparam  userdata xt.Pack.Struct on the stack.
 * \return  xt_pck_s pointer.
 * --------------------------------------------------------------------------*/
struct xt_pck *xt_pckc_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "xt.Pack.Struct" );
	if (NULL != ud)
		return (struct xt_pck *) ud;
	ud = luaL_testudata( luaVM, pos, "xt.Pack" );
	if (NULL != ud)
		return (struct xt_pck *) ud;
	if (check)
		luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Pack.Struct` or `xt.Pack` expected" );
	return NULL;
}


/**--------------------------------------------------------------------------
 * create a xt_pckr and push to LuaStack.
 * \param   luaVM  The lua state.
 * \param   xt_pck Packer reference.
 * \param   buffer offset.
 *
 * \return  struct xt_pckr*  pointer to the  xt_pckr struct
 * --------------------------------------------------------------------------*/
struct xt_pckr *xt_pckr_create_ud( lua_State *luaVM, struct xt_pck *p, size_t o )
{
	struct xt_pckr  *pr;
	pr = (struct xt_pckr *) lua_newuserdata( luaVM, sizeof( struct xt_pckr ));

	pr->p  = p;
	pr->o  = o;
	luaL_getmetatable( luaVM, "xt.Pack.Reader" );
	lua_setmetatable( luaVM, -2 );
	return pr;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being an xt.Pack.Result
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \param   int      check -> treats as check -> erros if fail
 * \lparam  userdata xt.Pack.Result on the stack.
 * \return  xt_pck_s pointer.
 * --------------------------------------------------------------------------*/
struct xt_pckr *xt_pckr_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "xt.Pack.Reader" );
	if (check)
		luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Pack.Reader` expected" );
	return (NULL==ud) ? NULL : ((struct xt_pckr *) ud);
}


/**--------------------------------------------------------------------------
 * Read a Struct packer value.
 *          This can not simply return a packer/Struct type since it now has
 *          meta information about the position it is requested from.  For this
 *          the is a new datatype xt.Pack.Result which carries type and position
 *          information
 * \param   luaVM    The lua state.
 * \lparam  userdata xt.Pack.Struct instance.
 * \lparam  key      string/integer.
 * \lreturn userdata Pack or Struct instance.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pckrc__index( lua_State *luaVM )
{
	struct xt_pckr *pr  = xt_pckr_check_ud( luaVM, -2, 0 );
	struct xt_pck  *pc  = (NULL == pr) ? xt_pckc_check_ud( luaVM, -2, 1 ) : pr->p;
	struct xt_pck  *p;
	int             pos = (NULL == pr) ? 0 : pr->o;

	// get idx table (struct) or packer type (array)
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->iR );
	// Stack: Struct,idx/name,idx/Packer
	if (LUA_TUSERDATA == lua_type( luaVM, -1 ))        // xt.Array
	{
		if (luaL_checkint( luaVM, -2 ) > (int) pc->n)             // Array out of bound: return nil
		{
			lua_pushnil( luaVM );
			return 1;
		}
		else
		{
			p = xt_pckc_check_ud( luaVM, -1, 0 );
			pos += (p->sz * luaL_checkint( luaVM, -2 ));
		}
	}
	else                                              // xt.Struct
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->oR );          // Stack: Struct,nm,idx,ofs
		if (lua_tonumber( luaVM, -3 ))
		{
			lua_rawgeti( luaVM, -1, lua_tointeger( luaVM, -3 ) );  // Stack: Struct,id,idx,ofs,pos
			pos += luaL_checkint( luaVM, -1);
			lua_pop( luaVM, 2 );                                   // Stack: Struct,id,idx
			lua_rawgeti( luaVM, -1, lua_tointeger( luaVM, -2 ) );  // Stack: Struct,id,idx,name
		}
		else
		{
			lua_pushvalue( luaVM, -3 );                            // Stack: Struct,name,idx,ofs,name
			lua_rawget( luaVM, -2 );                               // Stack: Struct,name,idx,ofs,id
			lua_rawgeti( luaVM, -2, lua_tointeger( luaVM, -1 ) );  // Stack: Struct,name,idx,ofs,id,pos
			pos += luaL_checkint( luaVM, -1);                       // Stack: Struct,name,idx,ofs,id,pos
			lua_pop( luaVM, 3 );                                   // Stack: Struct,name,idx
			lua_pushvalue( luaVM, -2 );                            // Stack: Struct,name,idx,name
		}
		lua_rawget( luaVM, -2 );                                  // Stack: Struct,name,idx,Pack
	}

	xt_pckr_create_ud( luaVM, xt_pckc_check_ud( luaVM, -1, 0 ), pos );
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an xt.Pack.Struct.
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pckrc__newindex( lua_State *luaVM )
{
	struct xt_pckr *pr  = xt_pckr_check_ud( luaVM, -3, 0 );

	(NULL == pr) ? xt_pckc_check_ud( luaVM, -3, 1 ) : pr->p;

	return xt_push_error( luaVM, "Packers are static and can't be updated!" );
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  luaVM lua Virtual Machine.
 * \lparam ud    xt.Pack.Struct.
 * \return int   # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pckc__gc( lua_State *luaVM )
{
	struct xt_pck *cp = xt_pckc_check_ud( luaVM, 1, 1 );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, cp->iR );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, cp->oR );
	return 0;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a Struct/Reader instance.
 * \param   luaVM  lua Virtual Machine.
 * \lparam  ud     xt.Pack.Struct/Reader instance.
 * \lreturn int    # of elements in xt.Pack.Struct/Reader instance.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pckrc__len( lua_State *luaVM )
{
	struct xt_pckr *pr = xt_pckr_check_ud( luaVM, 1, 0 );
	struct xt_pck  *pc = (NULL == pr) ? xt_pckc_check_ud( luaVM, -1, 1 ) : pr->p;

	lua_pushinteger( luaVM, pc->n );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an xt.Pack.Reader instance.
 *          This is used to either read from or write to a string or xt.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  ud        xt.Pack.Reader instance.
 * \lparam  ud,string xt.Buffer or Lua string.
 * \lparam   xt.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to xt.Pack.Reader.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pckr__call( lua_State *luaVM );    // declaration for recursive call
static int lxt_pckr__call( lua_State *luaVM )
{
	struct xt_pckr *pr  = xt_pckr_check_ud( luaVM, 1, 1 );
	struct xt_buf  *buf;
	unsigned char  *b;
	size_t          l;                   /// length of string or buffer overall
	size_t          n;                   /// iterator for complex types

	luaL_argcheck( luaVM,  2<=lua_gettop(luaVM) && lua_gettop( luaVM )<=3,
		2, "Calling an xt.Pack.Reader takes 2 or 3 arguments!");
	if (lua_isuserdata( luaVM, 2 ))      // xt.Buffer
	{
		buf = xt_buf_check_ud ( luaVM, 2 );
		luaL_argcheck( luaVM,  buf->len > pr->o+pr->p->sz,
		2, "The length of the Buffer must be longer than Pack offset plus Pack length.");
		b   =  &(buf->b[ pr->o ]);
	}
	else
	{
		b   = (unsigned char *) luaL_checklstring( luaVM, 2, &l );
		luaL_argcheck( luaVM,  l > pr->o+pr->p->sz,
		2, "The length of the Buffer must be longer than Pack offset plus Pack length.");
		luaL_argcheck( luaVM,  2 == lua_gettop( luaVM ), 2
		2, "Can't write to a Lua String since they are immutable.");
		b   =  b+pr->o;
	}

	if (2 == lua_gettop( luaVM ))     // read from input
	{
		if (pr->p->t < XT_PCK_STRUCT)      // handle atomic packer, return single value
		{
			return xt_pck_read( luaVM, pr->p, (const unsigned char *) b );
		}
		if (pr->p->t == XT_PCK_STRUCT)      // handle Struct, return table
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pr->p->iR ); // get index table
			lua_createtable( luaVM, 0, pr->p->n);   //Stack: r,buf,idx,res
			for (n=1; n<pr->p->n; n++)
			{
				lua_pushcfunction( luaVM, lxt_pckr__call );   //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lxt_pckrc__index ); //Stack: r,buf,idx,res,_call,__index
				lua_pushvalue( luaVM, -6 );          //Stack: r,buf,idx,res,__call,__index,r
				lua_rawgeti( luaVM, -5, n );         //Stack: r,buf,idx,res,__call,__index,r,name
				lua_pushvalue( luaVM, -1 );          //Stack: r,buf,idx,res,__call,__index,r,name,name
				lua_insert( luaVM, -5 );             //Stack: r,buf,idx,res,name,__call,__index,r,name
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,name,__call,value
				lua_pushvalue( luaVM, -6 );          //Stack: r,buf,idx,res,name,__call,value,buf
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,name,__call,value,buf
				lua_rawset( luaVM, -3 );             //Stack: r,buf,idx,res
			}
			lua_remove( luaVM, -2 );                //Stack: r,buf,res
			return 1;
		}
		if (pr->p->t == XT_PCK_ARRAY)      // handle Array; return table
		{
			lua_createtable( luaVM, 0, pr->p->n);   //Stack: r,buf,idx,res
			for (n=1; n<pr->p->n; n++)
			{
				lua_pushcfunction( luaVM, lxt_pckr__call );   //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lxt_pckrc__index ); //Stack: r,buf,idx,res,__index
				lua_pushvalue( luaVM, -5 );          //Stack: r,buf,idx,res,__call,__index,r
				lua_pushinteger( luaVM, n );         //Stack: r,buf,idx,res,__call,__index,r,n
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,__call,value
				lua_pushvalue( luaVM, -5 );          //Stack: r,buf,idx,res,__call,value,buf
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,value
				lua_rawseti( luaVM, -2, n );         //Stack: r,buf,idx,res
			}
			lua_remove( luaVM, -2 );                //Stack: r,buf,res
			return 1;
		}
	}
	else                              // write to input
	{
		if (pr->p->t < XT_PCK_STRUCT)      // handle atomic packer, return single value
		{
			return xt_pck_write( luaVM, pr->p, (unsigned char *) b );
		}
		else  // create a table ...
		{
			return xt_push_error( luaVM, "writing of complex types is not yet implemented");
		}
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * __tostring() representation of a xt.Pack.Struct.
 * \param   luaVM      The lua state.
 * \lparam  xt_pck_s   user_data.
 * \lreturn string     formatted string representing Struct.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pckc__tostring( lua_State *luaVM )
{
	struct xt_pck *pc  = xt_pckc_check_ud( luaVM, -1, 1 );

	switch( pc->t )
	{
		case XT_PCK_STRUCT:
			lua_pushfstring( luaVM, "xt.Pack.Struct[%d]{%d}: %p", pc->n, pc->sz, pc );
			break;
		case XT_PCK_ARRAY:
			lua_pushfstring( luaVM, "xt.Pack.Array[%d]{%d}: %p", pc->n, pc->sz, pc );
			break;
		default:
			xt_push_error( luaVM, "Can't read value from unknown packer type" );
	}
	return 1;
}

/**--------------------------------------------------------------------------
 * ToString representation of a xt.Pack.Reader.
 * \param   luaVM      The lua state.
 * \lparam  xt_pck_s   user_data.
 * \lreturn string     formatted string representing Struct.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pckr__tostring( lua_State *luaVM )
{
	struct xt_pckr *pr  = xt_pckr_check_ud( luaVM, -1, 1 );
	struct xt_pck  *p   = pr->p;

	switch( p->t )
	{
		case XT_PCK_INTL:
			lua_pushfstring( luaVM, "xt.Pack.Reader{INTL:%d/%d}: %p", p->sz, pr->o, p );
			break;
		case XT_PCK_INTB:
			lua_pushfstring( luaVM, "xt.Pack.Reader{INTB:%d/%d}: %p", p->sz, pr->o, p );
			break;
		case XT_PCK_BIT:
			lua_pushfstring( luaVM, "xt.Pack.Reader{BIT[%d/%d]:%d/%d}: %p", p->lB, p->oB, p->sz, pr->o, p );
			break;
		case XT_PCK_STR:
			lua_pushfstring( luaVM, "xt.Pack.Reader{STRING:%d/%d}: %p", p->sz, pr->o, p );
			break;
		case XT_PCK_FLT:
			lua_pushfstring( luaVM, "xt.Pack.Reader{FLOAT:%d/%d}: %p", p->sz, pr->o, p );
			break;
		case XT_PCK_STRUCT:
			lua_pushfstring( luaVM, "xt.Pack.Reader{STRUCT[%d]:%d/%d}: %p", p->n, p->sz, pr->o, p );
			break;
		case XT_PCK_ARRAY:
			lua_pushfstring( luaVM, "xt.Pack.Reader{ARRAY[%d]:%d/%d}: %p", p->n, p->sz, pr->o, p );
			break;
		default:
			xt_push_error( luaVM, "Can't read value from unknown packer type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the xt.Pack.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   luaVM lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int xt_pckrc_iter( lua_State *luaVM )
{
	struct xt_pckr *pr = xt_pckr_check_ud( luaVM, lua_upvalueindex( 1 ), 0 );
	struct xt_pck  *pc = (NULL == pr) ? xt_pckc_check_ud( luaVM, -1, 1 ) : pr->p;

	int crs;

	crs = lua_tointeger( luaVM, lua_upvalueindex( 2 ) );
	crs++;
	if (crs > (int) pc->n)
		return 0;
	else
	{
		lua_pushinteger( luaVM, crs );
		lua_replace( luaVM, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->iR );
	lua_rawgeti( luaVM, -2 ,crs );    // Stack: _idx, name
	lua_pushvalue( luaVM, -1);        // Stack: _idx, name, name
	lua_rawget( luaVM, -3 );          // Stack: _idx, name, Pack
	lua_remove( luaVM, -3 );          // remove idx table
	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the xt.Pack.Struct.
 * \param   luaVM lua Virtual Machine.
 * \lparam  iterator xt.Pack.Struct.
 * \lreturn pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pckrc__pairs( lua_State *luaVM )
{
	struct xt_pckr *pr  = xt_pckr_check_ud( luaVM, -1, 0 );
	struct xt_pck  *pc;

	pc = (NULL == pr) ? xt_pckc_check_ud( luaVM, -1, 1 ) : pr->p;

	lua_pushnumber( luaVM, 0 );
	lua_pushcclosure( luaVM, &xt_pckrc_iter, 2 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Pack.Struct library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pckc( lua_State *luaVM )
{
	// xt.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "xt.Pack.Struct" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pckrc__index);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pckrc__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_pckrc__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lxt_pckc__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pckrc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lxt_pckc__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Pack.Reader library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pckr( lua_State *luaVM )
{
	// xt.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "xt.Pack.Reader" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pckrc__index);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pckrc__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_pckrc__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lxt_pckr__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lxt_pckr__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pckrc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
