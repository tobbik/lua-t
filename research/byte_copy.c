/*
 * Example code read bits from stream into a fixed size integer.
 * This is more akin the way the Lua conversion is about to happen.
 * by Tobias Kieslich
 */
#include "research.h"
#include <stdio.h>
#include <limits.h>

#define SZ   ((int) sizeof( long ))

// Prints a binary representation of a number as in memory.
void printCharByte( char *v, size_t sz )
{
	size_t n;

	printf( "MEM: " );
	for (n=0; n<sz; n++)
		printf( "%02X ", v[n] & 0X00000000000000FF);
	printf( "\n" );
}


static long signInt( unsigned long val, size_t sz )
{
	unsigned long msk = (unsigned long) 1  << (sz*NB - 1);
	size_t n;
	printf( "INT: " );
	for (n=MXINT; n>0; n--)
		printf( "%02lX ", (msk >> (n-1)*NB) & 0X00000000000000FF );
	printf( "\n" );
	return (long) ((val ^ msk) - msk);
}


// Prints a binary representation of a number.
void printIntByte( unsigned long *val, size_t sz, int is_signed )
{
	size_t n;
	printf( "INT: " );
	for (n=MXINT; n>0; n--)
		//printf( "%02lX ", (i << (MXINT - sz*NB)) >> (NB*sz) );
		printf( "%02lX ", (*val >> (n-1)*NB) & 0X00000000000000FF );
	printf("\n");
	if (is_signed)
	{
		long sVal = signInt( *val, sz );
		printf("(%ld)\n\n", sVal);
	}
	else
		printf("(%zu)\n\n", *val);

}


// Copy byte by byte from one string to another. Honours endianess.
// dst is ALWAYS sizeof( long ) aka. MXINT
static void doCopyBytes( char *dst, char *src, size_t sz, int is_little )
{
#ifdef IS_LITTLE_ENDIAN
	if (is_little)
		while (sz-- != 0)
			*(dst++) = *(src+sz);
	else
	{
		src = src+sz-1;
		while (sz-- != 0)
			*(dst++) = *(src-sz);
	}
#else
	int i = 0;
	if (is_little)
		while (sz-- != 0)
			dst[i++] = src[sz];
	else
		while (sz-- != 0)
			dst[MXINT-1-i++] = src[sz];
#endif
}

static void copyBytes( unsigned long *val, char *src, size_t sz, int is_little )
{
	char *out  = (char *) val;
	*val       =  0;
	doCopyBytes( out, src, sz, is_little );
	printCharByte( src, MXINT );
	printCharByte( out, MXINT );
}

int main(void)
{
	char         sA[] = { 0x61, 0x42, 0x63, 0x44, 0x65, 0xC3, 0xBC, 0x48, 0x69,
	                      0x4A, 0x6B, 0x4C, 0x6D, 0x4E, 0x6F, 0x50, 0xC3, 0xB6, 0x00 };
	char         *s   = &sA[0];
	unsigned long val = 0;

	// unsigned int  I3  = 0;
	// int           i2  = 0;
	// char          b   = 0;
	// unsigned char B   = 0;
	// unsigned long I5  = 0;
	// int           I4  = 0;
	// short         h   = 0;

	printf("%s\n", s);
	printCharByte( s, 18 );
	printf("\n");

	copyBytes( &val, s+0,  3, 0 );
	printIntByte( &val, 3, 0 );

	copyBytes( &val, s+3,  2, 1 );
	printIntByte( &val, 2, 1 );

	copyBytes( &val, s+5,  1, 1 );
	printIntByte( &val, 1, 1 );

	copyBytes( &val, s+6,  1, 0 );
	printIntByte( &val, 1, 0 );

	copyBytes( &val, s+7,  5, 0 );
	printIntByte( &val, 5, 0 );

	copyBytes( &val, s+12, 4, 0 );
	printIntByte( &val, 4, 0 );

	copyBytes( &val, s+16, 2, 1 );
	printIntByte( &val, 2, 1 );
	return 0;
}


