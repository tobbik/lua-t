
/// The userdata struct for xt.Buffer
struct xt_buf {
	size_t         len;   ///<  length of the entire buffer in bytes
	unsigned char  b[1];  ///<  pointer to the variable size buffer -> must be last in struct
};


enum xt_pack_type {
	XT_PACK_BIT,          ///< X  Bit  wide field
	XT_PACK_INT,          ///< X  Byte wide field as Integer
	XT_PACK_FLT,          ///< X  Byte wide field as Float
	XT_PACK_STR,          ///< string buffer field
	XT_PACK_STRUCT,       ///< struct type combinator
	XT_PACK_SEQ,          ///< sequence type combinator
	XT_PACK_ARRAY,        ///< array type combinator
};

struct xt_pack {
	enum  xt_pack_type type;    ///< type of value in the packer
	unsigned char     *b;       ///< pointer to the associate buffer
	size_t             sz;      ///< how many bytes are covered in this struct
	size_t             ofs;     ///< offset in bytes where *b starts compared to buf->b
	size_t             blen;    ///< how many bytes are covered in this struct
	size_t             bofs;    ///< offset in bytes where *b starts compared to buf->b
	int                islittle;///< is this little endian?
};


enum xt_buf_fld_type {
	BUF_FLD_BIT,    /* X  Bit  wide field*/
	BUF_FLD_BYTE,   /* X  Byte wide field*/
	BUF_FLD_STR     /* string buffer field */
};


struct xt_buf_fld {
	enum  xt_buf_fld_type type;  ///< type of field
	/* size and position information */
	size_t             sz;       ///< size in bits
	size_t             ofs;      ///< how many bits from beginning of buffer
	/* accessor function pointers */
	int              (*write) (lua_State *luaVM);
	int              (*read)  (lua_State *luaVM);
	/** pointer to position in buffer according to type */
	union {
		uint8_t    *v8;
		uint16_t   *v16;
		uint32_t   *v32;
		uint64_t   *v64;
		char       *vS;
	} v;
	/** mask to access the bits in the field */
	union {
		uint8_t     m8;      /* (*v8  & m8)  >> shft == actual_value */
		uint16_t    m16;     /* (*v16 & m16) >> shft == actual_value */
		uint32_t    m32;     /* (*v32 & m32) >> shft == actual_value */
		uint64_t    m64;     /* (*v64 & m64) >> shft == actual_value */
	} m;
	/* right shift to next byte border for bitwise access */
	uint8_t            shft;
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


// xt_buf_fld.c
int                 luaopen_xt_buf_fld   ( lua_State *luaVM );
int                 xt_buf_fld_new_bits  ( lua_State *luaVM );
int                 xt_buf_fld_new_byte  ( lua_State *luaVM );
int                 xt_buf_fld_new_string( lua_State *luaVM );
struct xt_buf_fld  *xt_buf_fld_check_ud  ( lua_State *luaVM, int pos );
struct xt_buf_fld  *xt_buf_fld_create_ud ( lua_State *luaVM );

