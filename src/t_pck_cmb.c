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

#include "t_pck_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Create a t.Pack.Struct Object and put it onto the stack.
 * \param   L      Lua state.
 * \lparam  table  T.OrderedHashTable instance with packer definitions.
 * \return  struct t_pck*.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_str_create( lua_State *L )
{
	size_t            n;       ///< iterator for going through the arguments
	size_t            bo = 0;  ///< bit  offset within the sequence
	struct t_pck     *p;       ///< temporary packer/struct for iteration
	struct t_pck     *ps;      ///< the packer instance to be created
	struct t_pck_fld *pf;      ///< userdata for current field

	for (n=0; n<lua_rawlen( L, -1 ); n++)
	{
		lua_rawgeti( L, -1, n+1 );                 //S:… tbl key
		pf     = t_pck_fld_create_ud( L, bo );     //S:… tbl key Fld
		lua_rawgeti( L, -3, n+1 );                 //S:… tbl key Fld key
		lua_rawget( L, -4 );                       //S:… tbl key Fld Pck
		p      = t_pck_getPacker( L, -1 );         // turn Pck into true packer
		pf->pR = luaL_ref( L, LUA_REGISTRYINDEX ); // pops the packer from stack
		bo    += t_pck_getSize( L, p );
		// replace pack with new Pack.Field
		lua_rawset( L, -3 );                       //S:… tbl
	}

	ps    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	luaL_getmetatable( L, T_PCK_TYPE );           //S:… T.Pack
	lua_setmetatable( L, -2 ) ;
	ps->t = T_PCK_STR;
	ps->s = lua_rawlen( L, -2 );                  //S:… tbl pck
	lua_insert( L, -2 );                          //S:… pck tbl
	ps->m = luaL_ref( L, LUA_REGISTRYINDEX );     // register table

	return ps;
}


/**--------------------------------------------------------------------------
 * Create a t.Pack.Sequence Object and put it onto the stack.
 * \param   L      Lua state.
 * \param   int    sp start position on Stack for first Packer.
 * \param   int    ep   end position on Stack for last Packer.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_seq_create( lua_State *L, int sp, int ep )
{
	size_t            bo = 0; ///< bit offset within the sequence
	struct t_pck     *p;      ///< temporary packer/struct for iteration
	struct t_pck     *sq;     ///< the userdata this constructor creates
	struct t_pck_fld *pf;     ///< userdata for current field

	sq    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ) );
	sq->t = T_PCK_SEQ;
	sq->s = (ep-sp)+1;

	// create and populate index table
	lua_createtable( L, sq->s, 0 ); //S: p1 p2 … pn … Seq tbl
	while (lua_rawlen( L, -1 ) < sq->s)
	{
		p      = t_pck_getPacker( L, sp );
		pf     = t_pck_fld_create_ud( L, bo );
		lua_pushvalue( L, sp );      //S: p1 p2 … pn … Seq tbl Fld Pck
		pf->pR = luaL_ref( L, LUA_REGISTRYINDEX );  // pops the packer from stack

		luaL_getmetatable( L, T_PCK_FLD_TYPE );
		lua_setmetatable( L, -2 );   //S: p1 p2 … pn … Seq tbl Fld

		lua_rawseti( L, -2, lua_rawlen( L, -2 )+1 ); // tbl[i] = Fld
		bo += t_pck_getSize( L, p );
		lua_remove( L, sp );
	}
	sq->m = luaL_ref( L, LUA_REGISTRYINDEX ); // register table

	luaL_getmetatable( L, T_PCK_TYPE ); //S: … T.Pack
	lua_setmetatable( L, -2 ) ;

	return sq;
}


/**--------------------------------------------------------------------------
 * Create a t.Pack.Array Object and put it onto the stack.
 * \param   L          Lua state.
 * \lparam  ud/string  T.Pack, T.Pack.Struct, T.Pack.Array or format string.
 * \lparam  integer    Number of elements in the array.
 * \return  struct     t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_arr_create( lua_State *L )
{
	struct t_pck  __attribute__ ((unused)) *p  = t_pck_getPacker( L, -2 );  ///< packer
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
 * Create a t.Pack.Field Object and put it onto the stack.
 * \param   L      Lua state.
 * \param   int    ofs offset in byte.
 * \return  struct t_pck_fld*.
 * --------------------------------------------------------------------------*/
