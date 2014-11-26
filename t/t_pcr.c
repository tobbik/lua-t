/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pcr.c
 * \brief     create a packer Struct/Array
 *            Combinators for packers to create structures and arrays. This is
 *            interleved with T.Pack.Reader functionality (t_packr) becuase a
 *            lot of logic can be shared
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t.h"
#include "t_pck.h"


/**--------------------------------------------------------------------------
 * Desides if the element on pos is a packer kind of type.
 * It decides between the following options:
 *	    - T.Pack type              : just return it
 *	    - T.Pack.Reader            : return reader->t
 *	    - fmt string of single item: fetch from cache or create
 *	    - fmt string of mult items : let Sequence constructor handle and return result
 * \param   luaVM  The lua state.
 * \param   pos    position on stack.
 * \return  struct t_pcc* pointer.
 * --------------------------------------------------------------------------*/
static struct t_pcc
*t_pcc_get( lua_State luaVM, int pos )
{
	struct t_pcc     *p;      ///< packer
	int               is_little = IS_LITTLE_ENDIAN;
	const char       *fmt;
	if (lua_isuserdata( luaVM, pos ))
	{
		p = t_pcc_check_ud( luaVM, pos, 1 );          // allow T.Pack or T.Pack.Struct
	}
	else
	{
		fmt = luaL_checkstring( luaVM, pos );
		p = t_pcc_getoption( luaVM, &fmt, &is_little );
		while ('\0' != *fmt)
			while (NULL == p)
				p = t_pcc_getoption( luaVM, &fmt, &is_little );

		lua_replace( luaVM, pos );
	}
}


