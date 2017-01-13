#include <stdio.h>
#include <stdlib.h>
#include "t_unittest.h"

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
test_execute( const struct test_case *t, int (*setup) (), int (*teardown) (), int offset )
{
	int result = 0;
	source_line_offset = offset;

	for (; t->name != NULL; t++)
	{
		printf( "Test...%s\n", t->name );
		run_envelope( setup );
		result += run_test_case( t->func );
		run_envelope( teardown );
	}
	return result;
}


