/// The userdata struct for xt.Buffer
struct xt_buf {
	size_t         len;   ///<  length of the entire buffer in bytes
	unsigned char  b[1];  ///<  pointer to the variable size buffer -> must be last in struct
};


enum xt_pack_type {
	XT_PACK_BIT,          ///< X  Bit  wide field
	XT_PACK_INT,          ///< X  Byte wide field as Integer
	XT_PACK_FLT,          ///< X  Byte wide field as Float
	XT_PACK_STR,          ///< X  Byte wide field of char bytes
};


/// The userdata struct for xt.Buffer
struct xt_pack {
	enum  xt_pack_type type;    ///< type of value in the packer
	unsigned char     *b;       ///< pointer to the associate buffer
	size_t             sz;      ///< how many bytes are covered in this packer
	//size_t             ofs;     ///< offset in bytes where *b starts compared to buf->b
	size_t             blen;    ///< how many bits are covered in this packer
	size_t             bofs;    ///< offset in bits in the first byte
	int                islittle;///< is this little endian?
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
// xt_pack.c
int              luaopen_xt_pack  ( lua_State *luaVM );
int              lxt_pack_Int     ( lua_State *luaVM );
int              lxt_pack_Bit     ( lua_State *luaVM );
int              lxt_pack_String  ( lua_State *luaVM );
struct xt_pack  *xt_pack_check_ud ( lua_State *luaVM, int pos );
struct xt_pack  *xt_pack_create_ud( lua_State *luaVM, enum xt_pack_type );
int xt_pack_read( lua_State *luaVM, struct xt_pack *p, const unsigned char *buffer);
int xt_pack_write( lua_State *luaVM, struct xt_pack *p, unsigned char *buffer );

// Constructors
// xt_comb.c
int              luaopen_xt_comb  ( lua_State *luaVM );
int lxt_comb_Struct( lua_State *luaVM );
