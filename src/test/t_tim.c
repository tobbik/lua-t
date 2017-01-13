/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      test/t_tim.c
 * \brief     Unit test for the lua-t timer source code
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_unittest.h"

static struct timeval tA;
static struct timeval tB;
static struct timeval tC;
static int            secA  = 1234;
static int            usecA = 123456;
static int            secB  = 5678;
static int            usecB = 654321;

static int
setup( )
{
	tA.tv_sec  = secA;
	tA.tv_usec = usecA;
	tB.tv_sec  = secB;
	tB.tv_usec = usecB;
	return 0;
}

static int
teardown( )
{
	tA.tv_sec  = 0;
	tA.tv_usec = 0;
	tB.tv_sec  = 0;
	tB.tv_usec = 0;
	return 0;
}


static int
test_t_tim_failure( )
{
	_assert( tC.tv_sec  == secA + secB + 1 );
	_assert( tC.tv_sec  == secA + secB );
	_assert( tC.tv_usec == usecA + usecB + 1 );
	return 0;
}


static int
test_t_tim_add( )
{
	t_tim_add( &tA, &tB, &tC );
	_assert( tC.tv_sec  == secA + secB );
	_assert( tC.tv_usec == usecA + usecB );
	return 0;
}


static int
test_t_tim_add_ms_overflow( )
{
	tA.tv_usec += tB.tv_usec;

	// test proper microsecond overflow
	t_tim_add( &tA, &tB, &tC );
	_assert( tC.tv_sec  == secA + secB + 1 );
	_assert( tC.tv_usec == (usecA + usecB+ usecB) - 1000000 );
	return 0;
}

// Add all testable functions to the array
static const struct test_case all_tests [] = {
	{ "Adding two t_tim values"                    , test_t_tim_add },
	{ "Adding two t_tim values with usec overflow" , test_t_tim_add_ms_overflow },
	{ "Purposfully failing test"                   , test_t_tim_failure },
	{ NULL, NULL }
};

int
main()
{
	return test_execute( all_tests, setup, teardown );
}
