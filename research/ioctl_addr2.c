#include "research.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>


static const char * flags(int sd, const char * name)
{
    static char buf[1024];

    static struct ifreq ifreq;
    strcpy(ifreq.ifr_name, name);

    int r = ioctl(sd, SIOCGIFFLAGS, (char *)&ifreq);
    assert(r == 0);

    int l = 0;
#define FLAG(b) if(ifreq.ifr_flags & b) l += snprintf(buf + l, sizeof(buf) - l, #b " ")
    FLAG(IFF_UP);
    FLAG(IFF_BROADCAST);
    FLAG(IFF_DEBUG);
    FLAG(IFF_LOOPBACK);
    FLAG(IFF_POINTOPOINT);
    FLAG(IFF_RUNNING);
    FLAG(IFF_NOARP);
    FLAG(IFF_PROMISC);
    FLAG(IFF_NOTRAILERS);
    FLAG(IFF_ALLMULTI);
    FLAG(IFF_MASTER);
    FLAG(IFF_SLAVE);
    FLAG(IFF_MULTICAST);
    FLAG(IFF_PORTSEL);
    FLAG(IFF_AUTOMEDIA);
    FLAG(IFF_DYNAMIC);
#undef FLAG

    return buf;
}

int main( int argc,char *argv[] )
{
    static struct ifreq ifreqs[32];
    struct ifconf ifconf;
    (void) argc; // silence unused arguments warning
    (void) argv; // silence unused arguments warning
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.ifc_req = ifreqs;
    ifconf.ifc_len = sizeof(ifreqs);

    int sd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sd >= 0);

    int r = ioctl(sd, SIOCGIFCONF, (char *)&ifconf);
    assert(r == 0);

    for(size_t i = 0; i < ifconf.ifc_len/sizeof(struct ifreq); ++i)
    {
        printf("%s:    %s\t %s\t %s\t%s\n",
				  ifreqs[i].ifr_name
				, inet_ntoa(((struct sockaddr_in *)&ifreqs[i].ifr_addr)->sin_addr)
				, inet_ntoa(((struct sockaddr_in *)&ifreqs[i].ifr_broadaddr)->sin_addr)
				, inet_ntoa(((struct sockaddr_in *)&ifreqs[i].ifr_netmask)->sin_addr)
				, inet_ntoa(((struct sockaddr_in *)&ifreqs[i].ifr_dstaddr)->sin_addr)
			);
        printf(" flags: %s\n", flags(sd, ifreqs[i].ifr_name));
    }

    close(sd);

    return 0;
}
