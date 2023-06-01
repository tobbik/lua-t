/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael_kqe.c
 * \brief     epoll specific implementation for T.Loop.
 * \detail    Handles implmentation specific functions such as registreing
 *            events and executing the loop.  Being based on the kqueue system
 *            call this version shall work on a wide variety of BSD
 *            platforms including MacOS.  It is supposed to be the standard 
 *            implementation on BSD systems and the fastest implementation
 *            for BSD systems.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_ael_l.h"
#include <sys/time.h>         // struct timespec

#ifdef DEBUG
#include "t_dbg.h"
#endif

#include <unistd.h>           // close
#include <errno.h>            // EINTR,errno

#include <sys/event.h>        // kqueue'n friends


#define EVENT_MASK_MALLOC_SIZE(sz)   (((sz) + 3) / 4)
#define EVENT_MASK_OFFSET(fd)        ((fd) % 4 * 2)
#define EVENT_MASK_ENCODE(fd, mask)  (((mask) & 0x3) << EVENT_MASK_OFFSET(fd))

extern const char* t_ael_msk_lst[ ];

static inline int getEventMask( const char *eventsMask, int fd )
{
	return (eventsMask[ fd/4 ] >> EVENT_MASK_OFFSET( fd )) & 0x3;
}

static inline void addEventMask( char *eventsMask, int fd, int mask )
{
	eventsMask[ fd/4 ] |= EVENT_MASK_ENCODE( fd, mask );
}

static inline void resetEventMask( char *eventsMask, int fd )
{
	eventsMask[ fd/4 ] &= ~EVENT_MASK_ENCODE( fd, 0x3 );
}


struct p_ael_ste {
	int            kqfd;
	struct kevent  events[ T_AEL_SLOTSIZE ];

	/* Events mask for merge read and write event.
	 * To reduce memory consumption, we use 2 bits to store the mask
	 * of an event, so that 1 byte will store the mask of 4 events. */
	char           eventsMask[ (((T_AEL_SLOTSIZE) + 3) / 4) ];
};


/**--------------------------------------------------------------------------
 * get the state struct from the loop userdata.
 * \param   L     Lua state.
 * \param   ref   int; reference to state.
 * \return  state struct to state.
 * --------------------------------------------------------------------------*/
static inline struct p_ael_ste
*p_ael_getState( lua_State *L, int aelpos )
{
	struct p_ael_ste *state;
	lua_getiuservalue( L, aelpos, T_AEL_STEIDX );
	state = (struct p_ael_ste *) lua_touserdata( L, -1 );
	lua_pop( L, 1 );
	return state;
}


/**--------------------------------------------------------------------------
 * epoll specific initialization of t_ael->state.
 * \param   L   Lua state.
 * \param   ael struct t_ael*; pointer to userdata on Lua Stack.
 * \return  int
 * --------------------------------------------------------------------------*/
void
p_ael_create_ud_impl( lua_State *L )
{
	struct p_ael_ste *state;
	state = (struct p_ael_ste *) lua_newuserdata( L, sizeof( struct p_ael_ste ) );

	state->kqfd = kqueue( );
	if (-1 == state->kqfd)
		luaL_error( L, "couldn't create event socket for kqueue` loop" );
	t_ael_hlp_cloexec( state->kqfd );
}


/**--------------------------------------------------------------------------
 * Kqueue specific destruction of t_ael->state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  int.
 * --------------------------------------------------------------------------*/
void
p_ael_free_impl( lua_State *L, int aelpos )
{
	struct p_ael_ste *state = p_ael_getState( L, aelpos );
	close( state->kqfd );
}


/**--------------------------------------------------------------------------
 * Add a File/Socket event handler to the T.Loop.
 * \param  L      lua_State.
 * \param  aelpos int; position of struct t_ael ud on stack.
 * \param  dnd    struct assiciated with fd.
 * \param  fd     int  Socket/file descriptor.
 * \param  addmsk enum t_ael_msk - direction of descriptor to be observed.
 * \return int    success/fail;
 * --------------------------------------------------------------------------*/
