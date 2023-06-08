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

#include <time.h>
#include <sys/time.h>
#include <stdio.h>


#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


/* Enable the FD_CLOEXEC on the given fd to avoid fd leaks. 
 * This function should be invoked for fd's on specific places 
 * where fork + execve system calls are called. */
int
t_ael_hlp_cloexec( int fd )
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

#define MS_PER_SEC      1000ULL     // MS = milliseconds
#define US_PER_MS       1000ULL     // US = microseconds
#define HNS_PER_US      10ULL       // HNS = hundred-nanoseconds (e.g., 1 hns = 100 ns)
#define NS_PER_US       1000ULL

#define HNS_PER_SEC     (MS_PER_SEC * US_PER_MS * HNS_PER_US)
#define NS_PER_HNS      (100ULL)    // NS = nanoseconds
#define NS_PER_SEC      (MS_PER_SEC * US_PER_MS * NS_PER_US)

int
clock_gettime_monotonic( struct timespec *ts )
{
#if defined(LUAT_USE_WINDOWS)
	static LARGE_INTEGER ticksPerSec;
	LARGE_INTEGER ticks;

	if (!ticksPerSec.QuadPart)
	{
		QueryPerformanceFrequency( &ticksPerSec );
		if (!ticksPerSec.QuadPart) {
			errno = ENOTSUP;
			return -1;
		}
	}

	QueryPerformanceCounter(&ticks);

	ts->tv_sec  = (long) (ticks.QuadPart / ticksPerSec.QuadPart);
	ts->tv_nsec = (long) (((ticks.QuadPart % ticksPerSec.QuadPart) * NS_PER_SEC) / ticksPerSec.QuadPart);
#endif

#ifdef __MACH__
	clock_serv_t    cclock;
	mach_timespec_t mts;
	host_get_clock_service( mach_host_self( ), SYSTEM_CLOCK, &cclock );
	clock_get_time( cclock, &mts );
	mach_port_deallocate( mach_task_self( ), cclock );
	ts->tv_sec   = mts.tv_sec;
	ts->tv_nsec  = mts.tv_nsec;
#else
	clock_gettime( CLOCK_MONOTONIC, ts );
#endif

	return 0;
}

int
clock_gettime_realtime( struct timespec *ts )
{
#if defined(LUAT_USE_WINDOWS)
	FILETIME ft;
	ULARGE_INTEGER hnsTime;

	GetSystemTimePreciseAsFileTime( &ft );

	hnsTime.LowPart  = ft.dwLowDateTime;
	hnsTime.HighPart = ft.dwHighDateTime;

	// To get POSIX Epoch as baseline, subtract the number of hns intervals from Jan 1, 1601 to Jan 1, 1970.
	hnsTime.QuadPart -= (11644473600ULL * HNS_PER_SEC);

	// modulus by hns intervals per second first, then convert to ns, as not to lose resolution
	ts->tv_sec  = (long) (hnsTime.QuadPart / HNS_PER_SEC);
	ts->tv_nsec = (long) ((hnsTime.QuadPart % HNS_PER_SEC) * NS_PER_HNS);
#endif

#ifdef __MACH__
	clock_serv_t    cclock;
	mach_timespec_t mts;
	host_get_clock_service( mach_host_self(), CALENDAR_CLOCK, &cclock );
	clock_get_time( cclock, &mts );
	mach_port_deallocate( mach_task_self( ), cclock );
	ts.tv_sec  = mts.tv_sec;
	ts.tv_nsec = mts.tv_nsec;
#else
	clock_gettime( CLOCK_REALTIME, ts );
#endif

	return 0;
}

