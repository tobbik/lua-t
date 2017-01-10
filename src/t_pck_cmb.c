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
	size_t            n  = 0;  ///< iterator for going through the arguments
	size_t            o  = 0;  ///< byte offset within the sequence
	size_t            bo = 0;  ///< bit  offset within the sequence
	struct t_pck     *p;       ///< temporary packer/struct for iteration
	struct t_pck     *st;      ///< the userdata this constructor creates
	struct t_pck_fld *pf;      ///< userdata for current field

	st     = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	st->t  = T_PCK_STR;
	st->s  = (ep-sp) + 1;
	luaL_getmetatable( L, T_PCK_TYPE ); //S:… T.Pack
	lua_setmetatable( L, -2 ) ;

	// create and populate table
	lua_createtable( L, st->s, st->s ); // S:… Str tbl
	while (n < st->s)
	{
		luaL_argcheck( L, lua_istable( L, sp ), n+1,
			"Arguments must be tables with single key/"T_PCK_TYPE" pair" );
		// get key/value from table
		lua_pushnil( L );
		if (! lua_next( L, sp ))         //S:… Str tbl key Pck
			luaL_error( L, "The table argument must contain one key/value pair." );
		// check if key is already used!
		lua_remove( L, sp );             // remove the table now key/pck pair is on stack
		lua_pushvalue( L, -2 );          //S:… Str tbl key Pck key
		lua_rawget( L, -4 );             //S:… Str tbl key Pck nil?
		luaL_argcheck( L, lua_isnil( L, -1 ), ep-n-1, "Key duplicate in "T_PCK_TYPE".Struct." );
		lua_pop( L, 1 );                 //S:… Str tbl key Pck
		// create t_pck_fld
		p      = t_pck_getPacker( L, -1, &bo );  // allow T.Pack or T.Pack.Struct
		pf     = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ));
		luaL_getmetatable( L, T_PCK_FLD_TYPE );
		lua_setmetatable( L, -2 ) ;      //S:… Str tbl key Fld
		lua_insert( L, -2 );
		pf->pR = luaL_ref( L, LUA_REGISTRYINDEX );  // pops the packer from stack
		pf->o  = o/8;
		// populate tbl table
		lua_pushvalue( L, -2 );          //S:… Str tbl key Fld key
		lua_insert( L, -2 );             //S:… Str tbl key key Fld
		lua_rawset( L, -4 );             //S:… Str tbl key          tbl[ name  ] = Fld
		lua_rawseti( L, -2, n+1 );       //S:… Str tbl              tbl[ i ]     = nme
		o += t_pck_getSize( L, p, 1 );
		n++;
	}

	st->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register table
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
	size_t            n=0;    ///< iterator for going through the arguments
	size_t            o=0;    ///< byte offset within the sequence
	struct t_pck     *p;      ///< temporary packer/struct for iteration
	struct t_pck     *sq;     ///< the userdata this constructor creates
	struct t_pck_fld *pf;     ///< userdata for current field

	sq    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	sq->t = T_PCK_SEQ;
	sq->s = (ep-sp)+1;

	// create and populate index table
	lua_createtable( L, sq->s, sq->s ); //S: fmt … Seq tbl
	while (n < sq->s)
	{
		p      = t_pck_getPacker( L, sp, bo );
		pf     = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ));
		lua_pushvalue( L, sp );          //S: fmt … Seq tbl Fld Pck
		pf->pR = luaL_ref( L, LUA_REGISTRYINDEX );  // pops the packer from stack
		pf->o  = o/8;

		luaL_getmetatable( L, T_PCK_FLD_TYPE );
		lua_setmetatable( L, -2 );

		lua_rawseti( L, -2, n+1 );       //S: fmt … Seq tbl          tbl[i]   = Fld
		o += t_pck_getSize( L, p, 1 );
		n++;
		lua_remove( L, sp );
	}
	sq->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register table

	luaL_getmetatable( L, T_PCK_TYPE ); //S: … T.Pack
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
	size_t                                  bo = 0;
	struct t_pck  __attribute__ ((unused)) *p  = t_pck_getPacker( L, -2, &bo );  ///< packer
	struct t_pck     *ap;     ///< array userdata to be created

	ap    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	ap->t = T_PCK_ARR;
	ap->s = luaL_checkinteger( L, -2 );       // how many elements in array

	lua_pushvalue( L, -3 );                   //S: Pck n Arr Pck
	ap->m = luaL_ref( L, LUA_REGISTRYINDEX ); // record reference to packer

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
	struct t_pck_fld *opf  = NULL;  ///< Pack.Field to read from
	struct t_pck     *opc  = t_pck_fld_getPackFromStack( L, -2, &opf );
	struct t_pck_fld *ipf  = NULL;  ///< Pack.Field found at index
	struct t_pck     *ipc;
	struct t_pck_fld *npf;          ///< new Pack.Field to be returned

	luaL_argcheck( L, opc->t > T_PCK_RAW, -2, "Trying to index Atomic "T_PCK_TYPE" type" );

	if (LUA_TNUMBER == lua_type( L, -1 ) &&
	   ((luaL_checkinteger( L, -1 ) > (int) opc->s) || (luaL_checkinteger( L, -1 ) < 1))
	)
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( L );
		return 1;
	}
	// push empty field on stack
	npf    = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ));
	npf->o = (NULL == opf )? 0 : opf->o;  // recorded offset is 1 based -> don't add up
	// get idx table (struct) or packer type (array)
	lua_rawgeti( L, LUA_REGISTRYINDEX, opc->m );               // S: Str i/k ud tbl/Pck
	//t_stackDump( L );

	if (LUA_TUSERDATA == lua_type( L, -1 ))    // T.Array
	{
		ipc   = t_pck_check_ud( L, -1, 1 );                     // S: Arr i/k ud Pck
		if (T_PCK_BOL == ipc->t  || T_PCK_BTS == ipc->t  || T_PCK_BTU == ipc->t)
		{
			lua_pop( L, 1 );
			ipc = t_pck_create_ud( L, ipc->t, ipc->s,
				((ipc->s * (luaL_checkinteger( L, -2 )-1)) % NB ) );
		}
		npf->o += (((t_pck_getSize( L, ipc, 1 )) * (luaL_checkinteger( L, -3 )-1)) / NB);
	}
	else                                       // T.Pack.Struct/Sequence
	{
		if (T_PCK_SEQ == opc->t)
			lua_rawgeti( L, -1, luaL_checkinteger( L, -3 ) );   //S: Seq idx ud tbl Fld
		if (T_PCK_STR == opc->t)
		{
			if (lua_isinteger( L, -3 ) )
				lua_rawgeti( L, -1, luaL_checkinteger( L, -3) ); //S: Str idx ud tbl key
			else
				lua_pushvalue( L, -3 );                          //S: Str key ud tbl key
			lua_rawget( L, -2 );                                //S: Str i/k ud tbl Fld
		}
		lua_remove( L, -2 );                                   //S: Str i/k ud Fld
		ipc     = t_pck_fld_getPackFromStack( L, -1, &ipf );   //S: Str i/k ud Pck
		npf->o += ipf->o;
	}

	npf->pR  = luaL_ref( L, LUA_REGISTRYINDEX );              //S: Str i/k Fld
	luaL_getmetatable( L, T_PCK_FLD_TYPE );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Updating a packer value in an T.Pack.Struct/Sequence/Array is NOT ALLOWED.
 * \param   L     Lua state.
 * \lparam  ud    T.Pack.Struct/Sequence/Array instance.
 * \lparam  key   string/integer.
 * \lparam  value LuaType.
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
 *  -------------------------------------------------------------------------
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


