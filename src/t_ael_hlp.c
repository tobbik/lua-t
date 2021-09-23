/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael_hlp.c
 * \brief     Helper functions for the ael loop.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
//#include <arpa/inet.h>
//#include <unistd.h>
#include <fcntl.h>
//#include <string.h>
//#include <netdb.h>
#include <errno.h>
//#include <stdarg.h>
//#include <stdio.h>

/* Enable the FD_CLOEXEC on the given fd to avoid fd leaks. 
 * This function should be invoked for fd's on specific places 
 * where fork + execve system calls are called. */
int t_ael_hlp_cloexec( int fd )
{
	int r;
	int flags;

	do {
		r = fcntl( fd, F_GETFD );
	} while (r == -1 && errno == EINTR);

	if (r == -1 || (r & FD_CLOEXEC))
		return r;

	flags = r | FD_CLOEXEC;

	do {
		r = fcntl( fd, F_SETFD, flags );
	} while (r == -1 && errno == EINTR);

	return r;
}


