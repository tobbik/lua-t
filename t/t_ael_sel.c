/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael_sel.c
 * \brief     select() specific implementation for T.Loop.
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
#include "t_ael.h"

#include <string.h>           // memcpy
#ifndef _WIN32
#include <sys/time.h>         // gettimeofday
#endif


/**--------------------------------------------------------------------------
 * Select() specific initialization of t_ael.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  void
 * --------------------------------------------------------------------------*/
void
t_ael_create_ud_impl( struct t_ael *ael )
{
	FD_ZERO( &ael->rfds );
	FD_ZERO( &ael->wfds );
	FD_ZERO( &ael->rfds_w );
	FD_ZERO( &ael->wfds_w );
}


/**--------------------------------------------------------------------------
 * Add a File/Socket event handler to the T.Loop.
 * \param   struct t_ael*.
 * \param   int          fd.
 * \param   enum t_ael_t t - direction of socket to be observed.
 * --------------------------------------------------------------------------*/
void
t_ael_addhandle_impl( struct t_ael *ael, int fd, enum t_ael_t t )
{
	if (t & T_AEL_RD)    FD_SET( fd, &ael->rfds );
	if (t & T_AEL_WR)    FD_SET( fd, &ael->wfds );
}


/**--------------------------------------------------------------------------
 * Remove a File/Socket event handler to the T.Loop.
 * \param   struct t_ael*.
 * \param   int          fd.
 * \param   enum t_ael_t t - direction of socket to be observed.
 * --------------------------------------------------------------------------*/
void
t_ael_removehandle_impl( struct t_ael *ael, int fd, enum t_ael_t t )
{
	if (t & T_AEL_RD)    FD_CLR( fd, &ael->rfds );
	if (t & T_AEL_WR)    FD_CLR( fd, &ael->wfds );
}


/**--------------------------------------------------------------------------
 * Add an Timer event handler to the T.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata T.Loop.                                     // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
//void t_ael_addtimer_impl( struct t_ael *ael, struct timeval *tv )
//{
//}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the T.Loop
 * \param   luaVM         The lua state.
 * \param   struct t_ael   The loop struct.
 * \return  number returns from select.
 * --------------------------------------------------------------------------*/
int
t_ael_poll_impl( lua_State *luaVM, struct t_ael *ael )
{
	int              i,r;
	struct timeval  *tv;
	struct timeval   rt;           ///< timer to calculate runtime over this poll
	enum t_ael_t     t = T_AEL_NO; ///< handle action (read/write/either)

	gettimeofday( &rt, 0 );
	tv  = (NULL != ael->tm_head) ? ael->tm_head->tv : NULL;

	memcpy( &ael->rfds_w, &ael->rfds, sizeof( fd_set ) );
	memcpy( &ael->wfds_w, &ael->wfds, sizeof( fd_set ) );

	r = select( ael->mxfd+1, &ael->rfds_w, &ael->wfds_w, NULL, tv );
	//printf("RESULT: %d\n",r);
	if (r<0)
		return r;

	if (0==r) // deal with timer
		t_ael_executetimer( luaVM, ael, &rt );
	else      // deal with sockets/file handles
		for( i=0; r>0 && i <= ael->mxfd; i++ )
		{
			if (NULL == ael->fd_set[ i ])
				continue;
			if (ael->fd_set[ i ]->t & T_AEL_RD  &&  FD_ISSET( i, &ael->rfds_w ))
				t |= T_AEL_RD;
			if (ael->fd_set[ i ]->t & T_AEL_WR  &&  FD_ISSET( i, &ael->wfds_w ))
				t |= T_AEL_WR;
			t_ael_executehandle( luaVM, ael, i, t );
			r--;
		}

	return r;
}

