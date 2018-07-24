/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck.c
 * \brief     OOP wrapper for Packer definitions
 *            Allows for packing/unpacking numeric values to binary streams
 *            can work stand alone or as helper for Combinators
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */
#include <string.h>     // memcpy

#include "t_pck_l.h"
#include "t_buf.h"
#include "t.h"          // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif


/* #########################################################################
 *   _                      _          _
 *  | |_ _   _ _ __   ___  | |__   ___| |_ __   ___ _ __ ___
 *  | __| | | | '_ \ / _ \ | '_ \ / _ \ | '_ \ / _ \ '__/ __|
 *  | |_| |_| | |_) |  __/ | | | |  __/ | |_) |  __/ |  \__ \
 *   \__|\__, | .__/ \___| |_| |_|\___|_| .__/ \___|_|  |___/
 *       |___/|_|                       |_|
 *  ######################################################################## */
/**--------------------------------------------------------------------------
 * __tostring helper that prints the packer type.
 * \param   L       Lua state.
 * \param   enum    t_pck_typ T.Pack type.
 * \param   size_t  length of packer in bits or bytes (type depending).
 * \param   int     modifier (packer type dependent).
 * \lreturn string  string describing packer.
 * --------------------------------------------------------------------------*/
void
t_pck_format( lua_State *L, enum t_pck_t t, size_t s, int m )
{
	lua_pushfstring( L, "%s", t_pck_t_lst[ t ] );
	switch( t )
	{
		case T_PCK_BOL:
			lua_pushfstring( L, "" );
			break;
		case T_PCK_INT:
			lua_pushfstring( L, "%d%c%c", s,
			                 (T_PCK_ISSIGNED( m )) ? 's' : 'u',
			                 (T_PCK_ISLITTLE( m )) ? 'l' : 'b' );
			break;
		case T_PCK_FLT:
			lua_pushfstring( L, "%d%c", s,
			                 (T_PCK_ISLITTLE( m )) ? 'l' : 'b' );
			break;
		case T_PCK_RAW:
			lua_pushfstring( L, "%d", s/NB );
			break;
		case T_PCK_ARR:
		case T_PCK_SEQ:
		case T_PCK_STR:
			lua_pushfstring( L, "[%d]", s);
			break;
		case T_PCK_FNC:
			lua_pushfstring( L, "" );
			break;
		default:
			lua_pushfstring( L, "UNKNOWN" );
	}
	lua_concat( L, 2 );
}


/**--------------------------------------------------------------------------
 * Create userdata struct for T.Pack.
 * Checks first if requested type already exists exists in T.Pack.  Otherwise
 * create and register a new Type.  The format for a particular definition will
 * never change.  Hence there is no need to create multiple copies.  This
 * approach saves memory.
 * \param   L      Lua state.
 * \param   enum   t_pck_t.
 * \param   size   number of elements.
 * \param   mod    parameter.
 * \return  struct t_pck* pointer. Create a t_pack and push to LuaStack.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_create_ud( lua_State *L, enum t_pck_t t, size_t s, int m )
{
	struct t_pck  __attribute__ ((unused)) *p;

	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( L, -1, "t."T_PCK_IDNT );
	t_pck_format( L, t, s, m );
	lua_rawget( L, -2 );             //S: _ld t.pck pck/nil
	if (lua_isnil( L, -1 ))          // combinator or not in cache -> create it
	{
		lua_pop( L, 1 );              // pop nil
		p    = (struct t_pck *) lua_newuserdata( L, sizeof( struct t_pck ));
		p->t = t;
		p->s = s;
		p->m = m;

		luaL_getmetatable( L, T_PCK_TYPE );
		lua_setmetatable( L, -2 );
		if (t < T_PCK_FNC)           // register atomic cacheable types only
		{
			t_pck_format( L, t, s, m );
			lua_pushvalue( L, -2 );   //S: 'i'_ld t.pck pck fmt pck
			lua_rawset( L, -4 );
		}
	}
	p = t_pck_check_ud( L, -1, 1 ); //S:'i' _ld t.pck pck
	lua_rotate( L, -3, 1 );
	lua_pop( L, 2 );

	return p;                       //S: pck
}


/**--------------------------------------------------------------------------
 * Check if value on stack is T.Pack OR * T.Pack.Struct/Sequence/Array
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \param   int    check -> treats as check -> error if fail
 * \lparam  ud     T.Pack/Struct on the stack.
 * \return  t_pck* pointer to t_pck.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_PCK_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_PCK_TYPE );
	return (NULL==ud) ? NULL : (struct t_pck *) ud;
}


/**--------------------------------------------------------------------------
 * Get the size of a packer of any type in bits.
 * This will mainly be needed to calculate offsets when reading.  It reads
 * recursively over Structures, Sequences and Arrays.
 * \param   L       Lua state.
 * \param   struct* t_pck.
 * \return  int     size in bits.
 * TODO: return 0 no matter if even one item is of unknown length.
 * --------------------------------------------------------------------------*/
