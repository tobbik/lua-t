/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_pcc.h
 * \brief     data types for packers and associate structures
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


// T.Pack is designed to work like Lua 5.3 pack/unpack support.  By the same
// time it shall have more convienience and be more explicit.


enum t_pcc_t {
	TPCC_INT,      ///< Packer         Integer
	TPCC_FLT,      ///< Packer         Float
	TPCC_BIT,      ///< Packer         Bit
	TPCC_RAW,      ///< Packer         Raw  -  string/utf8/binary
	TPCC_ARR,      ///< Combinator     Array
	TPCC_SEQ,      ///< Combinator     Sequence
	TPCC_STR,      ///< Combinator     Struct
};


/// The userdata struct for T.Pack/T.Pack.Struct
struct t_pcc {
	enum  t_pcc_t  t;   ///< type of packer
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
	struct t_pcc   *p;   ///< reference to packer type
	size_t          o;   ///< offset from the beginning of the wrapping Struct
	size_t          s;   ///< size of this Reader
};


// t_pck.c
// Constructors
struct t_pcc  *t_pcc_check_ud ( lua_State *luaVM, int pos );
struct t_pcc  *t_pcc_create_ud( lua_State *luaVM, enum t_pcc_t );

// accessor helpers for the Packers
int t_pcc_read ( lua_State *luaVM, struct t_pcc *p, const unsigned char *buffer);
int t_pcc_write( lua_State *luaVM, struct t_pcc *p, unsigned char *buffer );

// tostring helper
void t_pcc_format( lua_State *luaVM, struct t_pcc *p );


// t_pckc.c
/*
int             luaopen_t_pckc  ( lua_State *luaVM );
int             luaopen_t_pckr  ( lua_State *luaVM );
struct t_pck   *t_pckc_check_ud ( lua_State *luaVM, int pos, int check );
struct t_pckr  *t_pckr_check_ud ( lua_State *luaVM, int pos, int check );
struct t_pckr  *t_pckr_create_ud( lua_State *luaVM, struct t_pck *p, size_t o );
int             lt_pckc_Struct  ( lua_State *luaVM );
int             lt_pckc_Array   ( lua_State *luaVM );
int             lt_pckc_Sequence( lua_State *luaVM );

*/
