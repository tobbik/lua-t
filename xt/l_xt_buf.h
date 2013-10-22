
struct xt_buf {
	size_t         len;
	unsigned char  b[1];  // variable size buffer -> last in struct
};


enum xt_buf_seg_type {
	BUF_SEG_BIT,    /* X  Bit  wide field*/
	BUF_SEG_BYTE,   /* X  Byte wide field*/
	BUF_SEG_STR     /* string buffer field */
};


struct xt_buf_seg {
	enum  xt_buf_seg_type type;    /* type of field  */
	/* pointer to position in buffer according to type */
	uint8_t           *v8;
	uint16_t          *v16;
	uint32_t          *v32;
	uint64_t          *v64; 
	char              *vS;
	// accessor function pointers
	int              (*write) (lua_State *luaVM);
	int              (*read)  (lua_State *luaVM);
	/* helpers for bitwise access */
	uint8_t            shft;
	uint64_t           mask;     /* (*valX & mask) >> shft == actual_value */
	/* size information */
	size_t             sz_bit;   /* size in bits   */
	size_t             ofs_bit;  /* how many bits  into the byte does it start    */
	size_t             ofs_byte; /* how many bytes into the buffer does it start  */
};


// Constructors
// l_xt_buf.c
int              luaopen_buf    (lua_State *luaVM);
struct xt_buf   *check_ud_buf   (lua_State *luaVM, int pos);
struct xt_buf   *create_ud_buf  (lua_State *luaVM, int size);


// l_xt_buf_seg.c
int                 luaopen_buf_seg    (lua_State *luaVM);
int                 c_new_buf_seg      (lua_State *luaVM);
struct xt_buf_seg  *check_ud_buf_seg   (lua_State *luaVM, int pos);
struct xt_buf_seg  *create_ud_buf_seg  (lua_State *luaVM);

