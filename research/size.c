#include "research.h"
#include <stdio.h>
#include <limits.h>

#define MAX_SIZET ((size_t)(~(size_t)0))
#define MAXSIZE   \
   (sizeof(size_t) < sizeof(int) ? MAX_SIZET : (size_t)(INT_MAX))

int main()
{

	printf( "size_t:      %lu\n", sizeof( size_t ) );
	printf( "int:         %lu\n", sizeof( int ) );
	printf( "MAXSIZE:     %lu\n", MAXSIZE );
	printf( "MAX_SIZET:   %lu\n", MAX_SIZET );
	return 0;
}
