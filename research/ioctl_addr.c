/*
  Example code to obtain IP and MAC for all available interfaces on Linux.
  by Adam Pierce <adam@doctort.org>

http://www.doctort.org/adam/

*/

#include "research.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>


int main( int argc,char *argv[] )
{
	char          buf[1024];
	struct ifconf ifc;
	struct ifreq *ifr;
	int           sck;
	int           nInterfaces;
	int           i;
	(void) argc; // silence unused arguments warning
	(void) argv; // silence unused arguments warning

/* Get a socket handle. */
	sck = socket(AF_INET, SOCK_DGRAM, 0);
	if(sck < 0)
	{
		perror("socket");
		return 1;
	}

/* Query available interfaces. */
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
	{
		perror("ioctl(SIOCGIFCONF)");
		return 1;
	}

/* Iterate through the list of interfaces. */
	ifr         = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)
	{
		struct ifreq *item = &ifr[i];

	/* Show the device name and IP address */
		printf("%s: IP %s",
		       item->ifr_name,
		       inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));

	/* Get the MAC address */
		if(ioctl(sck, SIOCGIFHWADDR, item) < 0)
		{
			perror("ioctl(SIOCGIFHWADDR)");
			return 1;
		}

	/* Get the broadcast address (added by Eric) */
		if(ioctl(sck, SIOCGIFBRDADDR, item) >= 0)
			printf(", BROADCAST %s", inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr));
		printf("\n");
	}

        return 0;
}
