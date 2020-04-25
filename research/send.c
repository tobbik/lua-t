#include "research.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

int main( int argc,char *argv[] )
{
	int sock,conn=10;
	//struct sockaddr_in cli;
	ssize_t res;
	(void) argc; // silence unused arguments warning
	(void) argv; // silence unused arguments warning

	signal( SIGPIPE, SIG_IGN );
	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1)
		printf("Could not create socket");
	printf("Socket[%d] created\n", sock);

	//Prepare the sockaddr_in structure
	//cli.sin_family      = AF_INET;
	//cli.sin_addr.s_addr = INADDR_ANY;
	//cli.sin_port        = htons( 8002 );

	//conn= connect( sock, (struct sockaddr *)&cli , sizeof(cli));

	printf( "Socket connected: %d\n", conn );
	//res = sendto( sock, "ABCDEF", 6, 0, NULL, sizeof( NULL ) );
	res = send( sock, "ABCDEF", 6, 0 );
	printf( "RESULT: %zd", res );
	return 0;
}