size_t
t_pck_getSize( lua_State *L, struct t_pck *p )
{
	size_t  s, ns;  ///< size accumulator for complex types
	size_t  n;      ///< iterator over complex types

	if (0 == p->s  ||  p->t == T_PCK_FNC  ||  p->t < T_PCK_ARR)
		return p->s;
	else
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, p->m ); // get packer or table
		switch (p->t)
		{
			case T_PCK_ARR:
				s = p->s * t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
				break;
			case T_PCK_SEQ:
			case T_PCK_STR:
				s = 0;
				for (n = 0; n < p->s; n++)
				{
					lua_rawgeti( L, -1, n+1 );    // get packer or key from table
					if (T_PCK_STR == p->t)
						lua_rawget( L, -2 );
					//t_pck_idx_getPackFromStack( L, -1, NULL );
					t_pck_idx_getPackFromFieldOnStack( L, -1, NULL, 0 );
					ns = t_pck_getSize( L, t_pck_check_ud( L, -1, 1 ) );
					if (ns)
						s += ns;
					else
					{
						s = 0;
						n = p->s; // break for loop; still clean up stack
					}
					lua_pop( L, 1 );
				}
				break;
			default:
				s = p->s;
		}
		lua_pop( L, 1 );                           // pop packer or table
	}
	return s;
}


/**--------------------------------------------------------------------------
 * Get T.Pack from a stack element at specified position.
 * The item@pos can be a t_pck or a t_pck_idx.  The way the function works
 * depends on two conditions:
 *        - item @pos is a t_pck_idx or a t_pck
 *        - arg **pcf is NULL or points to a *t_pck_idx
 * The behaviour is as follows:
 * - if item @pos is t_pck:
 *   - returns a pointer to that userdata
 * - if item @pos is t_pck_idx
 *   - return pointer to t_pck reference in t_pck_idx
 *   - item @pos in stack will be replaced by referenced t_pck instance
 * - if item @pos is t_pck_idx and arg **pcf!=NULL
 *   - **pcf will point to t_pck_idx instance
 * \param   *L      Lua state.
 * \param    pos    int; position on Lua stack.
 * \param  **pcf    struct** pointer to t_pck_idx pointer.
 * \return  *pck    struct*  pointer to t_pck.
 * --------------------------------------------------------------------------*/
struct t_pck
*t_pck_idx_getPackFromStack( lua_State * L, int pos, struct t_pck_idx **pci )
{
	void             *ud  = luaL_testudata( L, pos, T_PCK_IDX_TYPE );
	struct t_pck_idx *pi  = (NULL == ud) ? NULL : (struct t_pck_idx *) ud;
	struct t_pck     *pck;

	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	if (NULL == pi)
		return t_pck_check_ud( L, pos, 1 );
	else
	{
		if (NULL != pci)
			*pci = pi;
		// get parent-container; could only be a Array,Sequence,Struct ...
		lua_rawgeti( L, LUA_REGISTRYINDEX, pi->pR ); //S:… pi … pcp
		pck = t_pck_check_ud( L, -1, 1 );   // get reference; pck or tbl
		lua_rawgeti( L, LUA_REGISTRYINDEX, pck->m ); //S:… pi … pcp ref
		if (pck->t > T_PCK_ARR)                      //S:… pi … pcp tbl
		{
			lua_rawgeti( L, -1, pi->idx );            //S:… pi … pcp tbl pck
			if (pck->t == T_PCK_STR)                  //S:… pi … pcp tbl key
				lua_rawget( L, -2 );                   //S:… pi … pcp tbl pck
			lua_remove( L, -2 );                      //S:… pi … pcp pck
		}
		lua_replace( L, pos );                       //S:… pck … pcp
		lua_pop( L, 1 );
		pck = t_pck_check_ud( L, pos, 1 );
		return pck;
	}
}


