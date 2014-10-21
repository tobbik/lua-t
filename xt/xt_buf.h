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
};


/// The userdata struct for xt.Packer
struct xt_pck {
	size_t          sz;   ///< how many bytes are covered in this packer
	size_t          blen; ///< how many bits are covered in this packer
	size_t          bofs; ///< offset in bits in the first byte
	enum  xt_pck_t  t;    ///< type of value in the packer
	unsigned char  *b;    ///< pointer to the associate buffer
};


/// The userdata struct for xt.Packer.Struct
struct xt_pck_s {
	unsigned char     *b;       ///< pointer to the associate buffer
	size_t             sz;      ///< how many bytes are covered in this packer
	size_t             n;       ///< how many packers are in this sequence
	int                buf_ref; ///< Lua registry reference to buffer
	int                idx_ref; ///< Lua registry reference to buffer
	int                p[1];    ///< array of ref to packers in proper order -> must be last in struct
};

/// The userdata struct for xt.Packer.Array
struct xt_pck_a {
	unsigned char     *b;       ///< pointer to the associate buffer
	size_t             sz;      ///< how many bytes are covered in this packer
	size_t             n;       ///< how many packers are in this sequence
	int                buf_ref; ///< Lua registry reference to buffer
	int                typ_ref; ///< Lua registry reference to type (Packer,Struct,Array)
};

// Constructors
// xt_buf.c
int              luaopen_xt_buf  ( lua_State *luaVM );
int              lxt_buf_New     ( lua_State *luaVM );
struct xt_buf   *xt_buf_check_ud ( lua_State *luaVM, int pos );
struct xt_buf   *xt_buf_create_ud( lua_State *luaVM, int size );

// accessor helpers for the buffer
uint64_t  xt_buf_readbytes (               size_t sz, int islittle, const unsigned char * buf );
void      xt_buf_writebytes( uint64_t val, size_t sz, int islittle,       unsigned char * buf );
uint64_t  xt_buf_readbits  (               size_t sz, size_t ofs,   const unsigned char * buf );
void      xt_buf_writebits ( uint64_t val, size_t sz, size_t ofs,         unsigned char * buf );

// Constructors
// xt_pck.c
int             lxt_pck_Int     ( lua_State *luaVM );
int             lxt_pck_Bit     ( lua_State *luaVM );
int             lxt_pck_String  ( lua_State *luaVM );
struct xt_pck  *xt_pck_check_ud ( lua_State *luaVM, int pos );
struct xt_pck  *xt_pck_create_ud( lua_State *luaVM, enum xt_pck_t );

// accessor helpers for the buffer
int xt_pck_read ( lua_State *luaVM, struct xt_pck *p, const unsigned char *buffer);
int xt_pck_write( lua_State *luaVM, struct xt_pck *p, unsigned char *buffer );

// xt_pck_s.c
int              luaopen_xt_pck_s  ( lua_State *luaVM );
struct xt_pck_s *xt_pck_s_check_ud ( lua_State *luaVM, int pos );
int              lxt_pck_s__call   ( lua_State *luaVM );
int              lxt_pck_Struct    ( lua_State *luaVM );

// xt_pck_a.c
int              luaopen_xt_pck_a  ( lua_State *luaVM );
struct xt_pck_a *xt_pck_a_check_ud ( lua_State *luaVM, int pos );
int              lxt_pck_a__call   ( lua_State *luaVM );
int              lxt_pck_Array     ( lua_State *luaVM );

