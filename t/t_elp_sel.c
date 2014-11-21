/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_elp_sel.c
 * \brief     select() specific implementation for t.Loop.
 * Handles    implmentation specific functions such as registreing events and
 *            executing the loop
 *            Being based on the select() system call this version shall work
 *            on a wide variety of platforms. It is supposed to be the backup
 *            if better versions don't work. However, all known limitation to
 *            select() apply.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t.h"
#include "t_elp.h"

#include <string.h>           // memcpy
#ifndef _WIN32
#include <sys/time.h>         // gettimeofday
#endif


/**--------------------------------------------------------------------------
 * Select() specific initialization of t_elp.
 * \param   struct t_elp * pointer to new userdata on Lua Stack
 * \return  void
 * --------------------------------------------------------------------------*/
void
t_elp_create_ud_impl( struct t_elp *elp )
{
	FD_ZERO( &elp->rfds );
	FD_ZERO( &elp->wfds );
	FD_ZERO( &elp->rfds_w );
	FD_ZERO( &elp->wfds_w );
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the t.Loop.
 * \param   struct t_elp*.
 * \param   int       fd.
 * \param   int(bool) for reading on handle.
 * --------------------------------------------------------------------------*/
void
t_elp_addhandle_impl( struct t_elp *elp, int fd, int read )
{
	if (read)
	{
		elp->fd_set[ fd ]->t = t_elp_READ;
		FD_SET( fd, &elp->rfds );
	}
	else
	{
		elp->fd_set[ fd ]->t = t_elp_WRIT;
		FD_SET( fd, &elp->wfds );
	}
	elp->mxfd = (fd > elp->mxfd) ? fd : elp->mxfd;
}


/**--------------------------------------------------------------------------
 * Add an Timer event handler to the t.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata t.Loop.                                     // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
//void t_elp_addtimer_impl( struct t_elp *elp, struct timeval *tv )
//{
//}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the t.Loop
 * \param   luaVM         The lua state.
 * \param   struct t_elp   The loop struct.
 * \return  number returns from select.
 * --------------------------------------------------------------------------*/
int
t_elp_poll_impl( lua_State *luaVM, struct t_elp *elp )
{
	int              i,r;
	struct timeval  *tv;
	struct timeval   rt;      ///< timer to calculate runtime over this poll

	gettimeofday( &rt, 0 );
	tv  = (NULL != elp->tm_head) ? elp->tm_head->tv : NULL;

	memcpy( &elp->rfds_w, &elp->rfds, sizeof( fd_set ) );
	memcpy( &elp->wfds_w, &elp->wfds, sizeof( fd_set ) );

	r = select( elp->mxfd+1, &elp->rfds_w, &elp->wfds_w, NULL, tv );
	//printf("RESULT: %d\n",r);
	if (r<0)
		return r;

	if (0==r) // deal with timer
	{
		// get, unpack and execute func/parm table
		t_elp_executetimer( luaVM, elp, &rt );
	}
	else      // deal with sockets/file handles
	{
		for( i=0; r>0 && i <= elp->mxfd; i++ )
		{
			if (NULL==elp->fd_set[ i ])
				continue;
			if (FD_ISSET( i, &elp->rfds_w ) || FD_ISSET( i, &elp->wfds_w ))
			{
				r--;
				t_elp_executehandle( luaVM, elp, i );
			}
		}
	}
	return r;
}

