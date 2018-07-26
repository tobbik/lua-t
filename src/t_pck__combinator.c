/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck__combinator.c
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


static struct t_pck_idx
*t_pck_idx_create_ud( lua_State *L, size_t idx, int p_ref )
{
	struct t_pck_idx  __attribute__ ((unused)) *pci;

	pci      = (struct t_pck_idx *) lua_newuserdata( L, sizeof( struct t_pck_idx ));
	pci->idx = idx;
	pci->pR  = p_ref;
	luaL_getmetatable( L, T_PCK_IDX_TYPE );
	lua_setmetatable( L, -2 );
	return pci;
}


/**--------------------------------------------------------------------------
 * Get T.Pack from a stack element at specified position.
 * The item@pos can be a t_pck or a t_pck_idx.  The way the function depends on
 * to conditions:
 *        - item @pos is a t_pck_idx or a t_pck
 *        - arg **pcf is NULL or points to a *t_pck_idx
 * The behaviour is as follows:
 * - if item @pos is t_pck:
 *   - returns a pointer to that userdata
 * - if item @pos is t_pck_idx
 *   - return pointer to t_pck referenced in t_pck_idx
 *   - item @pos in stack will be replaced by referenced t_pck instance
 * - if item @pos is t_pck_idx and arg **pcf!=NULL
 *   - **pcf will point to t_pck_idx instance
 * \param   *L      Lua state.
 * \param    pos    int; position on Lua stack.
 * \param  **pcf    struct** pointer to t_pck_idx pointer.
 * \return  *pck    struct*  pointer to t_pck.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_idx_getPackFromFieldOnStack( lua_State * L, int pos, struct t_pck_idx **pcir, int los )
{
	int               o_top = lua_gettop( L );
	void             *ud    = luaL_testudata( L, pos, T_PCK_IDX_TYPE );
	struct t_pck_idx *pi    = (NULL == ud) ? NULL : (struct t_pck_idx *) ud;
	int               idx;
	struct t_pck_idx *pci;
	struct t_pck     *pck;

	lua_pushvalue( L, pos );
	if (NULL != pcir)
		*pcir = pi;
	//t_stackDump( L );
	// as long as pci->pR is an index
	while (NULL != (ud = luaL_testudata( L, -1, T_PCK_IDX_TYPE )))
	{
		pci = (struct t_pck_idx *) ud;
		lua_pop( L, 1 );                              // pop the pci from stack
		lua_pushinteger( L, pci->idx );
		lua_rawgeti( L, LUA_REGISTRYINDEX, pci->pR ); // new reference on stack
		printf("traversing\n"); t_stackDump( L );
	}

	pck   = t_pck_check_ud( L, -1, 1 );         //S:… pci … x y z pck
	lua_pop( L, 1 );
	//printf ( "%d   %d  ---  ", lua_gettop(L), o_top );
	//t_stackDump( L );
	while (lua_gettop( L ) > o_top)
	{
		idx = luaL_checkinteger( L, -1 );        // last pickled index
		lua_pop( L, 1 );                         //S:… pci … x y 
		lua_rawgeti( L, LUA_REGISTRYINDEX, pck->m );
		if (T_PCK_SEQ == pck->t)
			lua_rawgeti( L, -1, idx );
		if (T_PCK_STR == pck->t)
		{
			lua_rawgeti( L, -1, idx );
			lua_rawget( L, -2 );                  //S:… pci … x y pck
			printf("STRUCT: ");t_stackDump( L );
		}
		pck = t_pck_check_ud( L, -1, 1 );
		lua_pop( L, 2 );
		t_stackDump( L );
	}
	//t_stackDump( L );
	if (los)
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, pck->m );
		lua_replace( L, pos );
	}
	//t_stackDump( L );printf( "done\n" );
	return pck;
}


/**--------------------------------------------------------------------------
 * Create a t.Pack.Array instance and put it onto the stack.
 * \param  *L          Lua state.
 * \lparam  ud/string  T.Pack.* instance or format string.
 * \lparam  sz        int; Number of elements in the array.
 * \return  struct    t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_arr_create( lua_State *L )
{
	size_t  sz = luaL_checkinteger( L, -1 );

	lua_pop( L, 1 );           //S: PCK
	t_pck_getPacker( L, -1 );  // convert into a real packer

	return t_pck_create_ud( L, T_PCK_ARR, sz, luaL_ref( L, LUA_REGISTRYINDEX ) );
}


/**--------------------------------------------------------------------------
 * Create a t.Pack.Sequence instance and put it onto the stack.
 * Expects the packers as stack element located between sp and ep.
 * \param   L      Lua state.
 * \param   int    sp start position on Stack for first Packer.
 * \param   int    ep   end position on Stack for last  Packer.
 * \return  struct t_pck* pointer.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_seq_create( lua_State *L, int sp, int ep )
{
	//struct t_pck *p;              ///< temporary packer/struct for iteration
	size_t        sz = (ep-sp)+1; ///< size of Sequence

	lua_createtable( L, sz, 0 );      //S: … p1 p2 … pn … tbl

	// populate index table
	//t_stackDump(L);
	while (sz--)
	{
		lua_rotate( L, sp, -1 );       //S: … p2 … pn … Seq tbl p1
		//p = t_pck_getPacker( L, -1 );
		t_pck_getPacker( L, -1 );
		//printf("%d %d %zu    ", p->t, p->m, p->s );t_stackDump(L);
		lua_rawseti( L, -2, lua_rawlen( L, -2 )+1 ); // tbl[ i ] = Pck
	}
	return t_pck_create_ud( L, T_PCK_SEQ, lua_rawlen( L, -1 ), luaL_ref( L, LUA_REGISTRYINDEX ) );
}


/**--------------------------------------------------------------------------
 * Create a t.Pack.Struct instance and put it onto the stack.
 * Expects table on stack which has key/packer structure already.  Just turning
 * the packers into real packers. (convert formatstrings etc ...
 * \param   L      Lua state.
 * \lparam  table  T.OrderedHashTable instance with key/packer definitions.
 * \return  struct t_pck*.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_str_create( lua_State *L )
{
	size_t         n;      ///< iterator for going through the arguments

	for (n=0; n<lua_rawlen( L, -1 ); n++)
	{
		lua_rawgeti( L, -1, n+1 );                 //S:… tbl key
		lua_pushvalue( L, -1 );                    //S:… tbl key key
		lua_rawget( L, -3 );                       //S:… tbl key pck
		t_pck_getPacker( L, -1 );                  // turn Pck into true packer
		lua_rawset( L, -3 );                       //S:… tbl
	}
	return t_pck_create_ud( L, T_PCK_STR, lua_rawlen( L, -1), luaL_ref( L, LUA_REGISTRYINDEX ) );
}


/**--------------------------------------------------------------------------
 * Get the offset of a certain Pack.Index.
 * \param   *L      Lua state.
 * \param    pos    int; position on Lua stack.
 * \param  **pcf    struct** pointer to t_pck_idx pointer.
 * \return  *pck    struct*  pointer to t_pck.
 * --------------------------------------------------------------------------*/
