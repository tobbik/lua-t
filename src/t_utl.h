/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_utl.h
 * \brief     Headers Utility functions for the T.* universe.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

int     t_utl_prsCharNumber(               char c, int isHex );
int     t_utl_prsNumber    (               const char **fmt, int dflt, int isHex );
int     t_utl_prsMaxNumber ( lua_State *L, const char **fmt, int dflt, int max, int isHex );


void    t_utl_printIntBin  ( lua_Unsigned i );
void    t_utl_printCharBin ( volatile char *b, size_t sz );
void    t_utl_printIntHex  ( lua_Unsigned i );
void    t_utl_printCharHex ( volatile char *b, size_t sz );
