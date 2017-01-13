#define FAIL() \
	printf( "\tfailure in %s() line %d\n\n", __func__, __LINE__-source_line_offset )

#define _assert(test) \
	do { \
		if (!(test)) { \
			FAIL(); \
			return 1; \
		} \
	} while(0)

#define _envelope(envelope) \
	do { \
		int r=envelope(); \
		if(r) { \
			printf( "\tsetup/teardown failed\n\n"); \
			return r; \
		} \
	} while(0)

#define _verify(test) \
	do { \
		int r=test(); \
		tests_run++; \
		if(r) \
			return r; \
	} while(0)

int  source_line_offset;
int  tests_run;
void countlines( char *file_name );

struct test_case {
  const char   *name;
  int         (*func) ();
};

int test_execute( const struct test_case *t, int (*setup) (), int (*teardown) () );
