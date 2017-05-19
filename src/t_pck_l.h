/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck.h
 * \brief     data types for packers
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_pck.h"

// Helper Macros
// http://stackoverflow.com/questions/2100331/c-macro-definition-to-determine-big-endian-or-little-endian-machine
// http://esr.ibiblio.org/?p=5095
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)
#define IS_LITTLE_ENDIAN (1 == *(unsigned char *)&(const int){1})
//#define IS_LITTLE_ENDIAN (*(uint16_t*)"\0\1">>8)
//#define IS_BIG_ENDIAN (*(uint16_t*)"\1\0">>8)

// macro taken from Lua 5.3 source code
// number of bits in a character
#define NB                 CHAR_BIT

// mask for one character (NB 1's)
#define MC                 ((1 << NB) - 1)

// size of a lua_Integer
#define MXINT              ((int)sizeof(lua_Integer))

// Maximum bits that can be read or written
#define MXBIT              MXINT * NB


// T.Pack is designed to work like Lua 5.3 pack/unpack support.  By the same
// time it shall have more convienience and be more explicit.


// TODO: make the check of enums based on Bit Masks to have subtype grouping
/* Next gen definitions
enum t_pck_t {
	// atomic packer types
	  T_PCK_ISL      ///< Packer      Integer (signed,     little endian)
	, T_PCK_ISB      ///< Packer      Integer (signed,     big endian   )
	, T_PCK_ISN      ///< Packer      Integer (signed,     native endian)
	, T_PCK_IUL      ///< Packer      Integer (unssigned,  little endian)
	, T_PCK_IUB      ///< Packer      Integer (unssigned,  big endian   )
	, T_PCK_IUN      ///< Packer      Integer (unssigned,  native endian)

	, T_PCK_FLL      ///< Packer      Float   (unssigned,  little endian)
	, T_PCK_FLB      ///< Packer      Float   (unssigned,  big endian   )
	, T_PCK_FLN      ///< Packer      Float   (unssigned,  native endian)

	, T_PCK_BOL      ///< Packer      Boolean (Single Bit as Lua boolean)
	, T_PCK_BTS      ///< Packer      Bits    (signed int,   x bit wide )
	, T_PCK_BTU      ///< Packer      Bits    (unsigned int, x bit wide )

	, T_PCK_RAW      ///< Packer      Raw     (string/utf8/binary)
	// complex packer types
	, T_PCK_ARR      ///< Combinator  Array   (n packers of given type)
	, T_PCK_SEQ      ///< Combinator  Sequence(sequence of packers)
	, T_PCK_STR      ///< Combinator  Struct  (key:packer struct, ordered)
};

static const char *const t_pck_t_lst[] = {
	// atomic packer types
	  "Int%dsl"      ///< Packer      Integer (signed,     little endian)
	, "Int%dsb"      ///< Packer      Integer (signed,     big endian   )
	, "Int%dsn"      ///< Packer      Integer (signed,     native endian)
	, "Int%dul"      ///< Packer      Integer (unssigned,  little endian)
	, "Int%dub"      ///< Packer      Integer (unssigned,  big endian   )
	, "Int%dun"      ///< Packer      Integer (unssigned,  native endian)

	, "Float%dl"     ///< Packer      Float   (unssigned,  little endian)
	, "Float%db"     ///< Packer      Float   (unssigned,  big endian   )
	, "Float%dn"     ///< Packer      Float   (unssigned,  native endian)

	, "Bool"         ///< Packer      Boolean (Single Bit translated to Lua)
	, "Bit%ds"       ///< Packer      Bit     (signed int,   x bit wide  )
	, "Bit%du"       ///< Packer      Bit     (unsigned int, x bit wide  )

	, "Raw%d"        ///< Packer      Raw     (string/utf8/binary)
	// complex packer types
	, "Array"        ///< Combinator  Array   (n packers of given type)
	, "Sequence"     ///< Combinator  Sequence(sequence of packers)
	, "Struct"       ///< Combinator  Struct  (key:packer struct, ordered)
};

*/

