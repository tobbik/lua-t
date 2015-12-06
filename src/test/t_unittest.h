#define FAIL() printf("\nfailure in %s() line %d\n", __func__, __LINE__-source_line_offset)
#define _assert(test) do { if (!(test)) { FAIL(); return 1; } } while(0)
#define _verify(test) do { int r=test(); tests_run++; if(r) return r; } while(0)

int source_line_offset;
int tests_run;
void countlines( char * filename );

struct test_function {
  const char        *name;
  int  (*func) ();
};

int test_execute( const struct test_function *t );
