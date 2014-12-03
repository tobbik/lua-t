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

// TODO: make the cheack of enums based on Bit Masks to have subtype grouping
enum t_pck_t {
	// atomic packer types
	T_PCK_INT,      ///< Packer         Integer
	T_PCK_UNT,      ///< Packer         Unsigned Integer
	T_PCK_FLT,      ///< Packer         Float
	T_PCK_BOL,      ///< Packer         Boolean (1 Bit)
	T_PCK_BIT,      ///< Packer         Bit (integer x Bit)
	T_PCK_RAW,      ///< Packer         Raw  -  string/utf8/binary
	// complex packer types
	T_PCK_ARR,      ///< Combinator     Array
	T_PCK_SEQ,      ///< Combinator     Sequence
	T_PCK_STR,      ///< Combinator     Struct
};

static const char *const t_pck_t_lst[] = {
	// atomic packer types
	"Int",          ///< Packer         Integer
	"UInt",         ///< Packer         Unsigned Integer
	"Float",        ///< Packer         Float
	"Boolean",      ///< Packer         Boolean (1 Bit)
	"Bit",          ///< Packer         Bit (integer x Bit)
	"Raw",          ///< Packer         Raw  -  string/utf8/binary
	// complex packer types
	"Array",        ///< Combinator     Array
	"Sequence",     ///< Combinator     Sequence
	"Struct",       ///< Combinator     Struct
};


/// The userdata struct for T.Pack/T.Pack.Struct
struct t_pck {
	enum  t_pck_t  t;   ///< type of packer
	/// size of packer -> various meanings
	///  -- for int/uint, float, raw =  the number of bytes
	///  -- for bit, bits and nibble =  the number of bits
	///  -- for Seq,Struct,Arr       =  the number of elements in this Combinator
	size_t         s;
	/// modifier -> various meanings
	///  -- for int/uint             = Endian (0=Big, 1=Little)
	///  -- for bit                  = Offset from beginning of byte (bit numbering: MSB 1)
	///  -- for raw                  = ???
	///  -- for Arr                  = lua registry Reference for packer
	///  -- for Seq                  = lua registry Reference for the table
	///                                         idx[i]    = Pack
	///                                         idx[s+i]  = ofs
	///  -- for Struct               = lua registry Reference for the table
	///                                         idx[i]    = Pack
	///                                         idx[s+i]  = ofs
	///                                         idx[2s+i] = name
	///                                         idx[name] = i
	int            m;
};


/// The userdata struct for T.Pack.Reader
struct t_pcr {
	int      r;   ///< reference to packer type
	size_t   o;   ///< offset from the beginning of the wrapping Struct
	//size_t          s;   ///< size of this Reader
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
struct t_pck *t_pck_check_ud ( lua_State *luaVM, int pos, int check );
struct t_pck *t_pck_create_ud( lua_State *luaVM, enum t_pck_t t, size_t s, int m );

// accessor helpers for the Packers
int t_pck_read ( lua_State *luaVM, struct t_pck *p, const unsigned char *buffer);
int t_pck_write( lua_State *luaVM, struct t_pck *p, unsigned char *buffer );

// helpers for the Packers
struct t_pck *t_pck_getpck( lua_State *luaVM, int pos, size_t *bo );
int           t_pcr__callread ( lua_State *luaVM, struct t_pck *pc, const unsigned char *b );