/**--------------------------------------------------------------------------
 * Read all arguments from Stack.
 * Creates table like this: {
 *         [1]  = 'key1',
 *         [2]  = 'key2',
 *         key1 = pck1,
 *         key2 = pck2      }
 * \param   *L      Lua state.
 * \param    sp     int; start position; stack index for first parameter table.
 * \param    ep     int; end position;   stack index for last  parameter table.
 * \lparam   mult   Sequence of tables with one key/value pair.
 * \lreturn  table  Table filled according to OrderedHashTable structure.
 * \return   void.
 * --------------------------------------------------------------------------*/
static void
t_pck_readArguments( lua_State *L, int sp, int ep )
{
	size_t  i  = 0;         ///< iterator for going through the arguments
	size_t  n  = ep-sp + 1; ///< process how many arguments

	lua_createtable( L, n, n );
	while (i < n)
	{
		luaL_argcheck( L, lua_istable( L, sp ), i+1,
			"Each argument must be a table" );
		// get key/value from table
		lua_pushnil( L );           //S: sp … ep … tbl nil
		luaL_argcheck( L, lua_next( L, sp ), ep-n-1,
			"The table argument must contain a single key/value pair." );
		lua_remove( L, sp );        // remove the table now key/pck pair is on stack
		lua_pushvalue( L, -2 );     //S: sp … ep … tbl key val key
		lua_rawget( L, -4 );        //S: sp … ep … tbl key val valold?
		if (lua_isnil( L, -1 ))     // add a new value to the table
		{
			lua_pop( L, 1 );         // pop nil
			lua_pushvalue( L, -2 );  //S: sp … ep … tbl key val key
			lua_rawseti( L, -4, lua_rawlen( L, -4 )+1 );
			lua_rawset( L, -3 );     //S: sp … ep … tbl
		}
		else
			luaL_error( L, "No duplicates for Pack keys allowed" );

		i++;
	}
}


//###########################################################################
//   ____                _                   _
//  / ___|___  _ __  ___| |_ _ __ _   _  ___| |_ ___  _ __
// | |   / _ \| '_ \/ __| __| '__| | | |/ __| __/ _ \| '__|
// | |__| (_) | | | \__ \ |_| |  | |_| | (__| || (_) | |
//  \____\___/|_| |_|___/\__|_|   \__,_|\___|\__\___/|_|
//###########################################################################

/** -------------------------------------------------------------------------
 * Constructor for T.Pack.
 * Behaves differently based on arguments.
 * \param   L      Lua state.
 * \lparam  CLASS  table T.Pack.
 * \lparam  fmt    string.
 *      OR
 * \lparam  tbl,…  {name=T.Pack},… name value pairs.
 *      OR
 * \lparam  T.Pack elements.    copy constructor
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int lt_pck__Call( lua_State *L )
{
	lua_remove( L, 1 );                  // remove the t.Pack Class table
	switch (lua_type( L, 2 ))
	{
		case LUA_TTABLE:
			t_pck_readArguments( L, 1, lua_gettop( L ) );
			t_pck_str_create( L );         //S: pck tbl
			break;
		case LUA_TFUNCTION:
			t_pck_create_ud( L, T_PCK_FNC, 0, luaL_ref( L, LUA_REGISTRYINDEX ) );
			break;
		default:
			if (1==lua_gettop( L ))
				t_pck_getPacker( L, 1 );    // single packer or sequence from string
			else
				if (lua_isinteger( L, 2 ))
					t_pck_arr_create( L );   // if second is number it must be array
				else
					t_pck_seq_create( L, 1, lua_gettop( L ) );
			break;
	}
	return 1;
}


/* ############################################################################
 *    ____ _                                _   _               _
 *   / ___| | __ _ ___ ___   _ __ ___   ___| |_| |__   ___   __| |___
 *  | |   | |/ _` / __/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 *  | |___| | (_| \__ \__ \ | | | | | |  __/ |_| | | | (_) | (_| \__ \
 *   \____|_|\__,_|___/___/ |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
 * ######################################################################### */
