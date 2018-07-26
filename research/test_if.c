/*
 * Example code read bits from stream into a fixed size integer.
 * This is more akin the way the Lua conversion is about to happen.
 * by Tobias Kieslich
 */
#include "research.h"
#include <stdio.h>
#include <stdlib.h>

int num( int arg )
{
	return arg;
}

int main( int argc,char *argv[] )
{
	int a=0, n=-2;
	if (argc > 1) n = atoi( argv[1] );

	// assign return value of num() to a AND test a for a condition
	if ((a = num(n)) > 0 )
		printf( "MATCH:  a = %d\n", a );
	else
		printf( "a = %d\n", a );
	return 0;
}
