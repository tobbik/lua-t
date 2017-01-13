#include <stdio.h>
#include <stdlib.h>

#define FAIL(description) \
	printf( "\tfailure in %s() line %d -- \"%s\"\n\n", \
	__func__, __LINE__ - t_utst_source_line_offset, description )

#define _assert(test, description) \
	do { \
		if (! (test)) { \
			FAIL(description); \
			return 1; \
		} \
	} while (0)

#define _envelope(envelope) \
	do { \
		int r=envelope(); \
		if (r) { \
			printf( "\tsetup/teardown failed\n\n"); \
			return r; \
		} \
	} while (0)

#define _verify(test) \
	do { \
		int r=test(); \
		t_utst_all_run_tests++; \
		if (r) \
			return r; \
	} while (0)

int t_utst_all_run_tests;
int t_utst_source_line_offset;

struct t_utst_case {
  const char   *name;
  int         (*func) ();
};