struct t_pck_fld
*t_pck_fld_create_ud( lua_State *L, int ofs )
{
	struct t_pck_fld *pf;      ///< userdata for packer field

	pf    = (struct t_pck_fld *) lua_newuserdata( L, sizeof( struct t_pck_fld ) );
	luaL_getmetatable( L, T_PCK_FLD_TYPE );
	lua_setmetatable( L, -2 ) ;                //S:… tbl key Fld
	pf->o  = ofs;
	pf->pR = LUA_REFNIL;
	pf->bR = LUA_REFNIL;
	return pf;
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
	struct t_pck_fld *ppf  = NULL;  ///< Pack.Field to read from (parent)
	struct t_pck     *ppc  = t_pck_fld_getPackFromStack( L, 1, &ppf );
	struct t_pck_fld *ipf  = NULL;  ///< Pack.Field found at index
	struct t_pck     *ipc;          ///< Pack found at index or referenced from ipf
	struct t_pck_fld *npf;          ///< new Pack.Field to be returned
	size_t            idx;          ///< index of requested field

	luaL_argcheck( L, ppc->t > T_PCK_RAW, 1, "Trying to index Atomic "T_PCK_TYPE" type" );
	luaL_argcheck( L, (ppc->t < T_PCK_STR && LUA_TNUMBER == lua_type( L, 2 )) || ppc->t == T_PCK_STR,
		2, "Index for "T_PCK_TYPE".Array or "T_PCK_TYPE".Sequence must be numeric." );

	if (LUA_TNUMBER == lua_type( L, 2 ) && (idx = luaL_checkinteger( L, 2 )) && (idx > ppc->s || idx<1))
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( L );
		return 1;
	}

	// push empty field on stack
	npf     = t_pck_fld_create_ud( L, (NULL == ppf )? 0 : ppf->o );
	// get table (struct,sequence) or packer type (array)
	lua_rawgeti( L, LUA_REGISTRYINDEX, ppc->m );               //S: ??? i/k ud tbl/Pck

	switch (ppc->t)
	{
		case T_PCK_ARR:
			ipc     = t_pck_check_ud( L, -1, 1 );                //S: Arr i/k ud Pck
			npf->o += t_pck_getSize( L, ipc ) * (idx-1) ;
			break;
		case T_PCK_SEQ:
			lua_rawgeti( L, -1, idx );                           //S: Seq idx ud tbl Fld
			lua_remove( L, -2 );                                 //S: Seq idx ud Fld
			ipc     = t_pck_fld_getPackFromStack( L, -1, &ipf ); //S: Seq idx ud Pck
			npf->o += ipf->o;
			break;
		case T_PCK_STR:
			if (lua_isinteger( L, 2 ) )
				lua_rawgeti( L, -1, idx );                        //S: Str idx ud tbl key
			else
				lua_pushvalue( L, 2 );                            //S: Str key ud tbl key
			lua_rawget( L, -2 );                                 //S: Str i/k ud tbl Fld
			lua_remove( L, -2 );                                 //S: Str i/k ud Fld
			ipc     = t_pck_fld_getPackFromStack( L, -1, &ipf ); //S: Str i/k ud Pck
			npf->o += ipf->o;
			break;
		//case T_PCK_FNC:
			// TODO: To be implemented
		//	break;
		default:
			break;
	}

	npf->pR  = luaL_ref( L, LUA_REGISTRYINDEX );               //S: ??? i/k Fld
	if (ppf)     // if parent is pack.field add reference
	{
		lua_pushvalue( L, 1 );                                  //S: ??? i/k Fld ???
		npf->bR  = luaL_ref( L, LUA_REGISTRYINDEX );            //S: ??? i/k Fld
	}
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
	return luaL_error( L, T_PCK_TYPE" is read-only!" );
}


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
	struct t_pck_fld *pf;         ///< New T.Pack.Field to be returned
	struct t_pck_fld *pcf;        ///< T.Pack.Field read from current idx

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
	pf = t_pck_fld_create_ud( L, luaL_checkinteger( L, lua_upvalueindex( 2 ) ) );
	                                             //S: pck/tbl idx/key ud
	if (T_PCK_ARR == pct)
	{
		pc     = t_pck_check_ud( L, 1, 1 );
		pf->o += (idx-1) * pc->s;
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

