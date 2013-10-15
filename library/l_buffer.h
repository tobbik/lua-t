
struct buffer_stream {
	size_t         length;
	unsigned char  buffer[1];  // variable size buffer -> last in struct
};

// Constructors
// l_buffer_stream.c
int                     luaopen_buffer_stream   (lua_State *luaVM);
struct buffer_stream   *check_ud_buffer_stream   (lua_State *luaVM, int pos);
struct buffer_stream   *create_ud_buffer_stream  (lua_State *luaVM, int size);

