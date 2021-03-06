#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <linux/if_link.h>
#include <string.h>
#include <linux/if_packet.h>

void
print_sockaddr(struct sockaddr *addr,const char *name, struct ifaddrs *ifa )
{
    char addrbuf[128] ;
    struct rtnl_link_stats *stats;
    unsigned int flow_id=0, scope_id=0;
    struct sockaddr_ll   *hwaddr;

    if(addr->sa_family == AF_UNSPEC)
        return;
    switch(addr->sa_family) {
        case AF_INET:
            inet_ntop(addr->sa_family,&((struct sockaddr_in*)addr)->sin_addr,addrbuf,sizeof(addrbuf));
            break;
        case AF_INET6:
            inet_ntop(addr->sa_family,&((struct sockaddr_in6*)addr)->sin6_addr,addrbuf,sizeof(addrbuf));
            scope_id = ((struct sockaddr_in6*)addr)->sin6_scope_id;
            flow_id  = ((struct sockaddr_in6*)addr)->sin6_flowinfo;
            break;
        case AF_PACKET:
            hwaddr = (struct sockaddr_ll*)addr;
            sprintf(addrbuf, " %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                hwaddr->sll_addr[0],
                hwaddr->sll_addr[1],
                hwaddr->sll_addr[2],
                hwaddr->sll_addr[3],
                hwaddr->sll_addr[4],
                hwaddr->sll_addr[5],
                hwaddr->sll_addr[6],
                hwaddr->sll_addr[7]);

            //memset( addrbuf, 0, 128);
            if (NULL == ifa->ifa_data) break;
            else stats = ifa->ifa_data;
            printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
                   "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
                    stats->tx_packets, stats->rx_packets,
                    stats->tx_bytes, stats->rx_bytes );
            break;
        default:
            sprintf (addrbuf, "Unknown (%d)", (int)addr->sa_family );
            break;

    }
    if (AF_INET6 == addr->sa_family)
        printf("%-16s[%d] %s (%d ; %d)\n", name, addr->sa_family, addrbuf, flow_id, scope_id);
    else
        printf("%-16s[%d] %s\n", name, addr->sa_family, addrbuf);
}

void
print_ifaddr(struct ifaddrs *addr)
{
    //printf("-----------------------NEXT ITERATION------(%s)[%d] \n", addr->ifa_name, addr->ifa_addr->sa_family);
    printf("-----------------------NEXT ITERATION------(%s)\n", addr->ifa_name);
    if(addr->ifa_addr != NULL)
        print_sockaddr(addr->ifa_addr, "Address", addr);
    /*
    if(addr->ifa_netmask != NULL)
        print_sockaddr(addr->ifa_netmask,"Netmask", addr);
    if(addr->ifa_broadaddr != NULL)
        print_sockaddr(addr->ifa_broadaddr,"Broadcast addr.", addr);
    if(addr->ifa_dstaddr != NULL)
        print_sockaddr(addr->ifa_dstaddr,"Peer addr.", addr);
        */
    puts("");
}

int main( int argc,char *argv[] )
{
    struct ifaddrs *addrs,*ifa;
    (void) argc; // silence unused arguments warning
    (void) argv; // silence unused arguments warning

    if(getifaddrs(&addrs) != 0) {
        perror("getifaddrs");
        return 1;
    }
    for(ifa = addrs; ifa ; ifa = ifa->ifa_next) {
        print_ifaddr(ifa);
    }

    freeifaddrs(addrs);

    return 0;
}
