#include <stdio.h>
#include <string.h>
 
int main(int argc, char** argv)
{
    char   b[] = "CONNECT Alles was ich habe";
	 char  *n,*x,*r = b;
	 size_t idx;

	 printf( "%s\n", r);

	 n   = strchr( r, ' ');
	 idx = r-n;

	 printf( "%s  %zu\n", n, n-r );

    return 0;
}
