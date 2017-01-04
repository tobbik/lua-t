/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck_cmb.c
 * \brief     Helper functions for T.Pack combinators(Struct,Sequence,Array)
 *            - create functions
 *            - index functions
 *            - iterate functions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t.h"
#include "t_pck.h"


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Struct Object and put it onto the stack.
 * \param   L      Lua state.
 * \param   int    sp start position on Stack for first Packer.
 * \param   int    ep   end position on Stack for last Packer.
 * \lparam  tbl,…  multiple of type  table { name = T.Pack}.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_str_create( lua_State *L, int sp, int ep )
{
	size_t        n  = 1;  ///< iterator for going through the arguments
	size_t        o  = 0;  ///< byte offset within the sequence
	size_t        bo = 0;  ///< bit  offset within the sequence
	struct t_pck *p;       ///< temporary packer/struct for iteration
	struct t_pck *st;      ///< the userdata this constructor creates

	st     = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	st->t  = T_PCK_STR;
	st->s  = (ep-sp) + 1;

	// create and populate index table
	lua_newtable( L );                  // S:… Str tbl
	while (n <= st->s)
	{
		luaL_argcheck( L, lua_istable( L, sp ), n,
			"Arguments must be tables with single key/"T_PCK_TYPE" pair" );
		// Stack gymnastic:
		lua_pushnil( L );
		if (! lua_next( L, sp ))         // S:… Str tbl nme Pck
			luaL_error( L, "The table argument must contain one key/value pair." );
		// check if name is already used!
		lua_pushvalue( L, -2 );          // S:… Str tbl nme Pck name
		lua_rawget( L, -4 );             // S:… Str tbl nme Pck nil?
		if (! lua_isnoneornil( L, -1 ))
			luaL_error( L, "All elements in "T_PCK_TYPE".Struct must have unique key." );
		lua_pop( L, 1 );                 // S:… Str tbl nme Pck
		p = t_pck_getPacker( L, -1, &bo );  // allow T.Pack or T.Pack.Struct
		// populate tbl table
		lua_pushinteger( L, o/8 );       // S:… Str tbl nme Pck ofs
		lua_rawseti( L, -4, n + st->s ); // S:… Str tbl nme Pck       tbl[n+i]  = offset
		lua_rawseti( L, -3, n );         // S:… Str tbl nme           tbl[i  ]  = Pack
		lua_pushvalue( L, -1 );          // S:… Str tbl nme nme
		lua_rawseti( L, -3, st->s*2+n ); // S:… Str tbl nme           tbl[2n+i] = name
		lua_pushinteger( L, n);          // S:… Str tbl nme i
		lua_rawset( L, -3 );             // S:… Str tbl               tbl[name] = i
		o += t_pck_getSize( L, p, 1 );
		n++;
		lua_remove( L, sp );
	}

	st->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register index  table

	luaL_getmetatable( L, T_PCK_TYPE ); // S:… T.Pack.Struct
	lua_setmetatable( L, -2 ) ;

	return st;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Sequence Object and put it onto the stack.
 * \param   L      Lua state.
 * \param   int    sp start position on Stack for first Packer.
 * \param   int    ep   end position on Stack for last Packer.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_seq_create( lua_State *L, int sp, int ep, size_t *bo )
{
	size_t        n=1;    ///< iterator for going through the arguments
	size_t        o=0;    ///< byte offset within the sequence
	struct t_pck *p;      ///< temporary packer/struct for iteration
	struct t_pck *sq;     ///< the userdata this constructor creates

	sq     = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	sq->t  = T_PCK_SEQ;
	sq->s  = (ep-sp)+1;

	// create and populate index table
	lua_newtable( L );                  //S: fmt Seq idx
	while (n <= sq->s)
	{
		p = t_pck_getPacker( L, sp, bo );
		lua_pushvalue( L, sp );          //S: fmt Seq idx Pack
		lua_pushinteger( L, o/8 );       //S: fmt Seq idx Pack ofs
		lua_rawseti( L, -3, n + sq->s ); //S: fmt Seq idx Pack     idx[n+i] = offset
		lua_rawseti( L, -2, n );         //S: fmt Seq idx          idx[i]   = Pack
		o += t_pck_getSize( L, p, 1 );
		n++;
		lua_remove( L, sp );
	}
	sq->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register index  table

	luaL_getmetatable( L, T_PCK_TYPE ); //S: … T.Pack.Struct
	lua_setmetatable( L, -2 ) ;

	return sq;
}


/**--------------------------------------------------------------------------
 * Create a  T.Pack.Array Object and put it onto the stack.
 * \param   L          Lua state.
 * \lparam  ud/string  T.Pack, T.Pack.Struct, T.Pack.Array or format string.
 * \lparam  integer    Number of elements in the array.
 * \return  struct     t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_arr_create( lua_State *L )
{
	size_t                                    bo = 0;
	struct t_pck  __attribute__ ((unused))   *p  = t_pck_getPacker( L, -2, &bo );  ///< packer
	struct t_pck     *ap;     ///< array userdata to be created

	ap    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	ap->t = T_PCK_ARR;
	ap->s = luaL_checkinteger( L, -2 );       // how many elements in array

	lua_pushvalue( L, -3 );                   //S: Pck n Array Pck
	ap->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register packer table

	luaL_getmetatable( L, T_PCK_TYPE );
	lua_setmetatable( L, -2 ) ;

	return ap;
}


/**--------------------------------------------------------------------------
 * Read a Struct packer value.
 *          This can not simply return a packer/Struct type since it now has
 *          meta information about the position it is requested from.  For this
 *          the is a new datatype T.Pack.Result which carries type and position
 *          information
 * \param   L    Lua state.
 * \lparam  ud   T.Pack.Struct userdata instance.
 * \lparam  key  string/integer.
 * \lreturn ud   T.Pack.Field instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_pck_fld__index( lua_State *L )
{
	struct t_pck_fld *pf  = NULL;
	struct t_pck     *pc  = t_pck_fld_getPackFromStack( L, -2, &pf );
	struct t_pck     *p;
	struct t_pck_fld *r;

	luaL_argcheck( L, pc->t > T_PCK_RAW, -2, "Trying to index Atomic "T_PCK_TYPE" type" );

	if (LUA_TNUMBER == lua_type( L, -1 ) &&
	   ((luaL_checkinteger( L, -1 ) > (int) pc->s) || (luaL_checkinteger( L, -1 ) < 1))
	)
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( L );
		return 1;
	}
	// push empty field on stack
	r    = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ));
	r->o = (NULL == pf )? 0 : pf->o;  // recorded offset is 1 based -> don't add up
	// get idx table (struct) or packer type (array)
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m );
	//t_stackDump( L );
	                                                 // S: Str idx/name Fld idx/Pack
	if (LUA_TUSERDATA == lua_type( L, -1 ))    // T.Array
	{
		p     = t_pck_check_ud( L, -1, 1 );
		if (T_PCK_BOL == p->t  || T_PCK_BTS == p->t  || T_PCK_BTU == p->t)
		{
			lua_pop( L, 1 );
			p = t_pck_create_ud( L, p->t, p->s,
				((p->s * (luaL_checkinteger( L, -2 )-1)) % NB ) );
		}
		r->o += (((t_pck_getSize( L, p, 1 )) * (luaL_checkinteger( L, -3 )-1)) / NB);
	}
	else                                               // T.Pack.Struct/Sequence
	{
		if (! lua_tonumber( L, -3 ))            // T.Pack.Struct
		{
			lua_pushvalue( L, -3 );                    //S: Str key Fld idx key
			lua_rawget( L, -2 );                       //S: Str key Fld idx i
			lua_replace( L, -4 );                      //S: Str i Fld idx
		}
		lua_rawgeti( L, -1, lua_tointeger( L, -3 ) + pc->s );  //S: Seq i Fld idx ofs
		r->o += luaL_checkinteger( L, -1 );
		lua_pop( L, 1 );                              //S: Seq i Fld idx
		lua_rawgeti( L, -1, lua_tointeger( L, -3 ) ); //S: Seq i Fld idx Pack
		lua_remove( L, -2 );
		p =  t_pck_check_ud( L, -1, 1 );              //S: Seq i Fld Pack
	}

	r->pR  = luaL_ref( L, LUA_REGISTRYINDEX );       //S: Seq i Fld
	luaL_getmetatable( L, T_PCK_FLD_TYPE );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an T.Pack.Struct ---> NOT ALLOWED.
 * \param   L     Lua state.
 * \lparam  ud    Combinator instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_pck_fld__newindex( lua_State *L )
{
	struct t_pck *pc = t_pck_fld_getPackFromStack( L, -3, NULL );

	luaL_argcheck( L, pc->t > T_PCK_RAW, -3, "Atomic "T_PCK_TYPE" type has no fields" );

	return t_push_error( L, T_PCK_TYPE" is read-only!" );
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.Pack.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   L lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_pck_fld_iter( lua_State *L )
{
	struct t_pck     *pc  = t_pck_check_ud( L, lua_upvalueindex( 1 ), 1);
	struct t_pck_fld *r;

	// get current index and increment
	int crs = lua_tointeger( L, lua_upvalueindex( 2 ) ) + 1;

	luaL_argcheck( L, pc->t > T_PCK_RAW, lua_upvalueindex( 1 ),
	   "Attempt to index atomic T.Pack type" );

	if (crs > (int) pc->s)
		return 0;
	else
	{
		lua_pushinteger( L, crs );
		lua_replace( L, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m );//S: fnc nP _idx
	if (T_PCK_STR == pc->t)                        // Get the name for a Struct value
		lua_rawgeti( L, -1 , crs + pc->s*2 );   //S: fnc nP _idx nC
	else
		lua_pushinteger( L, crs );     // Stack: fnc iP _idx iC
	r = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ));
	lua_rawgeti( L, -3 , crs+pc->s ); //S: fnc xP _idx xC Rd ofs
	lua_rawgeti( L, -4 , crs );       //S: fnc xP _idx xC Rd ofs pack
	lua_remove( L, -5 );              //S: fnc xP xC Rd ofs pack

	r->pR = luaL_ref( L, LUA_REGISTRYINDEX );   //S: fnc xP xC Rd ofs
	r->o  = lua_tointeger( L, lua_upvalueindex( 3 ) ) + luaL_checkinteger( L, -1 );
	lua_pop( L, 1 );                  // remove ofs
	luaL_getmetatable( L, T_PCK_FLD_TYPE );
	lua_setmetatable( L, -2 );

	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.Pack.Struct.
 * \param   L lua Virtual Machine.
 * \lparam  iterator T.Pack.Struct.
 * \lreturn pos    position in t_buf.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
int
lt_pck_fld__pairs( lua_State *L )
{
	struct t_pck_fld *pf = NULL;
	t_pck_fld_getPackFromStack( L, -1, &pf );

	lua_pushnumber( L, 0 );
	lua_pushinteger( L, (NULL == pf) ? 0 : pf->o );  // preserve offset for iteration
	lua_pushcclosure( L, &t_pck_fld_iter, 3 );
	lua_pushvalue( L, -1 );
	lua_pushnil( L );
	return 3;
}

