/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/** -------------------------------------------------------------------------
 * \file      t_enc.h
 * \brief     Data Types and global functions for various En?decoders
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 *-------------------------------------------------------------------------*/

#define T_ENC_RC4_IDNT  "rc4"
#define T_ENC_CRC_IDNT  "crc"
#define T_ENC_B64_IDNT  "b64"

#define T_ENC_RC4_NAME  "RC4"
#define T_ENC_CRC_NAME  "Crc"
#define T_ENC_B64_NAME  "Base64"

#define T_ENC_RC4_TYPE  T_ENC_TYPE"."T_ENC_RC4_NAME
#define T_ENC_CRC_TYPE  T_ENC_TYPE"."T_ENC_CRC_NAME
#define T_ENC_B64_TYPE  T_ENC_TYPE"."T_ENC_B64_NAME

/// struct to keep track of a RC4 encoding
struct t_enc_rc4 {
	unsigned char  prng[256]; ///< Pseudo RNG, aka. the state
	uint8_t        i1;        ///< current index 1
	uint8_t        i2;        ///< current index 2
};


///  struct to keep track of a CRC encoding
struct t_enc_crc {
	/// Encoded polynom table
	union {
		uint8_t       t8 [256];
		uint16_t      t16[256];
		uint32_t      t32[256];
	};
	/// runing CRC value
	union {
		uint8_t       crc8;
		uint16_t      crc16;
		uint32_t      crc32;
	};
	/// initial CRC value
	union {
		uint8_t       init8;
		uint16_t      init16;
		uint32_t      init32;
	};
	int              bE;    ///< boolean BigEndian
	int             (*calc)( struct t_enc_crc *crc, const char *data, size_t len );
};


// Constructors
// t_enc_rc4.c
struct t_enc_rc4  *t_enc_rc4_check_ud ( lua_State *L, int pos );
struct t_enc_rc4  *t_enc_rc4_create_ud( lua_State *L );
int                luaopen_t_enc_rc4  ( lua_State *L );

// t_enc_crc.c
struct t_enc_crc  *t_enc_crc_check_ud ( lua_State *L, int pos );
struct t_enc_crc  *t_enc_crc_create_ud( lua_State *L );
int                luaopen_t_enc_crc  ( lua_State *L );

// t_enc_b64.c
int                luaopen_t_enc_b64  ( lua_State *L );

