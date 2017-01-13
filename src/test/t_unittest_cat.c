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
				"int main()\n"
					"{\n"
						"\treturn test_execute( all_tests, setup, teardown, %d );\n"
					"}\n\n",
			lines );
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
