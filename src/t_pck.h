/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pck.h
 * \brief     data types for packers
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_buf.h"            // read/write buffers
#include "t_oht.h"            // read arguments into struct

#define T_PCK_FLD_NAME  "Field"

#define T_PCK_FLD_TYPE  T_PCK_TYPE"."T_PCK_FLD_NAME


// macros taken from Lua 5.3 source code
/* number of bits in a character */
#define NB                 CHAR_BIT

/* mask for one character (NB 1's) */
#define MC                 ((1 << NB) - 1)

/* size of a lua_Integer */
#define MXINT              ((int)sizeof(lua_Integer))

// Maximum bits that can be read or written
#define MXBIT              MXINT * NB




// T.Pack is designed to work like Lua 5.3 pack/unpack support.  By the same
// time it shall have more convienience and be more explicit.

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
	, "Boolean"      ///< Packer         Boolean (1 Bit)
	, "BitSigned"    ///< Packer         Bit (integer x Bit)
	, "BitUnsigned"  ///< Packer         Bit (integer x Bit)
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
	///  -- for int/uint float raw = the number of bytes
	///  -- for bit bits nibble    = the number of bits
	///  -- for Seq StructArr      = the number of elements in this Combinator
	size_t         s;
	/// modifier -> various meanings
	///  -- for int/uint           = Endian (0=Big, 1=Little)
	///  -- for bit                = Offset from beginning of byte (bit numbering: MSB 0)
	///  -- for raw                = ??? (unused)
	///  -- for Arr                = lua registry Reference for packer
	///  -- for Seq                = lua registry Reference for the table
	///        idx[ i    ] = Pack
	///  -- for Struct             = lua registry Reference for the table
	///        idx[ i    ] = Pack
	///        idx[ name ] = i
	int            m;
};


/// The userdata struct for T.Pack.Reader
struct t_pcr {
	int      r;   ///< reference to packer type
	size_t   o;   ///< offset from the beginning of the wrapping Struct
};


/// The userdata struct for T.Pack.Field
struct t_pck_fld {
	int      pR;   ///< reference to packer type
	size_t   o;    ///< offset from the beginning of the wrapping Struct
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
int           t_pck_read      ( lua_State *L, struct t_pck *p, const unsigned char *buffer);
int           t_pck_write     ( lua_State *L, struct t_pck *p, unsigned char *buffer );

// helpers for the Packers
struct t_pck *t_pck_getPacker ( lua_State *L, int pos, size_t *bo );
size_t        t_pck_getSize( lua_State *L,  struct t_pck *p, int bits );
int           t_pck_fld__callread ( lua_State *L, struct t_pck *pc, const unsigned char *b );

struct t_pck *t_pck_fld_getPackFromStack( lua_State * L, int pos, struct t_pck_fld **pcf );


struct t_pck *t_pck_str_create( lua_State *L );
struct t_pck *t_pck_seq_create( lua_State *L, int sp, int ep, size_t *bo );
struct t_pck *t_pck_arr_create( lua_State *L );


int          lt_pck_fld__index( lua_State *L );
int          lt_pck_fld__newindex( lua_State *L );
int          lt_pck_fld__pairs( lua_State *L );

struct t_pck *t_pck_fmt_read( lua_State *L, const char **f, int *e, size_t *bo );

struct t_pck_fld *t_pck_fld_create_ud( lua_State *L, int ofs );