static struct t_pcc
*t_pcc_getnext( lua_State luaVM, const char **fmt, int pos  )
{
	int               nxt;
	struct t_pcc     *p;                          ///< packer
	int               is_little = IS_LITTLE_ENDIAN;

	if (NULL != fmt)
	{
		nxt = *((*f)++);
		if ('0' == nxt)
		{
			lua_pushnil( luaVM );
			return NULL;
		}
		p = t_pcc_getoption( luaVM, &fmt, &is_little );
		while (NULL == p)  p = t_pcc_getoption( luaVM, &fmt, &is_little );
		return p;
	}
	else
	{
	}
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Array Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  type identifier  T.Pack, T.Pack.Struct, T.Pack.Array .
 * \lparam  len              Number of elements in the array.
 * \lreturn userdata of T.Pack.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
lt_pcc_Array( lua_State *luaVM )
{
	struct t_pcc     *p;      ///< packer
	struct t_pcc     *ap;     ///< array userdata to be created
	int               is_little = IS_LITTLE_ENDIAN;
	const char       *fmt = luaL_checkstring( luaVM, 1 );

	p = t_pcc_get( luaVM, -2 );
	ap     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	ap->s  = luaL_checkinteger( luaVM, -2 );       // how many elements in the array
	ap->t  = T_PCK_ARRAY;
	//ap->sz = ap->n * sizeof( p->sz );

	lua_pushvalue( luaVM, -3 );
	ap->m = luaL_ref( luaVM, LUA_REGISTRYINDEX);  // register packer table

	luaL_getmetatable( luaVM, "T.Pack.Struct" );
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Sequence Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  T.Pack  or a sngle fmt string.
 * \lreturn userdata of T.Pack.Sequence.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
lt_pcc_Sequence( lua_State *luaVM )
{
	size_t            i;      ///< iterator for going through the arguments
	size_t            bc=0;   ///< count bitSize for bit type packers
	struct t_pck     *p;      ///< temporary packer/struct for iteration
	struct t_pck     *cp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	cp     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	cp->n  = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the Struct userdata)
	cp->sz = 0;
	cp->t  = T_PCK_SEQ;

	// create index table
	lua_createtable( luaVM, 2*cp->n, cp->n ); // Stack: ..., Struct,idx

	// populate table idx
	for (i=1; i<=cp->n; i++)
	{
		p = t_pckc_check_ud( luaVM, i, 1 );
		lua_pushvalue( luaVM, i);          // Stack: ...,Seq,idx,Pack
		lua_pushinteger( luaVM, cp->sz);   // Stack: ...,Seq,idx,Pack,ofs
		lua_rawseti( luaVM, -3, i+cp->n ); // Stack: ...,Seq,idx,Pack     idx[n+i] = offset
		lua_rawseti( luaVM, -2, i );       // Stack: ...,Seq,idx,         idx[i]   = Pack
		// handle Bit type packers
		if (T_PCK_BIT==p->t)
		{
			if ((p->oB-1) != bc%8)
				t_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			if ((bc + p->lB)/8 > bc/8)
					  cp->sz += 1;
			bc = bc + p->lB;
		}
		else
		{
			if (bc%8)
				t_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			else
				bc = 0;
			cp->sz += p->sz;
		}
	}
	cp->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "T.Pack.Struct" ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  table { name = T.Pack}.
 * \lreturn userdata of T.Pack.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
lt_pckc_Struct( lua_State *luaVM )
{
	size_t            i;      ///< iterator for going through the arguments
	size_t            bc=0;   ///< count bitSize for bit type packers
	struct t_pck     *p;      ///< temporary packer/struct for iteration
	struct t_pck     *cp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	cp     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	cp->n  = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the Struct userdata)
	cp->sz = 0;
	cp->t  = T_PCK_STRUCT;

	// create index table
	lua_createtable( luaVM, cp->n, cp->n ); // Stack: ..., Struct,idx

	while (t_pcc_getnext

	for (i=1; i<=cp->n; i++)
	{
		luaL_argcheck( luaVM, lua_istable( luaVM, i ), i,
			"Arguments must be tables with single key/T.Pack pair" );
		// Stack gymnastic:
		lua_pushnil( luaVM );
		if (!lua_next( luaVM, i ))         // Stack: ...,Struct,idx,name,Pack
			return t_push_error( luaVM, "the table argument must contain one key/value pair.");
		// check if name is already used!
		lua_pushvalue( luaVM, -2 );        // Stack: ...,Struct,idx,name,Pack,name
		lua_rawget( luaVM, -4 );
		if (! lua_isnoneornil( luaVM, -1 ))
			return t_push_error( luaVM, "All elements in T.Pack.Struct must have unique key.");
		lua_pop( luaVM, 1 );               // pop the nil
		p = t_pckc_check_ud( luaVM, -1, 1 );    // allow T.Pack or T.Pack.Struct
		// populate idx table
		lua_pushinteger( luaVM, cp->sz+1);  // Stack: ...,Seq,idx,name,Pack,ofs
		lua_rawseti( luaVM, -4, i+cp->n );  // Stack: ...,Seq,idx,name,Pack        idx[n+i] = offset
		lua_rawseti( luaVM, -3, i );        // Stack: ...,Seq,idx,name             idx[i] = Pack
		lua_pushvalue( luaVM, -1 );         // Stack: ...,Seq,idx,name,name
		lua_rawseti( luaVM, -3, cp->n*2+i );// Stack: ...,Seq,idx,name             idx[2n+i] = name
		lua_pushinteger( luaVM, i);         // Stack: ...,Seq,idx,name,i
		lua_rawset( luaVM, -3 );            // Stack: ...,Seq,idx                  idx[name] = i
		// Stack: ...,Struct,idx
		// handle Bit type packers
		if (T_PCK_BIT==p->t || T_PCK_BITS==p->t || T_PCK_NBL==p->t)
		{
			if ((p->oB-1) != bc%8)
				t_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			if ((bc + p->lB)/8 > bc/8)
					  cp->sz += 1;
			bc = bc + p->lB;
		}
		else
		{
			if (bc%8)
				t_push_error( luaVM, "bitsized fields must be ordered in appropriate size type" );
			else
				bc = 0;
			cp->sz += p->sz;
		}
	}
	cp->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "T.Pack.Struct" ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being an T.Pack OR * T.Pack.Struct/Array
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \param   int      check -> treats as check -> erros if fail
 * \lparam  userdata T.Pack.Struct on the stack.
 * \return  t_pck_s pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pcr_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "T.Pack.Struct" );
	if (NULL != ud)
		return (struct t_pck *) ud;
	ud = luaL_testudata( luaVM, pos, "T.Pack" );
	if (NULL != ud)
		return (struct t_pck *) ud;
	if (check)
		luaL_argcheck( luaVM, ud != NULL, pos, "`T.Pack.Struct` or `T.Pack` expected" );
	return NULL;
}


/**--------------------------------------------------------------------------
 * create a t_pckr and push to LuaStack.
 * \param   luaVM  The lua state.
 * \param   t_pck Packer reference.
 * \param   buffer offset.
 *
 * \return  struct t_pckr*  pointer to the  t_pckr struct
 * --------------------------------------------------------------------------*/
struct t_pckr
*t_pckr_create_ud( lua_State *luaVM, struct t_pck *p, size_t o )
{
	struct t_pckr  *pr;
	pr = (struct t_pckr *) lua_newuserdata( luaVM, sizeof( struct t_pckr ));

	pr->p  = p;
	pr->o  = o;
	luaL_getmetatable( luaVM, "T.Pack.Reader" );
	lua_setmetatable( luaVM, -2 );
	return pr;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being an T.Pack.Result
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \param   int      check -> treats as check -> erros if fail
 * \lparam  userdata T.Pack.Result on the stack.
 * \return  t_pck_s pointer.
 * --------------------------------------------------------------------------*/
struct t_pckr
*t_pckr_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "T.Pack.Reader" );
	if (check)
		luaL_argcheck( luaVM, ud != NULL, pos, "`T.Pack.Reader` expected" );
	return (NULL==ud) ? NULL : ((struct t_pckr *) ud);
}


