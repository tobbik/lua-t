#include "research.h"
#include <stdio.h>
#include <limits.h>

#define MAX_SIZET ((size_t)(~(size_t)0))
#define MAXSIZE   \
   (sizeof(size_t) < sizeof(int) ? MAX_SIZET : (size_t)(INT_MAX))

int main()
{

	printf( "size_t:      %lu\n", (unsigned long) sizeof( size_t ) );
	printf( "int:         %lu\n", (unsigned long) sizeof( int ) );
	printf( "MAXSIZE:     %lu\n", (unsigned long) MAXSIZE );
	printf( "MAX_SIZET:   %lu\n", (unsigned long) MAX_SIZET );
	return 0;
}
