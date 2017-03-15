/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_utl.c
 * \brief     Utility functions for the T.* universe.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t.h"
#include "t_utl.h"

/** -------------------------------------------------------------------------
 * Converts character into integer value.
 * \param     c     char.
 * \param     isHex int/bool.
 * \return    value int.
 *  -------------------------------------------------------------------------*/
int
t_utl_prsCharNumber( char c, int isHex )
{
	if ( c >= '0' && c <= '9' )
		return c - '0';
	if ( isHex && c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
	if ( isHex && c >= 'A' && c <= 'F' )
		return c - 'A' + 10;
	return -1;
}


/** -------------------------------------------------------------------------
 * reads from string until input is not numeric any more.
 * \param   char** number string.
 * \param   int    default value.
 * \param   isHex  int/bool.
 * \return  int    parse integer value.
 *  -------------------------------------------------------------------------*/
int
t_utl_prsNumber( const char **fmt, int dflt, int isHex )
{
	int    a = 0;
	int base = (isHex) ? 16 : 10;
	int    v = t_utl_prsCharNumber( **fmt, isHex );

	if (v < 0)    // no number
		return dflt;
	else
	{
		while (v>0 && a<(INT_MAX/base - base))
		{
			a = a*base + v;
			v = t_utl_prsCharNumber( *(++(*fmt)), isHex );
		}
	}
	return a;
}


/** -------------------------------------------------------------------------
 * Read a positive only integer from a string of numbers.
 * raises an error if it is larger than the maximum desired size.
 * \param  char* string of numbers.
 * \param  int   default value if no number is in string.
 * \param  int   max value allowed for int.
 * \param  isHex bool; Read as HexaDecimally formatted: else as decimal.
 * \return val   parsed integer value.
 *  -------------------------------------------------------------------------*/
int
t_utl_prsMaxNumber( lua_State *L, const char **fmt, int dflt, int max, int isHex )
{
	int val = t_utl_prsNumber( fmt, dflt, isHex );
	if (val > max || val <= 0)
		luaL_error( L, "size (%d) out of limits [1,%d]", val, max );
	return val;
}




/** -------------------------------------------------------------------------
 * Prints a binary representation of a number as in memory.
 * \param  char array to print.
 *-------------------------------------------------------------------------*/
void
t_utl_printCharBin( volatile char *b, size_t sz )
{
	size_t n, x;

	for (n=0; n<sz; n++)
	{
		for (x=CHAR_BIT; x>0; x--)
			printf( "%d", (b[n] >> (x-1) & 0x01) ? 1 : 0 );
		printf( " " );
	}
	printf( "\n" );
}

/** -------------------------------------------------------------------------
 * Prints a binary representation of a number.
 * \param  int integer to print.
 *-------------------------------------------------------------------------*/
void
t_utl_printIntBin( lua_Unsigned i )
{
	size_t n,x;

	for (n=sizeof( lua_Unsigned )*CHAR_BIT; n>0; n-=CHAR_BIT)
	{
		for (x=CHAR_BIT; x>0; x--)
			printf( "%d", ((i >> (n-CHAR_BIT+x-1)) & 0x01) ? 1 : 0 );
		printf( " " );
	}
	printf( "\n" );
}

/** -------------------------------------------------------------------------
 * Prints a hexadecimal representation of a number as in memory.
 * \param  char array to print.
 *-------------------------------------------------------------------------*/
void
t_utl_printCharHex( volatile char *b, size_t sz )
{
	size_t n;

	for (n=0; n<sz; n++)
		printf( "%02X ", b[n] & 0X00000000000000FF);
	printf( "\n" );
}

/** -------------------------------------------------------------------------
 * Prints a haxadecimal representation of a number.
 * \param  int integer to print.
 *-------------------------------------------------------------------------*/
void
t_utl_printIntHex( lua_Unsigned i )
{
	size_t n;
	for (n=sizeof(lua_Unsigned); n>0; n--)
		printf( "%02llX ", (i >> (n-1)*CHAR_BIT) & 0X00000000000000FF );
	printf("\n");
}