/**--------------------------------------------------------------------------
 * Read a Struct packer value.
 *          This can not simply return a packer/Struct type since it now has
 *          meta information about the position it is requested from.  For this
 *          the is a new datatype T.Pack.Result which carries type and position
 *          information
 * \param   luaVM    The lua state.
 * \lparam  userdata T.Pack.Struct instance.
 * \lparam  key      string/integer.
 * \lreturn userdata Pack or Struct instance.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pckrc__index( lua_State *luaVM )
{
	struct t_pckr *pr  = t_pckr_check_ud( luaVM, -2, 0 );
	struct t_pck  *pc  = (NULL == pr) ? t_pckc_check_ud( luaVM, -2, 1 ) : pr->p;
	struct t_pck  *p;
	int            pos = (NULL == pr) ? 0 : pr->o-1;  // recorded offset is 1 based -> don't add up

	if (LUA_TNUMBER == lua_type( luaVM, -1 ) &&
	      ((luaL_checkinteger( luaVM, -1 ) > (int) pc->n) || (luaL_checkinteger( luaVM, -1 ) < 1))
	   )
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( luaVM );
		return 1;
	}
	// get idx table (struct) or packer type (array)
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->iR );
	// Stack: Struct,idx/name,idx/Packer
	if (LUA_TUSERDATA == lua_type( luaVM, -1 ))        // T.Array
	{
		p = t_pckc_check_ud( luaVM, -1, 0 );
		pos += (p->sz * (luaL_checkinteger( luaVM, -2 )-1)) + 1;
	}
	else                                               // T.Struct/Sequence
	{
		lua_pushvalue( luaVM, -2 );        // Stack: Struct,key,idx,key
		if (! lua_tonumber( luaVM, -3 ))               // T.Struct
			lua_rawget( luaVM, -2);    // Stack: Struct,key,idx,i
		lua_rawgeti( luaVM, -2, lua_tointeger( luaVM, -1 ) + pc->n );  // Stack: Seq,key,idx,i,ofs
		pos += luaL_checkinteger( luaVM, -1);
		lua_rawgeti( luaVM, -3, lua_tointeger( luaVM, -2 ) );  // Stack: Seq,key,idx,i,ofs,Pack
	}
	p =  t_pckc_check_ud( luaVM, -1, 1 );  // Stack: Seq,key,idx,i,ofs,Pack,Reader
	lua_pop( luaVM, 3 );

	t_pckr_create_ud( luaVM, p, pos );
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an T.Pack.Struct.
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pckrc__newindex( lua_State *luaVM )
{
	struct t_pckr *pr  = t_pckr_check_ud( luaVM, -3, 0 );

	(NULL == pr) ? t_pckc_check_ud( luaVM, -3, 1 ) : pr->p;

	return t_push_error( luaVM, "Packers are static and can't be updated!" );
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  luaVM lua Virtual Machine.
 * \lparam ud    T.Pack.Struct.
 * \return int   # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_pckc__gc( lua_State *luaVM )
{
	struct t_pck *cp = t_pckc_check_ud( luaVM, 1, 1 );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, cp->iR );
	return 0;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a Struct/Reader instance.
 * \param   luaVM  lua Virtual Machine.
 * \lparam  ud     T.Pack.Struct/Reader instance.
 * \lreturn int    # of elements in T.Pack.Struct/Reader instance.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_pckrc__len( lua_State *luaVM )
{
	struct t_pckr *pr = t_pckr_check_ud( luaVM, 1, 0 );
	struct t_pck  *pc = (NULL == pr) ? t_pckc_check_ud( luaVM, -1, 1 ) : pr->p;

	lua_pushinteger( luaVM, pc->n );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an T.Pack.Reader/Struct instance.
 *          This is used to either read from or write to a string or T.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  ud        T.Pack.Reader instance.
 * \lparam  ud,string T.Buffer or Lua string.
 * \lparam  T.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to T.Pack.Reader.
 * \return  int    # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_pckrc__call( lua_State *luaVM )
{
	struct t_pckr *pr = t_pckr_check_ud( luaVM, 1, 0 );
	struct t_pck  *p  = (NULL == pr) ? t_pckc_check_ud( luaVM, 1, 1 ) : pr->p;
	size_t         o  = (NULL == pr) ? 0 : pr->o-1;

	struct t_buf  *buf;
	unsigned char *b;
	size_t         l;                   /// length of string or buffer overall
	size_t         n;                   /// iterator for complex types
	luaL_argcheck( luaVM,  2<=lua_gettop( luaVM ) && lua_gettop( luaVM )<=3, 2,
		"Calling an T.Pack.Reader takes 2 or 3 arguments!" );

	// are we reading/writing to from T.Buffer or Lua String
	if (lua_isuserdata( luaVM, 2 ))      // T.Buffer
	{
		buf = t_buf_check_ud ( luaVM, 2, 1 );
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

	if (2 == lua_gettop( luaVM ))    // read from input
	{
		if (p->t < T_PCK_ARRAY)      // handle atomic packer, return single value
		{
			return t_pck_read( luaVM, p, (const unsigned char *) b );
		}
		if (p->t == T_PCK_ARRAY)     // handle Array; return table
		{
			lua_createtable( luaVM, p->n, 0 );      //Stack: r,buf,idx,res
			for (n=1; n<=p->n; n++)
			{
				lua_pushcfunction( luaVM, lt_pckrc__call );  //Stack: r,buf,res,__call
				lua_pushcfunction( luaVM, lt_pckrc__index ); //Stack: r,buf,res,__call,__index
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
		if (p->t == T_PCK_SEQ)       // handle Sequence, return table
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->iR ); // get index table
			lua_createtable( luaVM, 0, p->n );      //Stack: r,buf,idx,res
			for (n=1; n<=p->n; n++)
			{
				lua_pushcfunction( luaVM, lt_pckrc__call );  //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lt_pckrc__index ); //Stack: r,buf,idx,res,_call,__index
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
		if (p->t == T_PCK_STRUCT)    // handle Struct, return table
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->iR ); // get index table
			lua_createtable( luaVM, 0, p->n );      //Stack: r,buf,idx,res
			for (n=1; n<=p->n; n++)
			{
				lua_pushcfunction( luaVM, lt_pckrc__call );  //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lt_pckrc__index ); //Stack: r,buf,idx,res,_call,__index
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
		if (p->t < T_PCK_ARRAY)      // handle atomic packer, return single value
		{
			return t_pck_write( luaVM, p, (unsigned char *) b );
		}
		else  // create a table ...
		{
			return t_push_error( luaVM, "writing of complex types is not yet implemented");
		}
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * __tostring() representation of a T.Pack.Struct.
 * \param   luaVM      The lua state.
 * \lparam  t_pck_s   user_data.
 * \lreturn string     formatted string representing Struct.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_pckc__tostring( lua_State *luaVM )
{
	struct t_pck *pc  = t_pckc_check_ud( luaVM, -1, 1 );

	if (pc->t<T_PCK_ARRAY || pc->t > T_PCK_STRUCT)
		t_push_error( luaVM, "Can't read value from unknown packer type" );
	lua_pushfstring( luaVM, "T.Pack.%s[%d]{%d}: %p",
		t_pck_t_lst[ pc->t ], pc->n, pc->sz, pc );
	return 1;
}


/**--------------------------------------------------------------------------
 * ToString representation of a T.Pack.Reader.
 * \param   luaVM     The lua state.
 * \lparam  t_pck_s   user_data.
 * \lreturn string    formatted string representing Struct.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_pckr__tostring( lua_State *luaVM )
{
	struct t_pckr *pr  = t_pckr_check_ud( luaVM, -1, 1 );
	struct t_pck  *p   = pr->p;

	lua_pushfstring( luaVM, "T.Pack.Reader[%d](", pr->o );
	t_pck_format( luaVM, p );
	lua_pushfstring( luaVM, "): %p", p );
	lua_concat( luaVM, 4 );

	return 1;
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.Pack.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   luaVM lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int
t_pckrc_iter( lua_State *luaVM )
{
	struct t_pckr *pr = t_pckr_check_ud( luaVM, lua_upvalueindex( 1 ), 0 );
	struct t_pck  *pc = (NULL == pr) ? t_pckc_check_ud( luaVM, lua_upvalueindex( 1 ), 1 ) : pr->p;
	int            o  = (NULL == pr) ? 0 : pr->o;

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
	if (T_PCK_STRUCT == pc->t)                    // Get the name for a Struct value
		lua_rawgeti( luaVM, -1 , crs + pc->n*2 );   // Stack: func,nP,_idx,nC
	else
		lua_pushinteger( luaVM, crs );     // Stack: func,iP,_idx,iC
	lua_rawgeti( luaVM, -2 , crs );       // Stack: func,nP,_idx,xC,pack
	lua_rawgeti( luaVM, -3 , crs+pc->n ); // Stack: func,nP,_idx,xC,pack,pos
	lua_remove( luaVM, -4 );              // Stack: func,nP,xC,pack,pos
	t_pckr_create_ud( luaVM,  t_pckc_check_ud( luaVM, -2, 1 ), luaL_checkinteger( luaVM, -1 ) + o );
	lua_insert( luaVM, -3);
	lua_pop( luaVM, 2);

	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.Pack.Struct.
 * \param   luaVM lua Virtual Machine.
 * \lparam  iterator T.Pack.Struct.
 * \lreturn pos    position in t_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int
lt_pckrc__pairs( lua_State *luaVM )
{
	struct t_pckr *pr = t_pckr_check_ud( luaVM, 1, 0 );
	if (NULL == pr)
		t_pckc_check_ud( luaVM, 1, 1 );

	lua_pushnumber( luaVM, 0 );
	lua_pushcclosure( luaVM, &t_pckrc_iter, 2 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the T.Pack.Struct library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_pckc( lua_State *luaVM )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "T.Pack.Struct" );   // stack: functions meta
	lua_pushcfunction( luaVM, lt_pckrc__index);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pckrc__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_pckrc__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lt_pckrc__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lt_pckc__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pckrc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lt_pckc__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the T.Pack.Reader library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_pckr( lua_State *luaVM )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "T.Pack.Reader" );   // stack: functions meta
	lua_pushcfunction( luaVM, lt_pckrc__index);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pckrc__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_pckrc__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lt_pckrc__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lt_pckr__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pckrc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
