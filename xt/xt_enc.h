
/**
 * \brief  struct to keep track of a Arc4 encoding
 */
struct xt_enc_arc4 {
	unsigned char  prng[256]; ///< Pseudo RNG, aka. the state
	uint8_t        i1;        ///< current index 1
	uint8_t        i2;        ///< current index 2
};



// Constructors
// xt_enc_rc4.c
struct xt_enc_arc4 *xt_enc_arc4_check_ud  (lua_State *luaVM, int pos);
struct xt_enc_arc4 *xt_enc_arc4_create_ud (lua_State *luaVM);
int                 xt_enc_arc4_new       (lua_State *luaVM);
int                 luaopen_xt_enc_arc4   (lua_State *luaVM);
