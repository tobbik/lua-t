
struct xt_buf {
	size_t         len;   // length in bytes
	unsigned char  b[1];  // variable size buffer -> last in struct
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
int              luaopen_xt_buf    (lua_State *luaVM);
int              xt_buf_New        (lua_State *luaVM);
struct xt_buf   *xt_buf_check_ud   (lua_State *luaVM, int pos);
struct xt_buf   *xt_buf_create_ud  (lua_State *luaVM, int size);


// xt_buf_fld.c
int                 luaopen_xt_buf_fld    (lua_State *luaVM);
int                 xt_buf_fld_new_bits   (lua_State *luaVM);
int                 xt_buf_fld_new_byte   (lua_State *luaVM);
int                 xt_buf_fld_new_string (lua_State *luaVM);
struct xt_buf_fld  *xt_buf_fld_check_ud      (lua_State *luaVM, int pos);
struct xt_buf_fld  *xt_buf_fld_create_ud     (lua_State *luaVM);