size_t
t_pck_idx_getOffset( lua_State *L, struct t_pck_idx *pci )
{
	struct t_pck      *pkc;     ///< Container pck
	//struct t_pck      *pck;     ///< Runner pck
	size_t             ofs = 0;
	size_t             n, n_sz;

	// get parent, can only be Array,Sequence,Struct or another Index?
	lua_rawgeti( L, LUA_REGISTRYINDEX, pci->pR ); //S:… pkc
	pkc = t_pck_check_ud( L, -1, 1 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, pkc->m );  //S:… pkc tbl/pck
	switch (pkc->t)
	{
		case T_PCK_ARR:
			n_sz = t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
			if (n_sz)
				ofs  = (pci->idx - 1)  *  t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
			break;
		case T_PCK_SEQ:
		case T_PCK_STR:
			for (n = 1; n < pci->idx; n++) // does not run for pci-idx==1 !
			{
				lua_rawgeti( L, -1, n );             //S:… pkc tbl key/pck
				if (T_PCK_STR == pkc->t)
					lua_rawget( L, -2);               //S:… pkc tbl pck
				//t_pck_check_ud( L, -1, 1 );          //S:… pkc tbl pck
				n_sz = t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
				if (n_sz)
					ofs += n_sz;
				else
				{
					// TODO: probably here read with buffer to get values from payload
					ofs = 0;
					n   = pkc->s; // break for loop; still clean up stack
				}
				lua_pop( L, 1 );
			}
			break;
		default:
			break;   // ofs = 0 (default)
	}
	lua_pop( L, 2 );
	return ofs;
}


