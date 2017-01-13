#include <stdio.h>
#include <stdlib.h>
#include "t_unittest.h"

void
countlines( char * file_name )
{
	source_line_offset = 0;
	char ch;
	FILE *fp = fopen( file_name, "r" );

	while (! feof( fp ))
	{
		ch = fgetc( fp );
		if ('\n' == ch)
			source_line_offset++;
	}
	fclose( fp );
}

static int run_test_case( int (*func) () )
{
	_verify( func );
	return 0;
}

static int run_envelope( int (*func) () )
{
	_envelope( func );
	return 0;
}


int
test_execute( const struct test_case *t, int (*setup) (), int (*teardown) () )
{
	int result = 0;
	for (; t->name != NULL; t++)
	{
		printf( "Test...%s\n", t->name );
		run_envelope( setup );
		result += run_test_case( t->func );
		run_envelope( teardown );
	}
	return result;
}