/**--------------------------------------------------------------------------
 * Get size in bytes covered by packer/struct/reader.
 * \param   L    Lua state.
 * \lparam  ud   T.Pack.* instance.
 * \lreturn int  size in bytes.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_GetSize( lua_State *L )
{
	//struct t_pck *p  = t_pck_idx_getPackFromStack( L, 1, NULL );
	struct t_pck *p  = t_pck_idx_getPackFromFieldOnStack( L, 1, NULL, 0 );
	size_t        sz = t_pck_getSize( L, p );
	lua_pushinteger( L, sz/NB );   // size in bytes
	lua_pushinteger( L, sz );      // size in bits
	return 2;
}


/**--------------------------------------------------------------------------
 * Get offset of a Pack.Field.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack.Field userdata instance.
 * \lreturn bytes  offset from beginning in bytes.
 * \lreturn bits   offset from beginning in bits.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_idx_GetOffset( lua_State *L )
{
	struct t_pck_idx *pci = NULL;
	size_t            ofs = 0;    ///< offset in bits
	//t_pck_idx_getPackFromStack( L, 1, &pci );
	t_pck_idx_getPackFromFieldOnStack( L, 1, &pci, 0 );
	luaL_argcheck( L, NULL != pci, 1, "Expected `"T_PCK_IDX_TYPE"`." );
	ofs = t_pck_idx_getOffset( L, pci );
	// TODO: Fix getting the offsets
	lua_pushinteger( L, ofs/NB );    // offset in Bytes
	lua_pushinteger( L, ofs );       // offset in Bits
	return 2;
}


/**--------------------------------------------------------------------------
 * Set the default endian style of the T.Pack Constructor for fmt.
 * \param   L    Lua state.
 * \lparam  ud   T.Pack.* instance.
 * \lreturn int  size in bytes.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_SetDefaultEndian( lua_State *L )
{
	char endian = *(luaL_checkstring( L, 1 ));
	if ('n' == endian) // native?
		endian = (IS_LITTLE_ENDIAN) ? 'l' : 'b';
	luaL_argcheck( L, endian == 'l' || endian == 'b', 1,
		"endianness must be 'l'/'b'/'n'" );
	_default_endian = (endian == 'l');
	return 0;
}


/**--------------------------------------------------------------------------
 * Get the specific subType of a Packer/Field.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack/T.Pack.Field userdata instance.
 * \lreturn string Name of pack type.
 * \lreturn string Name of pack sub-type.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck_Type( lua_State *L )
{
	//struct t_pck *p = t_pck_idx_getPackFromStack( L, 1, NULL );
	struct t_pck *p = t_pck_idx_getPackFromFieldOnStack( L, 1, NULL, 0 );
	lua_pushfstring( L, "%s", t_pck_t_lst[ p->t ] );
	t_pck_format( L, p->t, p->s, p->m );
	return 2;
}


/* ###########################################################################
//  __  __      _                         _   _               _
// |  \/  | ___| |_ __ _   _ __ ___   ___| |_| |__   ___   __| |___
// | |\/| |/ _ \ __/ _` | | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
// | |  | |  __/ || (_| | | | | | | |  __/ |_| | | | (_) | (_| \__ \
// |_|  |_|\___|\__\__,_| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
//#########################################################################  */

