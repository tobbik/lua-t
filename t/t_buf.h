/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_buf.h
 * \brief     data types for buffer and packers
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/// The userdata struct for T.Buffer
struct t_buf {
	size_t         len;   ///<  length of the entire buffer in bytes
	unsigned char  b[1];  ///<  pointer to the variable size buffer -> must be last in struct
};


// T.Pack is designed to work like Lua 5.3 pack/unpack support.  By the same
// time it shall have more convienience and be more explicit.

enum t_pck_t {
	T_PCK_INT,      ///< Packer         Integer
	T_PCK_UNT,      ///< Packer         Unsigned Integer
	T_PCK_FLT,      ///< Packer         Float
	T_PCK_BIT,      ///< Packer         Bit
	T_PCK_RAW,      ///< Packer         Raw  -  string/utf8/binary
	T_PCK_ARR,      ///< Combinator     Array
	T_PCK_SEQ,      ///< Combinator     Sequence
	T_PCK_STR,      ///< Combinator     Struct
};

static const char *const t_pck_t_lst[] = {
	"Int",          ///< Packer         Integer
	"UInt",         ///< Packer         Unsigned Integer
	"Float",        ///< Packer         Float
	"Bit",          ///< Packer         Bit
	"Raw",          ///< Packer         Raw  -  string/utf8/binary
	"Array",        ///< Combinator     Array
	"Sequence",     ///< Combinator     Sequence
	"Struct",       ///< Combinator     Struct
};


/// The userdata struct for T.Pack/T.Pack.Struct
struct t_pck {
	enum  t_pck_t  t;   ///< type of packer
	/// size of packer -> various meanings
	///  -- for int, float, raw      =  the number of bytes
	///  -- for bit, bits and nibble =  the number of bits
	///  -- for Seq,Struct,Arr       =  the number of elements in this Combinator
	size_t         s;
	/// modifier -> various meanings
	///  -- for int                  = Endian (-2=Big, -1=Little, 1=Little, 2=Big)
	///  -- for bit                  = Offset form beginning of byte (bit numbering: MSB 1)
	///  -- for Raw                  = 
	///  -- for Seq,Struct,Arr       = lua registry Reference for the table 
	/// The structure looks like the following:
	///                                         idx[i]    = Pack
	///                                         idx[n+i]  = ofs
	///                                         idx[2n+i] = name
	///                                         idx[name] = i
	int            m;
};


/// The userdata struct for T.Pack.Reader
struct t_pcr {
	struct t_pck   *p;   ///< reference to packer type
	size_t          o;   ///< offset from the beginning of the wrapping Struct
	size_t          s;   ///< size of this Reader
};


/// The userdata struct for T.Pack.Reader
struct t_pckr {
	struct t_pck   *p;   ///< reference to packer type
	size_t          o;   ///< offset from the beginning of the wrapping Struct
};


// t_buf.c
// Constructors
int              luaopen_t_buf  ( lua_State *luaVM );
struct t_buf   *t_buf_check_ud ( lua_State *luaVM, int pos, int check );
struct t_buf   *t_buf_create_ud( lua_State *luaVM, int size );

// helpers to check and verify input on stack
struct t_buf * t_buf_getbuffer( lua_State *luaVM, int pB, int pP, int *pos );



// t_pck.c
// Constructors
struct t_pck  *t_pck_check_ud ( lua_State *luaVM, int pos, int check );
struct t_pck  *t_pck_create_ud( lua_State *luaVM, enum t_pck_t );

// helpers to interpret format strings
struct t_pck *t_pck_lookup    ( lua_State *luaVM, enum t_pck_t t, size_t s, int m);
struct t_pck *t_pck_getoption ( lua_State *luaVM, const char **f, int *e );

// accessor helpers for the Packers
int t_pck_read ( lua_State *luaVM, struct t_pck *p, const unsigned char *buffer);
int t_pck_write( lua_State *luaVM, struct t_pck *p, unsigned char *buffer );

// tostring helper
void t_pck_format( lua_State *luaVM, enum t_pck_t t, size_t s, int m );



// t_pcr.c
/*
int             luaopen_t_pcr  ( lua_State *luaVM );
int             luaopen_t_pckr  ( lua_State *luaVM );
struct t_pck   *t_pckc_check_ud ( lua_State *luaVM, int pos, int check );
struct t_pckr  *t_pckr_check_ud ( lua_State *luaVM, int pos, int check );
struct t_pckr  *t_pckr_create_ud( lua_State *luaVM, struct t_pck *p, size_t o );
int             lt_pckc_Struct  ( lua_State *luaVM );
int             lt_pckc_Array   ( lua_State *luaVM );
int             lt_pckc_Sequence( lua_State *luaVM );
*/

