#include <stdio.h>
#include <stdlib.h>
#include "t_unittest.h"

void
countlines( char * filename )
{
	source_line_offset = 0;
	char ch;
	FILE *fp = fopen( filename,"r" );
	while( !feof( fp ) )
	{
		ch = fgetc( fp );
		if (ch == '\n')
		{
			source_line_offset++;
		}
	}
	fclose( fp );
}

static int do_verify( int (*func) () )
{
	_verify( func );
	return 0;
}

int
test_execute( const struct test_function *t )
{
	int result = 0;
	for (; t->name != NULL; t++)
	{
		printf( "Test...%s\n", t->name );
		result += do_verify( t->func );
	}
	return result;
}

