#include <stdio.h>
#include <stdlib.h>
#include "t_unittest.h"

static int
countlines( char * file_name )
{
	char  ch;
	FILE *fp    = fopen( file_name, "r" );
	int   lines = 0; 

	while (! feof( fp ))
	{
		ch = fgetc( fp );
		if ('\n' == ch)
			lines++;
		if (ch != EOF)
			putc( ch, stdout );
	}
	fclose( fp );
	return lines;
}

static void
add_main_function( int lines )
{
	fprintf( stdout, 
"static int t_utst_run_case( int (*func) () ) {\n\
	_verify( func );\n\
	return 0;\n\
}\n\
\n\
static int t_utst_run_envelope( int (*func) () ) {\n\
	_envelope( func );\n\
	return 0;\n\
}\n\
\n\
static int t_utst_case_run_suite( const struct t_utst_case *t ) {\n\
	int result = 0;\n\
	for (; t->name != NULL; t++) {\n\
		printf( \"Test...%%s\\n\", t->name );\n\
		t_utst_run_envelope( t_utst_case_setup );\n\
		result += t_utst_run_case( t->func );\n\
		t_utst_run_envelope( t_utst_case_teardown );\n\
	}\n\
	return result;\n\
}\n\
\n\
int main () {\n\
	t_utst_source_line_offset = %d;\n\
	return t_utst_case_run_suite( t_utst_all_tests );\n\
}\n\
", lines );
}

int
main( int argc, char *argv[] )
{
	int i        = 0;
	int lines    = 0;
	int x;

	// the world's shittiest implementation of cat
	for (i=1; i<argc; i++)
	{
		//printf( "READING:%s\n", argv[i] );
		x      = countlines( argv[i] );
		lines += (i != argc-1) ? x : 0;
	}

	add_main_function( lines );
}

