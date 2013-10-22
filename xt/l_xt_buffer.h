
struct buffer_stream {
	size_t         length;
	unsigned char  buffer[1];  // variable size buffer -> last in struct
};


enum buf_seg_type {
	NUM,            /* X  Bit wide field*/
	STR             /* Incrementing numerical value */
};


struct buffer_segment {
	enum  buf_seg_type     type;          /* type of field  */
	// struct buffer_stream  *buffer;
	/* pointer to position in buffer according to type */
	uint8_t               *val8;
	uint16_t              *val16;
	uint32_t              *val32;
	uint64_t              *val64;         /* val64 is also used for arbitrary length bit fields */
	char                  *valS;
	// accessor function pointers
	int                  (*write) (lua_State *luaVM);
	int                  (*read)  (lua_State *luaVM);
	/* access helpers */
	uint8_t                out_shft;
	uint64_t               out_mask;      // (*valX & out_mask) >> out_shft == actual_value
	/* size information */
	size_t                 size_bit;      /* size in bits   */
	size_t                 off_bit;    /* how many bits into the byte does it start  */
	size_t                 off_byte;   /* how many bytes into the buffer does it start  */
};


// Constructors
// l_buffer_stream.c
int                     luaopen_buffer_stream   (lua_State *luaVM);
struct buffer_stream   *check_ud_buffer_stream   (lua_State *luaVM, int pos);
struct buffer_stream   *create_ud_buffer_stream  (lua_State *luaVM, int size);


// l_buffer_segment.c
int                     luaopen_buffer_stream_segment   (lua_State *luaVM);
struct buffer_segment  *check_ud_buffer_segment   (lua_State *luaVM, int pos);
struct buffer_segment  *create_ud_buffer_segment  (lua_State *luaVM);

