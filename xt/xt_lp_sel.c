/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_lp_sel.c
 * \brief     select() specific implementation for xt.Loop.
 * Handles    implmentation specific functions such as registreing events and
 *            executing the loop
 *            Being based on the select() system call this version shall work
 *            on a wide variety of platforms. It is supposed to be the backup
 *            if better versions don't work. However, all known limitation to
 *            select() apply.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

#include "xt.h"
#include "xt_lp.h"

#ifndef _WIN32
#include <sys/time.h>
#endif


/**--------------------------------------------------------------------------
 * Select() specific initialization of xt_lp.
 * \param   struct xt_lp * pointer to new userdata on Lua Stack
 * \return  void
 * --------------------------------------------------------------------------*/
void xt_lp_create_ud_impl( struct xt_lp *lp )
{
	FD_ZERO( &lp->rfds );
	FD_ZERO( &lp->wfds );
	FD_ZERO( &lp->rfds_w );
	FD_ZERO( &lp->wfds_w );
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the xt.Loop.
 * \param   struct xt_lp*.
 * \param   int       fd.
 * \param   int(bool) for reading on handle.
 * --------------------------------------------------------------------------*/
void xt_lp_addhandle_impl( struct xt_lp *lp, int fd, int read )
{
	if (read)
	{
		lp->fd_set[ fd ]->t = XT_LP_READ;
		FD_SET( fd, &lp->rfds );
	}
	else
	{
		lp->fd_set[ fd ]->t = XT_LP_WRIT;
		FD_SET( fd, &lp->wfds );
	}
	lp->mxfd = (fd > lp->mxfd) ? fd : lp->mxfd;
}


/**--------------------------------------------------------------------------
 * Add an Timer event handler to the xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
//void xt_lp_addtimer_impl( struct xt_lp *lp, struct timeval *tv )
//{
//}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the xt.Loop
 * \param   luaVM          The lua state.
 * \param   struct xt_lp   The loop struct.
 * --------------------------------------------------------------------------*/
void xt_lp_poll_impl( lua_State *luaVM, struct xt_lp *lp )
{
	int              i,r;
	struct timeval  *tv;
	struct timeval   rt;      ///< timer to calculate runtime over this poll

	gettimeofday( &rt, 0 );
	tv  = (NULL != lp->tm_head) ? lp->tm_head->tv : NULL;

	memcpy( &lp->rfds_w, &lp->rfds, sizeof( fd_set ) );
	memcpy( &lp->wfds_w, &lp->wfds, sizeof( fd_set ) );

	r = select( lp->mxfd+1, &lp->rfds_w, &lp->wfds_w, NULL, tv );

	if (0==r) // deal with timer
	{
		// get, unpack and execute func/parm table
		xt_lp_executetimer( luaVM, lp, &rt );
	}
	else      // deal with sockets/file handles
	{
		for( i=0; r>0 && i <= lp->mxfd; i++ )
		{
			if (NULL==lp->fd_set[ i ])
				continue;
			if (FD_ISSET( i, &lp->rfds_w ) || FD_ISSET( i, &lp->wfds_w ))
			{
				r--;
				xt_lp_executehandle( luaVM, lp, i );
			}
		}
	}
}