// TODO: make the check of enums based on Bit Masks to have subtype grouping
enum t_pck_t {
	// atomic packer types
	  T_PCK_INT      ///< Packer         Integer
	, T_PCK_UNT      ///< Packer         Unsigned Integer
	, T_PCK_FLT      ///< Packer         Float
	, T_PCK_DBL      ///< Packer         Double
	, T_PCK_BOL      ///< Packer         Boolean (1 Bit)
	, T_PCK_BTS      ///< Packer         Bit (integer x Bit) - signed
	, T_PCK_BTU      ///< Packer         Bit (integer x Bit) - unsigned
	, T_PCK_RAW      ///< Packer         Raw - string/utf8/binary
	// complex packer types
	, T_PCK_ARR      ///< Combinator     Array
	, T_PCK_SEQ      ///< Combinator     Sequence
	, T_PCK_STR      ///< Combinator     Struct
};


static const char *const t_pck_t_lst[] = {
	// atomic packer types
	  "Int"          ///< Packer         Integer
	, "UInt"         ///< Packer         Unsigned Integer
	, "Float"        ///< Packer         Float
	, "Double"       ///< Packer         Double
	, "Bool"         ///< Packer         Boolean (1 Bit)
	, "SBit"         ///< Packer         Signed   Integer as BitArray (integer x Bit)
	, "UBit"         ///< Packer         Unsigned Integer as BitArray (integer x Bit)
	, "Raw"          ///< Packer         Raw - string/utf8/binary
	// complex packer types
	, "Array"        ///< Combinator     Array
	, "Sequence"     ///< Combinator     Sequence
	, "Struct"       ///< Combinator     Struct
};


/// The userdata struct for T.Pack/T.Pack.Struct/T.Pack.Array/T.Pack.Sequence
/// This is a very terse structure to make it use a very small amount of memory
/// since it is used often.
struct t_pck {
	enum  t_pck_t  t;                 ///< type of packer
	/// size of packer -> various meanings
	///  -- int/uint float raw = number of bits, NOT bytes
	///  -- bit bits nibble    = number of bits
	///  -- Seq Struct Arr     = number of elements in Combinator
	size_t         s;
	/// modifier -> various meanings
	///  -- int/uint float     = Endian ( 0=Big, 1=Little )
	///  -- bit                = Offset from beginning of byte ( bit numbering: MSB 0 )
	///  -- raw                = ??? (unused)
	///  -- Arr                = LUA_REGISTRYINDEX for packer
	///  -- Seq                = LUA_REGISTRYINDEX for table
	///        table[ i    ] = Pack.Field (has offset information)
	///  -- Struct             = LUA_REGISTRYINDEX for table
	///        table[ i    ] = name       -- schema of OrderedHashTable
	///        table[ name ] = Pack.Field
	int            m;
};

/// The userdata struct for T.Pack.Field
struct t_pck_fld {
	int      pR;   ///< LUA_REGISTRYINDEX to the T.Pack or T.Pack.Field type
	size_t   o;    ///< offset from the beginning of the wrapping Combinator in bits
};

/// Union for serializing floats (taken from Lua 5.3)
union Ftypes {
	float       f;
	double      d;
	lua_Number  n;
	char buff[ 5 * sizeof( lua_Number ) ];  // enough for any float type
};



// t_pck.c
// Constructors
struct t_pck *t_pck_check_ud  ( lua_State *L, int pos, int check );
struct t_pck *t_pck_create_ud ( lua_State *L, enum t_pck_t t, size_t s, int m );

// accessor helpers for the Packers
size_t        t_pck_read      ( lua_State *L, const char *b, struct t_pck *p, size_t o );
int           t_pck_write     ( lua_State *L,       char *b, struct t_pck *p, size_t o );

// helpers for the Packers
struct t_pck *t_pck_getPacker ( lua_State *L, int pos, size_t *bo );
size_t        t_pck_getSize   ( lua_State *L,  struct t_pck *p );

struct t_pck *t_pck_fld_getPackFromStack( lua_State * L, int pos, struct t_pck_fld **pcf );


struct t_pck *t_pck_str_create( lua_State *L );
struct t_pck *t_pck_seq_create( lua_State *L, int sp, int ep, size_t *bo );
struct t_pck *t_pck_arr_create( lua_State *L );


int          lt_pck_fld__index( lua_State *L );
int          lt_pck_fld__newindex( lua_State *L );
int          lt_pck_fld__pairs( lua_State *L );
size_t       t_pck_fld__callread( lua_State *L, struct t_pck *pc, const char *b, size_t o );

struct t_pck *t_pck_fmt_read( lua_State *L, const char **f, int *e, size_t *bo );

struct t_pck_fld *t_pck_fld_create_ud( lua_State *L, int ofs );

