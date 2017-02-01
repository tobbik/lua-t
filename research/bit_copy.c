/*
 * Example code read bits from stream into an integer.
 * by Tobias Kieslich
 */
#include "research.h"
#include <stdio.h>
#include <limits.h>

// Prints a binary representation of a number as in memory.
void printCharBin( char *v, size_t sz )
{
	size_t n,x;

	printf( "MEM: " );
	for (n=0; n<sz; n++)
	{
		for (x=NB; x>0; x--)
			printf( "%d", (*v >> (x-1) & 0x01) ? 1 : 0 );
		printf( " " );
		v++;
	}
	printf( "\n" );
}

// Prints a binary representation of a number.
void printIntBin( long i )
{
	size_t n,x;

	printf( "INT: " );
	for (n=sizeof( long )*NB; n>0; n-=NB)
	{
		for (x=NB; x>0; x--)
			printf( "%d", ((i >> (n-NB+x-1)) & 0x01) ? 1 : 0 );
		printf( " " );
	}
	printf( "\n" );
}


// Copy byte by byte from one string to another. Honours endianess.
// dst is ALWAYS sizeof( long ) aka. MXINT
static void copyBytes( char *dst, char *src, size_t sz, bool is_little )
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


static void  getValue( char *src, size_t sz, size_t ofs, bool is_signed )
{
	unsigned long val    = 0;
	unsigned long msk    = 0;
	char         *out    = (char *) &val;
	size_t        a_byte = ((sz + ofs) / NB) + 1;    // how many bytes to copy for ALL bits
	size_t        l_shft = MXBIT - a_byte*NB + ofs;
	//size_t        r_shft = MXBIT - sz;
	copyBytes( out, src, a_byte, TRUE );
	printCharBin( out, 8 );
	printIntBin( val);
	val = (val << l_shft);
	printIntBin( val);
	val = val >> (MXBIT - sz);
	//val = (val << l_shft) >> (MXBIT - sz);
	if (is_signed)
	{
		msk = (unsigned long) 1  << (sz - 1);
		printIntBin( msk );
		printIntBin( (long) ((val ^ msk) - msk) );
		printf( "%ld", (long) ((val ^ msk) - msk) );
	}
	else
	{
		printIntBin( val );
		printf( "%zu", val );
	}

	printf( "\n\n" );
}


int main(void)
{
	//char          sA[] = { 0x95, 0x65, 0xDC, 0x14, 0xD9, 0x00 };
	char          sA[] = { 0x9A, 0xB2, 0xEE, 0x0A, 0x7C, 0x65, 0x25, 0x00 };
	char         *s    = &sA[0];

	printCharBin( s, 5 );
	printf( "\n" );

	getValue( s+0, 4, 0, TRUE );
	getValue( s+0, 7, 4, FALSE );
	getValue( s+1, 3, 3, TRUE );
	getValue( s+1, 3, 6, TRUE );
	getValue( s+2, 3, 1, TRUE );
	getValue( s+2, 3, 4, TRUE );
	getValue( s+2, 3, 7, TRUE );
	getValue( s+3, 3, 2, TRUE );
	getValue( s+3, 3, 5, TRUE );
	getValue( s+4, 3, 0, TRUE );
	getValue( s+4,15, 3, TRUE );
	getValue( s+6, 1, 2, FALSE );
	getValue( s+6, 1, 3, FALSE );
	getValue( s+6, 1, 4, FALSE );
	getValue( s+6, 1, 5, FALSE );
	getValue( s+6, 1, 6, TRUE );
	getValue( s+6, 1, 7, TRUE );

	return 0;
}


