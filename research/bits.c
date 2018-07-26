/*
 * Explore 2 co.
 * by Tobias Kieslich
 */
#include "research.h"
#include <stdio.h>
#include <limits.h>


// Prints a binary representation of a number.
void printIntBin( long long unsigned int i )
{
	size_t n,x;

	printf( "INT: " );
	for (n=MXBIT; n>0; n-=NB)
	{
		for (x=NB; x>0; x--)
			printf( "%d", ((i >> (n-NB+x-1)) & 0x01) ? 1 : 0 );
		printf( " " );
	}
	printf( "\n" );
}


int main( int argc,char *argv[] )
{
	long long unsigned int   val, msk;
	size_t                   i,s;
	(void) argc; // silence unused arguments warning
	(void) argv; // silence unused arguments warning

	for (i=0; i<MXBIT; i++)
	{
		s   = i + 1;
		val = 1 - 2;  // -1 = 0xFFFFFF....
		val = (val << (MXBIT-s)) >> (MXBIT-s);  // truncate bits
		msk = (long long unsigned int) 1  << (s - 1);
		printf( "%016llX   ", val);
		printIntBin(val);
		printf( "%016llX   ", msk);
		printIntBin(msk);
		printf( "%016llX   ", msk^val);
		printIntBin(msk^val);
		printf( "%016llX   ", (msk^val)-msk);
		printIntBin((msk^val)-msk);
		printf( "\n" );
	}

	return 0;
}


