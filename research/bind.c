#include "research.h"
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

int main( int argc,char *argv[] )
{
	int socket_desc;
	struct sockaddr_in server;
	(void) argc; // silence unused arguments warning
	(void) argv; // silence unused arguments warning

	//Create socket
	socket_desc = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (socket_desc == -1)
		printf("Could not create socket");

	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port        = htons( 8888 );

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3);
	return 0;
}
