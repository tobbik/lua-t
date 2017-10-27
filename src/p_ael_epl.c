/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael_epl.c
 * \brief     epoll specific implementation for T.Loop.
 * \detail    Handles implmentation specific functions such as registreing
 *            events and executing the loop.  Being based on the epoll system
 *            call this version shall work on a wide variety of Linux
 *            platforms.  It is supposed to be the standard implementation on
 *            Linux system and the fastest implementation for Linux systems.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_ael_l.h"
#include "t_tim.h"            // t_tim_now, struct timeval

#ifdef DEBUG
#include "t_dbg.h"
#endif

#include <string.h>           // memcpy
#include <stdlib.h>           // malloc, free
#include <unistd.h>           // close
#include <sys/epoll.h>

struct p_ael_ste {
	int                 epfd;
	struct epoll_event *events;
};

/**--------------------------------------------------------------------------
 * epoll specific initialization of t_ael->state.
 * \param   L      Lua state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  int
 * --------------------------------------------------------------------------*/
int
p_ael_create_ud_impl( lua_State *L, struct t_ael *ael )
{
	struct p_ael_ste *state = (struct p_ael_ste *) malloc( sizeof( struct p_ael_ste ) );
	if (NULL == state)
		return luaL_error( L, "couldn't allocate memory for loop" );

	state->events = malloc( sizeof( struct epoll_event ) * ael->fdCount );
	if (!state->events)
	{
		free( state );
		return luaL_error( L, "couldn't create sructure for epoll loop" );
	}

	state->epfd = epoll_create( 1024 );   // 1024 is a kernel hint
	memset( state->events, 0, sizeof( struct epoll_event ) );
	if (state->epfd == -1)
	{
		free( state->events );
		free( state );
		return luaL_error( L, "couldn't create sructure for epoll loop" );
	}
	ael->state = state;
	return 1;
}


/**--------------------------------------------------------------------------
 * Epoll specific destruction of t_ael->state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  int.
 * --------------------------------------------------------------------------*/
void
p_ael_free_impl( struct t_ael *ael )
{
	struct p_ael_ste *state = (struct p_ael_ste *) ael->state;
	close( state->epfd );
	free(  state->events );
	free(  state );
}


/**--------------------------------------------------------------------------
 * Add a File/Socket event handler to the T.Loop.
 * \param  ael    struct t_ael pointer to loop.
 * \param  fd     int  Socket/file descriptor.
 * \param  addmsk enum t_ael_msk - direction of descriptor to be observed.
 * \return int    success/fail;
 * --------------------------------------------------------------------------*/
int
p_ael_addhandle_impl( lua_State *L, struct t_ael *ael, int fd, enum t_ael_msk addmsk )
{
	struct p_ael_ste *state = (struct p_ael_ste *) ael->state;
	//printf("ADDING DESCRIPTOR: %d: {%s + %s = %s}\n", fd,
	//		t_ael_msk_lst[ ael->fdSet[ fd ].msk ],
	//		t_ael_msk_lst[ addmsk ],
	//		t_ael_msk_lst[ addmsk | ael->fdSet[ fd ].msk ] );
	addmsk |= ael->fdSet[ fd ].msk;   // Merge old events
	// If the fd was already monitored for some event, we need a MOD
	// operation. Otherwise we need an ADD operation.
	int                op    = (T_AEL_NO == ael->fdSet[ fd ].msk)
	                           ? EPOLL_CTL_ADD
	                           : EPOLL_CTL_MOD;
	struct epoll_event ee    = {0,{0}}; // avoid valgrind warning

	ee.events = 0;
	if (addmsk & T_AEL_RD) ee.events |= EPOLLIN;
	if (addmsk & T_AEL_WR) ee.events |= EPOLLOUT;
	ee.data.fd = fd;
	if (-1 == epoll_ctl( state->epfd, op, fd, &ee ))
		return t_push_error( L, "Error %s descriptor [%d:%s] to set",
				(op == EPOLL_CTL_MOD) ? "modifying" : "adding",
				fd, t_ael_msk_lst[ addmsk ] );
	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a File/Socket event handler to the T.Loop.
 * \param  ael    struct t_ael pointer to loop.
 * \param  fd     int  Socket/file descriptor.
 * \param  delmsk enum t_ael_msk - direction of descriptor to stop observing.
 * \return int    success/fail;
 * --------------------------------------------------------------------------*/
int
p_ael_removehandle_impl( lua_State *L, struct t_ael *ael, int fd, enum t_ael_msk delmsk )
{
	struct p_ael_ste *state = (struct p_ael_ste *) ael->state;
	//printf( "REMOVING DESCRIPTOR: %d: {%s - %s = %s}\n", fd,
	//		t_ael_msk_lst[ ael->fdSet[ fd ].msk ],
	//		t_ael_msk_lst[ delmsk ],
	//		t_ael_msk_lst[ ael->fdSet[ fd ].msk & (~delmsk) ] );
	delmsk = ael->fdSet[ fd ].msk & (~delmsk);
	// If the fd remains monitored for some event, we need a MOD
	// operation. Otherwise we need an DEL operation.
	int op = (T_AEL_NO != delmsk)
	         ? EPOLL_CTL_MOD
	         : EPOLL_CTL_DEL;
	struct epoll_event ee    = {0,{0}}; // avoid valgrind warning

	ee.events = 0;
	if (delmsk & T_AEL_RD) ee.events |= EPOLLIN;
	if (delmsk & T_AEL_WR) ee.events |= EPOLLOUT;
	ee.data.fd = fd;
	if (-1 == epoll_ctl( state->epfd, op, fd, &ee ))
		return t_push_error( L, "Error %s descriptor [%d:%s] in set",
				(op == EPOLL_CTL_MOD) ? "modifying" : "removing",
				fd, t_ael_msk_lst[ delmsk ] );
	return 1;
}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the T.Loop
 * \param   L              Lua state.
 * \param   struct t_ael   The loop struct.
 * \return  int            number returns from select.
 * --------------------------------------------------------------------------*/
int
p_ael_poll_impl( lua_State *L, struct t_ael *ael )
{
	UNUSED( L );
	struct p_ael_ste   *state = (struct p_ael_ste *) ael->state;
	struct timeval     *tv    = (NULL != ael->tmHead)
	   ? ael->tmHead->tv
	   : NULL;
	struct epoll_event *e;
	int                 i,r,c = 0;
	int                 msk;

	r = epoll_wait(
	   state->epfd,
	   state->events,
	   ael->fdCount,
	   tv ? (tv->tv_sec*1000 + tv->tv_usec/1000) : -1 );
	//printf( "    &&&&&&&&&&&& POLL RETURNED: %d &&&&&&&&&&&&&&&&&&\n", r );

	if (r > 0)
	{
		for (i=0; i<r; i++)
		{
			msk = T_AEL_NO;
			e   = state->events + i;

			if (e->events & EPOLLIN)  msk |= T_AEL_RD;
			if (e->events & EPOLLOUT || e->events & EPOLLERR || e->events & EPOLLHUP) msk |= T_AEL_WR;
			//if (e->events & EPOLLERR) msk |= T_AEL_WR;
			//if (e->events & EPOLLHUP) msk |= T_AEL_WR;
			if (T_AEL_NO != msk)
			{
				//printf( "  _____ FD: %d triggered[%s]____\n", e->data.fd, t_ael_msk_lst[ msk ] );
				ael->fdExc[ c++ ]               = e->data.fd;
				ael->fdSet[ e->data.fd ].exMsk = msk;
			}
		}
	}
	return c;
}