int
p_ael_addhandle_impl( lua_State *L, int aelpos, struct t_ael_dnd *dnd, int fd, enum t_ael_msk addmsk )
{
	struct p_ael_ste *state = p_ael_getState( L, aelpos );
	struct kevent     ke;
	UNUSED( dnd );

	if (addmsk & T_AEL_RD)
	{
		EV_SET( &ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL );
		if (kevent( state->kqfd, &ke, 1, NULL, 0, NULL ) == -1)
			return t_push_error( L, 1, 1, "Error observing readability descriptor [%d:%s]",
			   fd, t_ael_msk_lst[ addmsk ] );
	}
	if (addmsk & T_AEL_WR)
	{
		EV_SET( &ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL );
		if (kevent( state->kqfd, &ke, 1, NULL, 0, NULL ) == -1)
			return t_push_error( L, 1, 1, "Error observing writebility descriptor [%d:%s]",
			   fd, t_ael_msk_lst[ addmsk ] );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a File/Socket event handler to the T.Loop.
 * \param  L      lua_State.
 * \param  aelpos int; position of struct t_ael ud on stack.
 * \param  dnd    struct assiciated with fd.
 * \param  fd     int  Socket/file descriptor.
 * \param  delmsk enum t_ael_msk - direction of descriptor to stop observing.
 * \return int    success/fail;
 * --------------------------------------------------------------------------*/
int
p_ael_removehandle_impl( lua_State *L, int aelpos, struct t_ael_dnd *dnd, int fd, enum t_ael_msk delmsk )
{
	struct p_ael_ste *state = p_ael_getState( L, aelpos );
	struct kevent     ke;
	UNUSED( dnd );

	if (delmsk & T_AEL_RD)
	{
		EV_SET( &ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		if (kevent( state->kqfd, &ke, 1, NULL, 0, NULL ) == -1)
			return t_push_error( L, 1, 1, "Error removing observing readability descriptor [%d:%s]",
			   fd, t_ael_msk_lst[ delmsk ] );
	}
	if (delmsk & T_AEL_WR)
	{
		EV_SET( &ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL );
		if (kevent( state->kqfd, &ke, 1, NULL, 0, NULL ) == -1)
			return t_push_error( L, 1, 1, "Error removing observing writebility descriptor [%d:%s]",
			   fd, t_ael_msk_lst[ delmsk ] );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Set up a epoll_wait() call for all events in the T.Loop
 * \param   L       Lua state.
 * \param   aelpos  int; position of t_ael loop struct on stack.
 * \param   timeout struct timeval; timeout until the next fallthrough.
 * \return  int     number returns from select.
 * --------------------------------------------------------------------------*/
int
p_ael_poll_impl( lua_State *L, struct timeval *timeout, int aelpos )
{
	struct p_ael_ste   *state = p_ael_getState( L, aelpos );
	int    j,retval,numevents = 0;
	struct kevent          *e;
	int              fd, mask;
	struct timespec        ts;

#if PRINT_DEBUGS == 1
	printf( "    &&&&&&&&&&&& SETUP KQUEUE: %ldms &&&&&&&&&&&&&&&&&&\n", T_AEL_TIMEVAL2MS( timeout ) );
#endif

	if (timeout)
	{
		ts.tv_sec  = timeout->tv_sec;
		ts.tv_nsec = timeout->tv_usec * 1000;
		retval = kevent( state->kqfd, NULL, 0, state->events, T_AEL_SLOTSIZE, &ts );
	}
	else
		retval = kevent( state->kqfd, NULL, 0, state->events, T_AEL_SLOTSIZE, NULL);

#if PRINT_DEBUGS == 1
	printf( "    &&&&&&&&&&&& KQUEUE POLLED: %d &&&&&&&&&&&&&&&&&&\n", retval );
#endif

	if (retval > 0)
	{
		/* Normally we execute the read event first and then the write event.
		 * When the barrier is set, we will do it reverse.
		 * 
		 * However, under kqueue, read and write events would be separate
		 * events, which would make it impossible to control the order of
		 * reads and writes. So we store the event's mask we've got and merge
		 * the same fd events later. */
		for (j = 0; j < retval; j++)
		{
			e    = state->events + j;
			fd   = e->ident;
			mask = 0;

			if      (e->filter == EVFILT_READ)  mask = T_AEL_RD;
			else if (e->filter == EVFILT_WRITE) mask = T_AEL_WR;
			addEventMask( state->eventsMask, fd, mask );
		}

		/* Re-traversal to merge read and write events, and set the fd's mask to
		 * 0 so that events are not added again when the fd is encountered again. */
		numevents = 0;
		lua_getiuservalue( L, aelpos, T_AEL_DSCIDX );
		for (j = 0; j < retval; j++)
		{
			e    = state->events+j;
			fd   = e->ident;
			mask = getEventMask( state->eventsMask, fd );

			if (mask)
			{
#if PRINT_DEBUGS == 1
				printf( "  _____ FD: %lu triggered[%s]____\n", e->ident, t_ael_msk_lst[ mask ] );
#endif
				//printf("EPOLL DND  ");t_stackDump(L);
				lua_rawgeti( L, -1, fd );           //S: ael nds dnd
				resetEventMask( state->eventsMask, fd );
				t_ael_dnd_execute( L, t_ael_dnd_check_ud( L, -1, 1 ), mask );
				lua_pop( L, 1 );
				numevents++;
			}
		}
	}
	else if (retval == -1 && EINTR != errno)
		t_push_error(L, 1, 1, "p_ael_poll_impl: kevent() failed, " );

	return numevents;
}

