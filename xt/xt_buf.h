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
	XT_PCK_BIT,          ///< X  Bit  wide field
	XT_PCK_FLT,          ///< X  Byte wide field as Float
	XT_PCK_STR,          ///< X  Byte wide field of char bytes
	XT_PCK_ARRAY,        ///< X  Array    Type Packer combinator
	XT_PCK_SEQ,          ///< X  Sequence Type Packer combinator
	XT_PCK_STRUCT,       ///< X  Struct   Type Packer combinator
};


/// The userdata struct for xt.Pack/xt.Pack.Struct
struct xt_pck {
	size_t          sz;   ///< how many bytes are covered in this Structure (incl. su structs)
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
int              lxt_buf_New     ( lua_State *luaVM );
struct xt_buf   *xt_buf_check_ud ( lua_State *luaVM, int pos );
struct xt_buf   *xt_buf_create_ud( lua_State *luaVM, int size );

// accessor helpers for the buffer
uint64_t  xt_buf_readbytes (               size_t sz, int islittle, const unsigned char * buf );
void      xt_buf_writebytes( uint64_t val, size_t sz, int islittle,       unsigned char * buf );
uint64_t  xt_buf_readbits  (               size_t sz, size_t ofs,   const unsigned char * buf );
void      xt_buf_writebits ( uint64_t val, size_t sz, size_t ofs,         unsigned char * buf );

// xt_pck.c
// Constructors
int             lxt_pck_Int     ( lua_State *luaVM );
int             lxt_pck_Bit     ( lua_State *luaVM );
int             lxt_pck_String  ( lua_State *luaVM );
struct xt_pck  *xt_pck_check_ud ( lua_State *luaVM, int pos );
struct xt_pck  *xt_pck_create_ud( lua_State *luaVM, enum xt_pck_t );

// accessor helpers for the Packers
int xt_pck_read ( lua_State *luaVM, struct xt_pck *p, const unsigned char *buffer);
int xt_pck_write( lua_State *luaVM, struct xt_pck *p, unsigned char *buffer );

// xt_pckc.c
int              luaopen_xt_pckc  ( lua_State *luaVM );
int              luaopen_xt_pckr  ( lua_State *luaVM );
struct xt_pck   *xt_pckc_check_ud ( lua_State *luaVM, int pos, int check );
struct xt_pckr  *xt_pckr_check_ud ( lua_State *luaVM, int pos, int check );
struct xt_pckr  *xt_pckr_create_ud( lua_State *luaVM, struct xt_pck *p, size_t o );
int              lxt_pckc_Struct  ( lua_State *luaVM );
int              lxt_pckc_Array   ( lua_State *luaVM );
int              lxt_pckc_Sequence( lua_State *luaVM );