/**--------------------------------------------------------------------------
 * Read a t.Array/Sequence/Struct/Function packer value.
 * \param   L    Lua state.
 * \lparam  ud   T.Pack.* userdata instance.
 * \lparam  key  string/integer.
 * \lreturn ud   T.Pack.Field instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_pck__index( lua_State *L )
{
	//struct t_pck_idx *pci = NULL;  ///< Pack.Index to read from (parent)
	//struct t_pck     *pck = t_pck_idx_getPackFromStack( L, 1, &pci );
	printf("IDX_ACCESS:  "); t_stackDump(L);
	struct t_pck     *pck = t_pck_idx_getPackFromFieldOnStack( L, 1, NULL, 0 );
	size_t            idx = 0;     ///< index of requested field
	size_t            i,n;         ///< iterators to figure out index

	printf("__INDEX:  "); t_stackDump(L);
	luaL_argcheck( L, pck->t > T_PCK_FNC, 1, "can't index Atomic "T_PCK_TYPE" type" );
	luaL_argcheck( L, (pck->t < T_PCK_STR && LUA_TNUMBER == lua_type( L, 2 )) || pck->t == T_PCK_STR,
		2, "Index for "T_PCK_TYPE".Array or "T_PCK_TYPE".Sequence must be numeric." );

	if (LUA_TNUMBER == lua_type( L, 2 ) && (idx = luaL_checkinteger( L, 2 )) && (idx > pck->s || idx<1))
	{
		// Array/Sequence out of bound: return nil
		lua_pushnil( L );
		return 1;
	}

	// get idx from struct
	if (T_PCK_STR == pck->t && ! lua_isinteger( L, 2 ))
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, pck->m );      //S: Str key tbl
		lua_pushvalue( L, 2 );                            //S: Str key tbl key
		lua_rawget( L, -2 );                              //S: Str key tbl pck/nil
		if (! lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );                               //S: Str key tbl
			n = lua_rawlen( L, -1 );
			for (i=0; i<n; i++)
			{
				lua_rawgeti( L, -1, i+1 );                  //S: Str key tbl keyN
				if (lua_rawequal( L, -1, -3 ))
				{
					idx = i+1;
					i   = n+1;  // shortcut the loop; still lua_pop the key
				}
				lua_pop( L, 1 );            // pop keyN
			}
		}
		else
			lua_pop( L, 1 );                               //S: Str key tbl (pop nil)
		lua_pop( L, 1 );                                  //S: Str key
	}
	lua_pop( L, 1 );                                     //S: Pck

	if (0 != idx)
		//pci = t_pck_idx_create_ud( L, idx, luaL_ref( L, LUA_REGISTRYINDEX ) );
		t_pck_idx_create_ud( L, idx, luaL_ref( L, LUA_REGISTRYINDEX ) );
	else
		lua_pushnil( L );
	//printf( "GET FIELD[%zu]: ", idx); t_stackDump(L);
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
lt_pck__newindex( lua_State *L )
{
	struct t_pck *pc = t_pck_idx_getPackFromStack( L, 1, NULL );
	luaL_argcheck( L, pc->t > T_PCK_FNC, 1, "Atomic "T_PCK_TYPE" type has no fields" );
	return luaL_error( L, T_PCK_TYPE" is read-only!" );
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the t.Pack.Struct.
 * It will return key,value pairs in properly ordered.
 * \param   *L       Lua state.
 * \lparam   tbl/ud  referenced table with element or userdata T.Pack.
 * \lparam   value   previous key or index.
 * \lreturn  mult    current key, current value.
 * \return   int     # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_pck_iter( lua_State *L )
{
	int               idx = luaL_checkinteger( L, lua_upvalueindex( 1 ) ) + 1;
	int               len = luaL_checkinteger( L, lua_upvalueindex( 3 ) );
	enum t_pck_t      pct = luaL_checkinteger( L, lua_upvalueindex( 4 ) );
	int               itn = luaL_checkinteger( L, lua_upvalueindex( 5 ) );
	//struct t_pck     *pck;
	//struct t_pck_idx *pci;          ///< New Pack.Index to be returned
	//struct t_pck_idx *pci_r;        ///< Pack.Index read from current idx

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
	                                             //S: pck/tbl idx/key
	if (T_PCK_ARR == pct)
	{
		//pck     = t_pck_check_ud( L, 1, 1 );
		t_pck_check_ud( L, 1, 1 );
		//pf->o += (idx-1) * pck->s;
		lua_pushvalue( L, 1 );                    //S: tbl idx pck
	}
	if (T_PCK_SEQ == pct)
	{
		lua_rawgeti( L, 1, idx );                 //S: tbl idx pck
		//pck     = t_pck_fld_getPackFromStack( L, -1, &pci_r );//S: tbl idx pck
	}
	if (T_PCK_STR == pct)
	{
		lua_rawgeti( L, 1, idx );                 //S: tbl idx key
		lua_rawget( L, 1 );                       //S: tbl idx pck
		//pck     = t_pck_fld_getPackFromStack( L, -1, &pci_r );//S: tbl idx ud pck
	}
	t_pck_idx_create_ud( L, idx, luaL_ref( L, LUA_REGISTRYINDEX ) );
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
lt_pck__pairs( lua_State *L )
{
	struct t_pck_idx *pci = NULL;
	struct t_pck     *pck = t_pck_idx_getPackFromStack( L, 1, &pci );

	luaL_argcheck( L, pck->t > T_PCK_FNC, 1,
	   "can't index atomic T.Pack type" );

	lua_pushinteger( L, 0 );                                      //S: pck idx
	lua_pushinteger( L, (NULL == pci) ? 0 : t_pck_idx_getOffset( L, pci ) );             //S: pck idx ofs
	lua_pushinteger( L, (int) pck->s );                           //S: pck idx ofs len
	lua_pushinteger( L, pck->t );                                 //S: pck idx ofs len typ
	lua_pushinteger( L, T_PCK_ARR==pck->t || T_PCK_SEQ==pck->t ); //S: pck idx ofs len typ itt
	lua_pushcclosure( L, &t_pck_iter, 5 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, pck->m );                  //S: pck fnc tbl/pck
	//lua_rawgeti( L, -1, 1                                       //S: pck fnc tbl/pck key
	lua_pushnil( L );                                             //S: pck fnc tbl/pck nil
	lua_remove( L, 1 );
	return 3;
}

