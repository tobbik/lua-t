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
#include "t_buf.h"


/**--------------------------------------------------------------------------
 * Decides if the element on pos is a packer kind of type.
 * It decides between the following options:
 *     - T.Pack type              : just return it
 *     - T.Pack.Reader            : return reader->p
 *     - fmt string of single item: fetch from cache or create
 *     - fmt string of mult items : let Sequence constructor handle and return result
 * \param   luaVM  The lua state.
 * \param   pos    position on stack.
 * \param   atom   boolean atomic packers only.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_getpck( lua_State luaVM, int pos, int atom )
{
	struct t_pck *p;      ///< packer
	struct t_pcr *r;      ///< reader
	int           l = IS_LITTLE_ENDIAN;
	int           o = 0;
	int           n = 1;  ///< counter for packers created from fmt string
	const char   *fmt;

	if (lua_isuserdata( luaVM, pos ))
	{
		r = t_pcr_check_ud( luaVM, pos, 0 );
		p = (NULL == r) ? t_pcc_check_ud( luaVM, pos, 1 ) : r->p;
	}
	else
	{
		fmt = luaL_checkstring( luaVM, pos );
		p   = t_pck_getoption( luaVM, &fmt, &l, &o );
		if ('\0' != *(fmt++))
			while (NULL != p )
				p   = t_pck_getoption( luaVM, &fmt, &l, &o );
			p =  t_pck_mksequence( luaVM, luaL_checkstring( luaVM, pos ) );
	}
	if (atom && T_PCK_RAW < p->t)
		luaL_error( luaVM, "Atomic Packer type required" );
	return p;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Array Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  type identifier  T.Pack, T.Pack.Struct, T.Pack.Array .
 * \lparam  len              Number of elements in the array.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_mkarray( lua_State *luaVM )
{
	struct t_pck     *p  = t_pck_getpck( luaVM, -2, 0 );  ///< packer
	struct t_pck     *ap;     ///< array userdata to be created

	ap    = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	ap->t = T_PCK_ARR;
	ap->s = luaL_checkinteger( luaVM, -2 );      // how many elements in the array

	lua_pushvalue( luaVM, -3 );  // Stack: Pack,n,Array,Pack
	ap->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register packer table
	//ap->sz = ap->n * sizeof( p->sz );


	luaL_getmetatable( luaVM, "T.Pack.Struct" );
	lua_setmetatable( luaVM, -2 ) ;

	return ap;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Sequence Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \param   int sp start position on Stack for first Packer.
 * \param   int ep   end position on Stack for last Packer.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_mksequence( lua_State *luaVM, int sp, int ep )
{
	size_t        n=1, i; ///< iterator for going through the arguments
	size_t        o=0;    ///< byte offset within the sequence
	struct t_pck *p;      ///< temporary packer/struct for iteration
	struct t_pck *sq;     ///< the userdata this constructor creates

	sq     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	sq->t  = T_PCK_SEQ;
	sq->s  = ep-sp;

	// create and populate index table
	lua_newtable( luaVM );                  // Stack: fmt,Seq,idx
	for (i=sp; i<=ep, i++)
	{
		p = t_pck_check_ud( luaVM, i, 1 );
		lua_pushvalue( luaVM, i );           // Stack: fmt,Seq,idx,Pack
		lua_pushinteger( luaVM, o );         // Stack: fmt,Seq,idx,Pack,ofs
		lua_rawseti( luaVM, -3, n + sq->s ); // Stack: fmt,Seq,idx,Pack     idx[n+i] = offset
		lua_rawseti( luaVM, -2, n );         // Stack: fmt,Seq,idx,         idx[i]   = Pack
		o += t_pck_getsize( luaVM, p );
		n++;
	}
	sq->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "T.Pack.Struct" ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \param   luaVM  The lua state.
 * \param   int sp start position on Stack for first Packer.
 * \param   int ep   end position on Stack for last Packer.
 * \lparam  ... multiple of type  table { name = T.Pack}.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_mkstruct( lua_State *luaVM, int sp, int ep )
{
	size_t        n=1, i; ///< iterator for going through the arguments
	size_t        o=0;    ///< byte offset within the sequence
	struct t_pck *p;      ///< temporary packer/struct for iteration
	struct t_pck *st;     ///< the userdata this constructor creates

	st     = (struct t_pck *) lua_newuserdata( luaVM, sizeof( struct t_pck ) );
	st->t  = T_PCK_SEQ;
	st->s  = ep-sp;

	// create and populate index table
	lua_newtable( luaVM );                // Stack: fmt,Seq,idx
	for (i=sp; i<=ep, i++)
	{
		luaL_argcheck( luaVM, lua_istable( luaVM, i ), i,
			"Arguments must be tables with single key/T.Pack pair" );
		// Stack gymnastic:
		lua_pushnil( luaVM );
		if (!lua_next( luaVM, i ))         // Stack: ...,Struct,idx,name,Pack
			return t_push_error( luaVM, "the table argument must contain one key/value pair." );
		// check if name is already used!
		lua_pushvalue( luaVM, -2 );        // Stack: ...,Struct,idx,name,Pack,name
		lua_rawget( luaVM, -4 );
		if (! lua_isnoneornil( luaVM, -1 ))
			return t_push_error( luaVM, "All elements in T.Pack.Struct must have unique key." );
		lua_pop( luaVM, 1 );               // pop the nil
		p = t_pck_getpck( luaVM, -1, 1 );    // allow T.Pack or T.Pack.Struct
		// populate idx table
		lua_pushinteger( luaVM, o );         // Stack: ...,Seq,idx,name,Pack,ofs
		lua_rawseti( luaVM, -4, n + st->s ); // Stack: ...,Seq,idx,name,Pack        idx[n+i] = offset
		lua_rawseti( luaVM, -3, n );         // Stack: ...,Seq,idx,name             idx[i] = Pack
		lua_pushvalue( luaVM, -1 );          // Stack: ...,Seq,idx,name,name
		lua_rawseti( luaVM, -3, st->s*2+n ); // Stack: ...,Seq,idx,name             idx[2n+i] = name
		lua_pushinteger( luaVM, n);          // Stack: ...,Seq,idx,name,i
		lua_rawset( luaVM, -3 );             // Stack: ...,Seq,idx                  idx[name] = i
		o += t_pck_getsize( luaVM, p );
		n++;
	}
	st->m = luaL_ref( luaVM, LUA_REGISTRYINDEX); // register index  table

	luaL_getmetatable( luaVM, "T.Pack.Struct" ); // Stack: ...,T.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
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
lt_pck__index( lua_State *luaVM )
{
	struct t_pcr *pr  = t_pcr_check_ud( luaVM, -2, 0 );
	struct t_pck *pc  = (NULL == pr) ? t_pck_check_ud( luaVM, -2, 1 ) : pr->p;
	struct t_pck *p;
	int           pos = (NULL == pr) ? 0 : pr->o-1;  // recorded offset is 1 based -> don't add up

	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, -2, "Trying to index Atomic T.Pack type" );

	if (LUA_TNUMBER == lua_type( luaVM, -1 ) &&
	      ((luaL_checkinteger( luaVM, -1 ) > (int) pc->n) || (luaL_checkinteger( luaVM, -1 ) < 1))
	   )
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( luaVM );
		return 1;
	}
	// get idx table (struct) or packer type (array)
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->m );
	// Stack: Struct,idx/name,idx/Packer
	if (LUA_TUSERDATA == lua_type( luaVM, -1 ))        // T.Array
	{
		p = t_pck_check_ud( luaVM, -1, 0 );
		pos += ((t_pck_getsize( luaVm, p )) * (luaL_checkinteger( luaVM, -2 )-1)) + 1;
	}
	else                                               // T.Struct/Sequence
	{
		lua_pushvalue( luaVM, -2 );        // Stack: Struct,key,idx,key
		if (! lua_tonumber( luaVM, -3 ))               // T.Struct
			lua_rawget( luaVM, -2);    // Stack: Struct,key,idx,i
		lua_rawgeti( luaVM, -2, lua_tointeger( luaVM, -1 ) + pc->s );  // Stack: Seq,key,idx,i,ofs
		pos += luaL_checkinteger( luaVM, -1);
		lua_rawgeti( luaVM, -3, lua_tointeger( luaVM, -2 ) );  // Stack: Seq,key,idx,i,ofs,Pack
	}
	p =  t_pckc_check_ud( luaVM, -1, 1 );  // Stack: Seq,key,idx,i,ofs,Pack,Reader
	lua_pop( luaVM, 3 );

	t_pcr_create_ud( luaVM, p, pos );
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an T.Pack.Struct ---> NOT ALLOWED.
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__newindex( lua_State *luaVM )
{
	struct t_pcr *pr  = t_pckr_check_ud( luaVM, -3, 0 );

	(NULL == pr) ? t_pck_check_ud( luaVM, -3, 1 ) : pr->p;

	return t_push_error( luaVM, "Packers are static and can't be updated!" );
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
lt_pcr__call( lua_State *luaVM )
{
	struct t_pcr *pr = t_pcr_check_ud( luaVM, 1, 0 );
	struct t_pck *p  = (NULL == pr) ? t_pckc_check_ud( luaVM, 1, 1 ) : pr->p;
	size_t        o  = (NULL == pr) ? 0 : pr->o-1;

	struct t_buf *buf;
	unsigned char *b;
	size_t        l;                   /// length of string or buffer overall
	size_t        n;                   /// iterator for complex types
	luaL_argcheck( luaVM,  2<=lua_gettop( luaVM ) && lua_gettop( luaVM )<=3, 2,
		"Calling an T.Pack.Reader takes 2 or 3 arguments!" );

	// are we reading/writing to from T.Buffer or Lua String
	if (lua_isuserdata( luaVM, 2 ))      // T.Buffer
	{
		buf = t_buf_check_ud ( luaVM, 2, 1 );
		luaL_argcheck( luaVM,  buf->len >= o+t_pck_getsize( luaVM, p ), 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		b   =  &(buf->b[ o ]);
	}
	else
	{
		b   = (unsigned char *) luaL_checklstring( luaVM, 2, &l );
		luaL_argcheck( luaVM,  l > o+t_pck_getsize( luaVM, p ), 2,
			"The length of the Buffer must be longer than Pack offset plus Pack length." );
		luaL_argcheck( luaVM,  2 == lua_gettop( luaVM ), 2,
			"Can't write to a Lua String since they are immutable." );
		b   =  b + o;
	}

	if (2 == lua_gettop( luaVM ))    // read from input
	{
		if (p->t < T_PCK_ARR)         // handle atomic packer, return single value
		{
			return t_pck_read( luaVM, p, (const unsigned char *) b );
		}
		if (p->t == T_PCK_ARR)     // handle Array; return table
		{
			lua_createtable( luaVM, p->s, 0 );      //Stack: r,buf,idx,res
			for (n=1; n <= p->s; n++)
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
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->m ); // get index table
			lua_createtable( luaVM, 0, p->s );      //Stack: r,buf,idx,res
			for (n=1; n <= p->s; n++)
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
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, p->m ); // get index table
			lua_createtable( luaVM, 0, p->n );      //Stack: r,buf,idx,res
			for (n=1; n <= p->s; n++)
			{
				lua_pushcfunction( luaVM, lt_pckrc__call );  //Stack: r,buf,idx,res,__call
				lua_pushcfunction( luaVM, lt_pckrc__index ); //Stack: r,buf,idx,res,_call,__index
				lua_pushvalue( luaVM, -6 );          //Stack: r,buf,idx,res,__call,__index,r
				lua_rawgeti( luaVM, -5, 2*p->s + n );//Stack: r,buf,idx,res,__call,__index,r,name
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
		if (p->t < T_PCK_ARR)          // handle atomic packer, return single value
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
t_pck_iter( lua_State *luaVM )
{
	struct t_pcr *pr = t_pckr_check_ud( luaVM, lua_upvalueindex( 1 ), 0 );
	struct t_pck *pc = (NULL == pr) ? t_pckc_check_ud( luaVM, lua_upvalueindex( 1 ), 1 ) : pr->p;
	int           o  = (NULL == pr) ? 0 : pr->o;

	// get current index and increment
	int crs = lua_tointeger( luaVM, lua_upvalueindex( 2 ) ) + 1;

	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, 1, "Attempt to get length of atomic T.Pack type" );

	if (crs > (int) pc->s)
		return 0;
	else
	{
		lua_pushinteger( luaVM, crs );
		lua_replace( luaVM, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, pc->m );
	if (T_PCK_STR == pc->t)                        // Get the name for a Struct value
		lua_rawgeti( luaVM, -1 , crs + pc->s*2 );   // Stack: func,nP,_idx,nC
	else
		lua_pushinteger( luaVM, crs );     // Stack: func,iP,_idx,iC
	lua_rawgeti( luaVM, -2 , crs );       // Stack: func,nP,_idx,xC,pack
	lua_rawgeti( luaVM, -3 , crs+pc->s ); // Stack: func,nP,_idx,xC,pack,pos
	lua_remove( luaVM, -4 );              // Stack: func,nP,xC,pack,pos
	t_pckr_create_ud( luaVM,  t_pckc_check_ud( luaVM, -2, 1 ), luaL_checkinteger( luaVM, -1 ) + o );
	lua_insert( luaVM, -3 );
	lua_pop( luaVM, 2 );

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
lt_pck__pairs( lua_State *luaVM )
{
	struct t_pcr *pr = t_pcr_check_ud( luaVM, 1, 0 );
	struct t_pck *pc = (NULL == pr) ? t_pckc_check_ud( luaVM, 1, 1 ) : pr->p;
	luaL_argcheck( luaVM, pc->t > T_PCK_RAW, 1, "Attempt to get length of atomic T.Pack type" );

	lua_pushnumber( luaVM, 0 );
	lua_pushcclosure( luaVM, &t_pck_iter, 2 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the T.Pack/Struct library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_pck( lua_State *luaVM )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "T.Pack" );   // stack: functions meta
	lua_pushcfunction( luaVM, lt_pck__index);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_pck__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_pck__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lt_pck__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_pck__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lt_pck__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pushcfunction( luaVM, lt_pcr__call );
	lua_setfield( luaVM, -2, "__call" );
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
