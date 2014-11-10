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
	ap->n  = luaL_checkinteger( luaVM, -2 );       // how many elements in the array
	ap->sz = ap->n * sizeof( p->sz );
	ap->t  = XT_PCK_ARRAY;

	luaL_getmetatable( luaVM, "xt.Pack.Struct" );
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  xt.Pack.Sequence Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  xt.Pack.
 * \lreturn userdata of xt.Pack.Sequence.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pckc_Sequence( lua_State *luaVM )
{
	size_t             i;      ///< iterator for going through the arguments
	size_t             bc=0;   ///< count bitSize for bit type packers
	struct xt_pck     *p;      ///< temporary packer/struct for iteration
	struct xt_pck     *cp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	cp     = (struct xt_pck *) lua_newuserdata( luaVM, sizeof( struct xt_pck ) );
	cp->n  = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the Struct userdata)
	cp->sz = 0;
	cp->t  = XT_PCK_SEQ;

	// create index table
	lua_createtable( luaVM, 2*cp->n, cp->n ); // Stack: ..., Struct,idx

	// populate table idx
	for (i=1; i<=cp->n; i++)
	{
		p = xt_pckc_check_ud( luaVM, i, 1 );
		lua_pushvalue( luaVM, i);          // Stack: ...,Seq,idx,Pack
		lua_pushinteger( luaVM, cp->sz+1); // Stack: ...,Seq,idx,Pack,ofs
		lua_rawseti( luaVM, -3, i+cp->n ); // Stack: ...,Seq,idx,Pack     idx[n+i] = offset
		lua_rawseti( luaVM, -2, i );       // Stack: ...,Seq,idx,         idx[i]   = Pack
		lua_pushvalue( luaVM, i);          // Stack: ...,Seq,idx,Pack
		// handle Bit type packers
		if (XT_PCK_BIT==p->t || XT_PCK_BITS==p->t || XT_PCK_NBL==p->t)
		{
			if ((p->oB-1) != bc%8)
				xt_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			if ((bc + p->lB)/8 > bc/8)
					  cp->sz += 1;
			bc = bc + p->lB;
		}
		else
		{
			if (bc%8)
				xt_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			else
				bc = 0;
			cp->sz += p->sz;
		}
		lua_pop( luaVM, 1 );   // pop packer from stack for next round
	}
	cp->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "xt.Pack.Struct" ); // Stack: ...,xt.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  xt.Pack.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  table { name = xt.Pack}.
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

	for (i=1; i<=cp->n; i++)
	{
		luaL_argcheck( luaVM, lua_istable( luaVM, i ), i,
			"Arguments must be tables with single key/xt.Pack pair" );
		// Stack gymnastic:
		lua_pushnil( luaVM );
		if (!lua_next( luaVM, i ))         // Stack: ...,Struct,idx,name,Pack
			return xt_push_error( luaVM, "the table argument must contain one key/value pair.");
		// check if name is already used!
		lua_pushvalue( luaVM, -2 );        // Stack: ...,Struct,idx,name,Pack,name
		lua_rawget( luaVM, -4 );
		if (! lua_isnoneornil( luaVM, -1 ))
			return xt_push_error( luaVM, "All elements in xt.Pack.Struct must have unique key.");
		lua_pop( luaVM, 1 );               // pop the nil
		p = xt_pckc_check_ud( luaVM, -1, 1 );    // allow xt.Pack or xt.Pack.Struct
		// populate idx table
		lua_pushinteger( luaVM, cp->sz+1);  // Stack: ...,Seq,idx,name,Pack,ofs
		lua_rawseti( luaVM, -4, i+cp->n );  // Stack: ...,Seq,idx,name,Pack             idx[n+i] = offset
		lua_pushvalue( luaVM, -1 );         // Stack: ...,Seq,idx,name,Pack,Pack
		lua_rawseti( luaVM, -4, i );        // Stack: ...,Seq,idx,name,Pack             idx[i] = Pack
		lua_insert( luaVM, -2);             // Stack: ...,Seq,idx,Pack,name             swap pack/name
		lua_pushvalue( luaVM, -1 );         // Stack: ...,Seq,idx,Pack,name,name
		lua_rawseti( luaVM, -4, cp->n*2+i );// Stack: ...,Seq,idx,Pack,name             idx[2n+i] = name
		lua_pushinteger( luaVM, i);         // Stack: ...,Seq,idx,Pack,name,i
		lua_rawset( luaVM, -4 );            // Stack: ...,Seq,idx,Pack                  idx[name] = i
		// Stack: ...,Struct,idx,Pack/Struct
		// handle Bit type packers
		if (XT_PCK_BIT==p->t || XT_PCK_BITS==p->t || XT_PCK_NBL==p->t)
		{
			if ((p->oB-1) != bc%8)
				xt_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			if ((bc + p->lB)/8 > bc/8)
					  cp->sz += 1;
			bc = bc + p->lB;
		}
		else
		{
			if (bc%8)
				xt_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			else
				bc = 0;
			cp->sz += p->sz;
		}
		lua_pop( luaVM, 1 );   // pop packer from stack for next round
	}
	cp->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "xt.Pack.Struct" ); // Stack: ...,xt.Pack.Struct
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

	if (lua_tonumber( luaVM, -1 ) && luaL_checkinteger( luaVM, -1 ) > (int) pc->n)
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( luaVM );
		return 1;
	}
	// get idx table (struct) or packer type (array)
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->iR );
	// Stack: Struct,idx/name,idx/Packer
	if (LUA_TUSERDATA == lua_type( luaVM, -1 ))        // xt.Array
	{
		p = xt_pckc_check_ud( luaVM, -1, 0 );
		pos += (p->sz * luaL_checkinteger( luaVM, -2 ));
	}
	else                                              // xt.Struct/Sequence
	{
		lua_pushvalue( luaVM, -2 );        // Stack: Struct,key,idx,key
		if (! lua_tonumber( luaVM, -3 ))               // xt.Struct
			lua_rawget( luaVM, -2);    // Stack: Struct,key,idx,i
		lua_rawgeti( luaVM, -2, lua_tointeger( luaVM, -1 ) + pc->n );  // Stack: Seq,key,idx,i,ofs
		pos += luaL_checkinteger( luaVM, -1);
		lua_rawgeti( luaVM, -3, lua_tointeger( luaVM, -2 ) );  // Stack: Seq,key,idx,i,ofs,Pack
	}
	p =  xt_pckc_check_ud( luaVM, -1, 1 );  // Stack: Seq,key,idx,i,ofs,Pack,Reader
	lua_pop( luaVM, 3 );

	xt_pckr_create_ud( luaVM, p, pos );
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
 * __call (#) for a an xt.Pack.Reader/Struct instance.
 *          This is used to either read from or write to a string or xt.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  ud        xt.Pack.Reader instance.
 * \lparam  ud,string xt.Buffer or Lua string.
 * \lparam   xt.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to xt.Pack.Reader.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pckrc__call( lua_State *luaVM );    // declaration for recursive call
static int lxt_pckrc__call( lua_State *luaVM )
{
	struct xt_pckr *pr = xt_pckr_check_ud( luaVM, -2, 0 );
	struct xt_pck  *p  = (NULL == pr) ? xt_pckc_check_ud( luaVM, -2, 1 ) : pr->p;
	size_t          o  = (NULL == pr) ? 0 : pr->o;

	struct xt_buf  *buf;
	unsigned char  *b;
	size_t          l;                   /// length of string or buffer overall
	size_t          n;                   /// iterator for complex types
	luaL_argcheck( luaVM,  2<=lua_gettop(luaVM) && lua_gettop( luaVM )<=3, 2,
		"Calling an xt.Pack.Reader takes 2 or 3 arguments!" );

	// are we reading/writing to from xt.Buffer or Lua String
	if (lua_isuserdata( luaVM, 2 ))      // xt.Buffer
	{
		buf = xt_buf_check_ud ( luaVM, 2, 1 );
		luaL_argcheck( luaVM,  buf->len >= o+p->sz, 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		b   =  &(buf->b[ o ]);
	}
	else
	{
		b   = (unsigned char *) luaL_checklstring( luaVM, 2, &l );
		luaL_argcheck( luaVM,  l > o+p->sz, 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		luaL_argcheck( luaVM,  2 == lua_gettop( luaVM ), 2,
			"Can't write to a Lua String since they are immutable." );
		b   =  b + o;
	}

	if (2 == lua_gettop( luaVM ))     // read from input
	{
		if (p->t < XT_PCK_ARRAY)      // handle atomic packer, return single value
		{
			return xt_pck_read( luaVM, p, (const unsigned char *) b );
		}
		if (p->t == XT_PCK_ARRAY)      // handle Array; return table
		{
			lua_createtable( luaVM, 0, p->n );   //Stack: r,buf,idx,res
			for (n=1; n<p->n; n++)
			{
				lua_pushcfunction( luaVM, lxt_pckrc__call );  //Stack: r,buf,res,__call
				lua_pushcfunction( luaVM, lxt_pckrc__index ); //Stack: r,buf,res,__call,__index
				lua_pushvalue( luaVM, -5 );          //Stack: r,buf,res,__call,__index,r
				lua_pushinteger( luaVM, n );         //Stack: r,buf,res,__call,__index,r,n
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,res,__call,value
				lua_pushvalue( luaVM, -4 );          //Stack: r,buf,res,__call,value,buf
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,res,value
				lua_rawseti( luaVM, -2, n );         //Stack: r,buf,res
			}
			lua_remove( luaVM, -2 );                //Stack: r,buf,res
			return 1;
		}
		if (p->t == XT_PCK_SEQ)            // handle Sequence, return table
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->iR ); // get index table
			lua_createtable( luaVM, 0, p->n );   //Stack: r,buf,idx,res
			for (n=1; n<p->n; n++)
			{
				lua_pushcfunction( luaVM, lxt_pckrc__call );  //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lxt_pckrc__index ); //Stack: r,buf,idx,res,_call,__index
				lua_pushvalue( luaVM, -6 );          //Stack: r,buf,idx,res,__call,__index,r
				lua_pushinteger( luaVM, n );         //Stack: r,buf,idx,res,__call,__index,r,i
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,__call,value
				lua_pushvalue( luaVM, -5 );          //Stack: r,buf,idx,res,__call,value,buf
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,value
				lua_rawseti( luaVM, -2, n );         //Stack: r,buf,idx,res
			}
			lua_remove( luaVM, -2 );                //Stack: r,buf,res
			return 1;
		}
		if (p->t == XT_PCK_STRUCT)      // handle Struct, return table
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->iR ); // get index table
			lua_createtable( luaVM, 0, p->n );   //Stack: r,buf,idx,res
			for (n=1; n<p->n; n++)
			{
				lua_pushcfunction( luaVM, lxt_pckrc__call );  //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lxt_pckrc__index ); //Stack: r,buf,idx,res,_call,__index
				lua_pushvalue( luaVM, -6 );          //Stack: r,buf,idx,res,__call,__index,r
				lua_rawgeti( luaVM, -5, 2*p->n + n );//Stack: r,buf,idx,res,__call,__index,r,name
				lua_pushvalue( luaVM, -1 );          //Stack: r,buf,idx,res,__call,__index,r,name,name
				lua_insert( luaVM, -5 );             //Stack: r,buf,idx,res,name,__call,__index,r,name
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,name,__call,value
				lua_pushvalue( luaVM, -6 );          //Stack: r,buf,idx,res,name,__call,value,buf
				lua_call( luaVM, 2, 1 );             //Stack: r,buf,idx,res,name,value
				lua_rawset( luaVM, -3 );             //Stack: r,buf,idx,res
			}
			lua_remove( luaVM, -2 );                //Stack: r,buf,res
			return 1;
		}
	}
	else                              // write to input
	{
		if (p->t < XT_PCK_ARRAY)      // handle atomic packer, return single value
		{
			return xt_pck_write( luaVM, p, (unsigned char *) b );
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

	if (pc->t<XT_PCK_ARRAY || pc->t > XT_PCK_STRUCT)
		xt_push_error( luaVM, "Can't read value from unknown packer type" );
	lua_pushfstring( luaVM, "xt.Pack.%s[%d]{%d}: %p",
		xt_pck_t_lst[ pc->t ], pc->n, pc->sz, pc );
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

	lua_pushfstring( luaVM, "xt.Pack.Reader(" );
	xt_pck_format( luaVM, p );
	lua_pushfstring( luaVM, ")[%d]: %p", pr->o, p );
	lua_concat( luaVM, 4 );

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
	struct xt_pck  *pc = (NULL == pr) ? xt_pckc_check_ud( luaVM, lua_upvalueindex( 1 ), 1 ) : pr->p;
	int              o = (NULL == pr) ? 0 : pr->o;

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
	if (XT_PCK_STRUCT == pc->t)                    // Get the name for a Struct value
		lua_rawgeti( luaVM, -1 , crs + pc->n*2 );   // Stack: func,nP,_idx,nC
	else
		lua_pushinteger( luaVM, crs );     // Stack: func,iP,_idx,iC
	lua_rawgeti( luaVM, -2 , crs );       // Stack: func,nP,_idx,xC,pack
	lua_rawgeti( luaVM, -3 , crs+pc->n ); // Stack: func,nP,_idx,xC,pack,pos
	lua_remove( luaVM, -4 );              // Stack: func,nP,xC,pack,pos
	xt_pckr_create_ud( luaVM,  xt_pckc_check_ud( luaVM, -2, 1 ), luaL_checkinteger( luaVM, -1 ) + o );
	lua_insert( luaVM, -3);
	lua_pop( luaVM, 2);

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
	struct xt_pckr *pr = xt_pckr_check_ud( luaVM, 1, 0 );
	if (NULL == pr)
		xt_pckc_check_ud( luaVM, 1, 1 );

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
	lua_pushcfunction( luaVM, lxt_pckrc__call );
	lua_setfield( luaVM, -2, "__call" );
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
	lua_pushcfunction( luaVM, lxt_pckrc__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lxt_pckr__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pckrc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