/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Pack/Field instance.
 * \param   L      Lua state.
 * \lparam  ud     T.Pack userdata instance.
 * \lreturn string formatted string representing packer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_pck__tostring( lua_State *L )
{
	struct t_pck_idx *pci = NULL;
	//struct t_pck     *pck = t_pck_idx_getPackFromStack( L, -1, &pci );
	struct t_pck     *pck = t_pck_idx_getPackFromFieldOnStack( L, -1, &pci, 0 );

	lua_pushfstring( L, (NULL == pci) ? T_PCK_TYPE"(" : T_PCK_IDX_TYPE"(" );
	t_pck_format( L, pck->t, pck->s, pck->m );
	if (NULL == pci) // using ternary operator ( ) ? _ : _  -> compiler error
		lua_pushfstring( L,  "): %p", pck);
	else
		lua_pushfstring( L,  "): %p", pci);
	lua_concat( L, 3 );

	return 1;
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  L     Lua state.
 * \lparam ud    T.Pack.Struct userdata instance.
 * \return int   # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__gc( lua_State *L )
{
	struct t_pck_idx *pci = NULL;
	//struct t_pck     *pck = t_pck_idx_getPackFromStack( L, -1, &pci );
	struct t_pck     *pck = t_pck_idx_getPackFromFieldOnStack( L, -1, &pci, 0 );
	if (NULL != pci)
		luaL_unref( L, LUA_REGISTRYINDEX, pci->pR );
	if (NULL == pci && pck->t > T_PCK_RAW)
		luaL_unref( L, LUA_REGISTRYINDEX, pck->m );
	return 0;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of a Struct/Field instance.
 * \param  L     Lua state.
 * \lparam  ud   T.Pack.Struct/Field instance.
 * \lreturn int  # of elements in T.Pack.Struct/Field instance.
 * \return  int  # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck__len( lua_State *L )
{
	//struct t_pck *pck = t_pck_idx_getPackFromStack( L, -1, NULL );
	struct t_pck *pck = t_pck_idx_getPackFromFieldOnStack( L, -1, NULL, 0 );

	luaL_argcheck( L, pck->t > T_PCK_RAW, 1, "Attempt to get length of atomic "T_PCK_TYPE" type" );

	lua_pushinteger( L, pck->s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call helper to read from a T.Pack.Array/Sequence/Struct instance.
 * Leaves one element on the stack.
 * \param   *L      Lua state pointer.
 * \param   *pck    struct* t_pck instance.
 * \param   *b      char*   buffer to read from.
 * \param   ofs     size_t  running bit offset.
 * \lreturn value   value   read from the buffer.
 * \return  offset  after   read.
 * -------------------------------------------------------------------------*/
static size_t
t_pck__callread( lua_State *L, struct t_pck *pck, const char *b, size_t ofs )
{
	struct t_pck     *p;         ///< packer currently processing
	struct t_pck_idx *pci;       ///< packer field currently processing
	size_t            n;         ///< iterator for complex types

	if (pck->t < T_PCK_ARR)       // handle atomic packer, return single value
	{
		t_pck_read( L, b + ofs/NB, pck, ofs%NB );
		return ofs + pck->s;
	}

	// for all others we need the p->m and a result table
	lua_createtable( L, pck->s, 0 );             //S:… res
	lua_rawgeti( L, LUA_REGISTRYINDEX, pck->m ); //S:… res tbl/pck
	if (pck->t == T_PCK_ARR)       // handle Array; return table
	{
		p  = t_pck_check_ud( L, -1, 1 );
		for (n=0; n<pck->s; n++)
		{
			ofs = t_pck__callread( L, p, b, ofs );       // S:… res typ val
			lua_rawseti( L, -3, n+1 );
		}
		lua_pop( L, 1 );
		return ofs;
	}
	if (pck->t == T_PCK_SEQ)       // handle Sequence, return table
	{
		for (n=0; n<pck->s; n++)
		{
			lua_rawgeti( L, -1, n+1 );         //S:… res tbl pck
			//p   = t_pck_idx_getPackFromStack( L, -1, &pci );
			p   = t_pck_idx_getPackFromFieldOnStack( L, -1, &pci, 0 );
			ofs = t_pck__callread( L, p, b, ofs );//S:… res tbl pck val
			lua_rawseti( L, -4, n+1 );         //S:… res tbl pck
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );
		return ofs;
	}
	if (pck->t == T_PCK_STR)       // handle Struct, return oht
	{
		for (n=0; n<pck->s; n++)
		{
			lua_rawgeti( L, -1, n+1 );         //S:… res tbl key
			lua_pushvalue( L, -1 );            //S:… res tbl key key
			lua_rawget( L, -3 );               //S:… res tbl key idx
			//p   = t_pck_idx_getPackFromStack( L, -1, &pci );
			p   = t_pck_idx_getPackFromFieldOnStack( L, -1, &pci, 0 );
			ofs = t_pck__callread( L, p, b, ofs );//S:… res tbl key pck val
			lua_remove( L, -2 );               //S:… res tbl key val
			lua_pushvalue( L, -2 );            //S:… res tbl key val key
			lua_rawseti( L, -5, lua_rawlen( L, -5 )+1 );
			lua_rawset( L, -4 );
		}
		lua_pop( L, 1 );                      //S:… res

		lua_newtable( L );                    //S:… res tbl
		luaL_getmetatable( L, "T.ProxyTableIndex" ); //S:… res tbl prx
		lua_rotate( L, -3, -1 );              //S:… tbl prx res
		lua_rawset( L, -3 );;                 //S:… tbl
		luaL_getmetatable( L, "T.OrderedHashTable" );
		lua_setmetatable( L, -2 );

		return ofs;
	}
	lua_pushnil( L );
	return ofs;
}


/**--------------------------------------------------------------------------
 * __call (#) for a an T.Pack.Field/Struct instance.
 *          This is used to either read from or write to a string or T.Buffer.
 *          one argument means read, two arguments mean write.
 * \param   L         Lua state.
 * \lparam  ud        T.Pack.Field instance.
 * \lparam  ud,string T.Buffer, t.Buffer.Segment or Lua string.
 * \lparam  T.Buffer or Lua string.
 * \lreturn value     read from Buffer/String according to T.Pack.Field.
 * \return  int    # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_pck_idx__call( lua_State *L )
{
	struct t_pck_idx  *pci = NULL;
	//struct t_pck      *pck = t_pck_idx_getPackFromStack( L, 1, &pci );
	struct t_pck      *pck = t_pck_idx_getPackFromFieldOnStack( L, 1, &pci, 0 );
	size_t             ofs = (NULL == pci) ? 0 : t_pck_idx_getOffset( L, pci );
	//size_t             ofs = (NULL == pf) ? 0 : pf->o;
	char              *b;
	size_t             l;                   /// length of string  overall
	int                canwrite;            /// false if string is passed

	luaL_argcheck( L,  2<=lua_gettop( L ) && lua_gettop( L )<=3, 2,
		"Calling an "T_PCK_IDX_TYPE" takes 2 or 3 arguments!" );

	b = t_buf_checklstring( L, 2, &l, &canwrite );
	//printf( " %ld  %lu %zu %lu %zu \n", (NULL==pf)?-1:pf->o, ofs, o/NB, l*NB, t_pck_getSize( L, pck ) );

	luaL_argcheck( L, (l*NB)+NB >= ofs + t_pck_getSize( L, pck ), 2,
		"String/Buffer must be longer than "T_PCK_TYPE" offset plus length." );

	if (2 == lua_gettop( L ))      // read from input
	{
		t_pck__callread( L, pck, (const char *) b, ofs );
		return 1;
	}
	else                           // write to input
	{
		luaL_argcheck( L, (canwrite), 2, "Can't write value to string type" );
		if (pck->t < T_PCK_ARR)      // handle atomic packer, return single value
			return t_pck_write( L, b + ofs/NB, pck, ofs%NB );
		else                        // create a table ...
			return luaL_error( L, "writing of complex types is not implemented" );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pck_fm [] = {
	  { "__call"         , lt_pck__Call}
	, { NULL             , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_pck_cf [] = {
	  { "size"           , lt_pck_GetSize }
	, { "defaultEndian"  , lt_pck_SetDefaultEndian }
	, { "type"           , lt_pck_Type }
	, { "offset"         , lt_pck_idx_GetOffset }
	, { NULL             , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_pck_m [] = {
	  { "__call"         , lt_pck_idx__call }
	, { "__index"        , lt_pck__index }
	, { "__newindex"     , lt_pck__newindex }
	, { "__pairs"        , lt_pck__pairs }
	, { "__gc"           , lt_pck__gc }
	, { "__len"          , lt_pck__len }
	, { "__tostring"     , lt_pck__tostring }
	, { NULL             , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the T.Pack library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_pck( lua_State *L )
{
	// T.Pack.Field instance metatable
	luaL_newmetatable( L, T_PCK_IDX_TYPE );  // stack: functions meta
	luaL_setfuncs( L, t_pck_m, 0 );

	// T.Pack instance metatable
	luaL_newmetatable( L, T_PCK_TYPE );      // stack: functions meta
	luaL_setfuncs( L, t_pck_m, 0 );
	lua_pop( L, 2 );                         // remove both metatables

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( L, t_pck_cf );
	lua_pushinteger( L, sizeof( int ) );
	lua_setfield( L, -2, "intsize" );
	lua_pushinteger( L, NB );
	lua_setfield( L, -2, "charbits" );
	lua_pushinteger( L, MXINT );
	lua_setfield( L, -2, "numsize" );
	luaL_newlib( L, t_pck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
