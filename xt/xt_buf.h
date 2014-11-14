/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_buf.h
 * \brief     data types for buffer and packers
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

/// The userdata struct for xt.Buffer
struct xt_buf {
	size_t         len;   ///<  length of the entire buffer in bytes
	unsigned char  b[1];  ///<  pointer to the variable size buffer -> must be last in struct
};


enum xt_pck_t {
	XT_PCK_INTB,         ///< X  Byte wide field as Integer -> Big    Endian
	XT_PCK_INTL,         ///< X  Byte wide field as Integer -> Little Endian
	XT_PCK_BYTE,         ///< X  1 Byte wide field
	XT_PCK_NBL,          ///< X  Nibble (4 bits) HI/LO stored in p->oB
	XT_PCK_BIT,          ///< X  1 Bit   wide field
	XT_PCK_BITS,         ///< X  x Bits  wide field
	XT_PCK_FLT,          ///< X  Byte wide field as Float
	XT_PCK_STR,          ///< X  Byte wide field of char bytes
	XT_PCK_ARRAY,        ///< X  Array    Type Packer combinator
	XT_PCK_SEQ,          ///< X  Sequence Type Packer combinator
	XT_PCK_STRUCT,       ///< X  Struct   Type Packer combinator
};

static const char *const xt_pck_t_lst[] = {
	"IntB",         ///< X  Byte wide field as Integer -> Big    Endian
	"IntL",         ///< X  Byte wide field as Integer -> Little Endian
	"Byte",         ///< X  1 Byte wide field
	"Nibble",       ///< X  Nibble (4 bits) HI/LO stored in p->oB
	"Bit",          ///< X  1 Bit   wide field
	"Bits",         ///< X  x Bits  wide field
	"Float",        ///< X  Byte wide field as Float
	"String",       ///< X  Byte wide field of char bytes
	"Array",        ///< X  Array    Type Packer combinator
	"Sequence",     ///< X  Sequence Type Packer combinator
	"Struct",       ///< X  Struct   Type Packer combinator
	NULL
};


/// The userdata struct for xt.Pack/xt.Pack.Struct
struct xt_pck {
	size_t          sz;   ///< how many bytes are covered in this Structure (incl. sub elements)
	size_t          lB;   ///< how many bits are covered in this packer
	size_t          oB;   ///< offset in bits in the first byte
	/// Lua Registry Reference to the table which holds the pack reference, name
	/// and offset controlled by index.  The structure looks like the following:
	/// idx[i]    = Pack
	/// idx[n+i]  = ofs
	/// idx[2n+i] = name
	/// idx[name] = i
	int             iR;   ///< Lua registry ref to index table
	size_t          n;    ///< How many elements in the Packer
	enum  xt_pck_t  t;    ///< type of packer
};


/// The userdata struct for xt.Pack.Reader
struct xt_pckr {
	struct xt_pck   *p;   ///< reference to packer type
	size_t           o;   ///< offset from the beginning of the wrapping Struct
};


// xt_buf.c
// Constructors
int              luaopen_xt_buf  ( lua_State *luaVM );
struct xt_buf   *xt_buf_check_ud ( lua_State *luaVM, int pos, int check );
struct xt_buf   *xt_buf_create_ud( lua_State *luaVM, int size );

// helpers to check and verify input on stack
struct xt_buf * xt_buf_getbuffer( lua_State *luaVM, int pB, int pP, int *pos );



// xt_pck.c
// Constructors
struct xt_pck  *xt_pck_check_ud ( lua_State *luaVM, int pos );
struct xt_pck  *xt_pck_create_ud( lua_State *luaVM, enum xt_pck_t );

// accessor helpers for the Packers
int xt_pck_read ( lua_State *luaVM, struct xt_pck *p, const unsigned char *buffer);
int xt_pck_write( lua_State *luaVM, struct xt_pck *p, unsigned char *buffer );

// tostring helper
void xt_pck_format( lua_State *luaVM, struct xt_pck *p );


// xt_pckc.c
int              luaopen_xt_pckc  ( lua_State *luaVM );
int              luaopen_xt_pckr  ( lua_State *luaVM );
struct xt_pck   *xt_pckc_check_ud ( lua_State *luaVM, int pos, int check );
struct xt_pckr  *xt_pckr_check_ud ( lua_State *luaVM, int pos, int check );
struct xt_pckr  *xt_pckr_create_ud( lua_State *luaVM, struct xt_pck *p, size_t o );
int              lxt_pckc_Struct  ( lua_State *luaVM );
int              lxt_pckc_Array   ( lua_State *luaVM );
int              lxt_pckc_Sequence( lua_State *luaVM );


