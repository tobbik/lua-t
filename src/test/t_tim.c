/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      test/t_tim.c
 * \brief     Unit test for the lua-t timer source code
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_unittest.h"

static int
test_t_tim_add( )
{
	struct timeval tA;
	struct timeval tB;
	struct timeval tC;

	tA.tv_sec  = 1234;
	tA.tv_usec = 223456;
	tB.tv_sec  = 5678;
	tB.tv_usec = 789012;

	// test proper microsecond overflow
	t_tim_add( &tA, &tB, &tC );
	_assert( tC.tv_sec  == 1234 + 5678 + 1 );
	_assert( tC.tv_usec == (223456 + 789012) % 1000000 );
	return 0;
}

// Add all testable functions to the array
static const struct test_function all_tests [] = {
	{ "Adding two t_tim values", test_t_tim_add },
	{ NULL, NULL }
};

int
main()
{
	return test_execute( all_tests );
}