//--------------------------------------------------------------------------
 * Pairs method to iterate over the T.Pack.Struct.
 * \param   L lua Virtual Machine.
 * \lparam  iterator T.Pack.Struct.
 * \lreturn pos    position in t_buf.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------
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
*/

/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.Pack.Struct.
 * It will return key,value pairs in properly ordered.
 * \param   L       Lua state.
 * \lparam  tbl/ud  referenced table with element or userdata T.Pack.
 * \lparam  value   previous key or index.
 * \lreturn mult    current key, current value.
 * \return  int     # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_pck_fld_iter( lua_State *L )
{
	int               idx = luaL_checkinteger( L, lua_upvalueindex( 1 ) ) + 1;
	int               len = luaL_checkinteger( L, lua_upvalueindex( 3 ) );
	enum t_pck_t      pct = luaL_checkinteger( L, lua_upvalueindex( 4 ) );
	int               itn = luaL_checkinteger( L, lua_upvalueindex( 5 ) );
	struct t_pck     *pc;
	struct t_pck_fld *pf;
	struct t_pck_fld *pcf;

	//check and update running index
	if (idx > len)
		return 0;
	else
	{
		lua_pushinteger( L, idx );
		lua_replace( L, lua_upvalueindex( 1 ) );
	}
	lua_pop( L, 1 );         // pop phony index  //S: tbl/pck
	if (itn)
		lua_pushinteger( L, idx );
	else
		lua_rawgeti( L, 1, idx );
	pf    = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ));
	pf->o = luaL_checkinteger( L, lua_upvalueindex( 2 ) );
	                                             //S: pck/tbl idx/key ud
	if (T_PCK_ARR == pct)
	{
		pc     = t_pck_check_ud( L, 1, 1 );
		pf->o +=  idx*pc->s;
		lua_pushvalue( L, 1 );                    //S: tbl idx ud pck
	}
	if (T_PCK_SEQ == pct)
	{
		lua_rawgeti( L, 1, idx );                 //S: tbl idx ud fld
		pc     = t_pck_fld_getPackFromStack( L, -1, &pcf );//S: tbl idx ud pck
		pf->o += pcf->o;
	}
	if (T_PCK_STR == pct)
	{
		lua_rawgeti( L, 1, idx );                 //S: tbl idx ud key
		lua_rawget( L, 1 );                       //S: tbl idx ud fld
		pc     = t_pck_fld_getPackFromStack( L, -1, &pcf );//S: tbl idx ud pck
		pf->o += pcf->o;
	}
	pf->pR = luaL_ref( L, LUA_REGISTRYINDEX );   //S: tbl/pck idx/key ud
	luaL_getmetatable( L, T_PCK_FLD_TYPE );
	lua_setmetatable( L, -2 );
	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.Pack.Struct.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack.Struct/Sequence/Array
 * \lreturn mult   iterfunction    tbl/pck    phonyindex.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
int
lt_pck_fld__pairs( lua_State *L )
{
	struct t_pck_fld *pf = NULL;
	struct t_pck     *pc = t_pck_fld_getPackFromStack( L, 1, &pf );

	luaL_argcheck( L, pc->t > T_PCK_RAW, 1,
	   "Attempt to index atomic T.Pack type" );

	lua_pushinteger( L, 0 );                                    //S: pck idx
	lua_pushinteger( L, (NULL == pf) ? 0 : pf->o );             //S: pck idx ofs
	lua_pushinteger( L, (int) pc->s );                          //S: pck idx ofs len
	lua_pushinteger( L, pc->t );                                //S: pck idx ofs len typ
	lua_pushinteger( L, T_PCK_ARR==pc->t || T_PCK_SEQ==pc->t ); //S: pck idx ofs len typ itt
	lua_pushcclosure( L, &t_pck_fld_iter, 5 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, pc->m );                 //S: pck fnc tbl/pck
	//lua_rawgeti( L, -1, 1                                     //S: pck fnc tbl/pck key
	lua_pushnil( L );                                           //S: pck fnc tbl/pck nil
	lua_remove( L, 1 );
	return 3;
}

