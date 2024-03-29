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
#include <sys/time.h>         // struct timeval

#ifdef DEBUG
#include "t_dbg.h"
#endif

#include <unistd.h>           // close
#include <sys/epoll.h>

#define P_AEL_EPL_SLOTSZ 1024 // how many events can be returned for ONE call to epoll_wait()
                              // NOT how many fd can be observed, which is limited by ulimit!

static const char* t_ael_msk_lst[ ] = {
	  "NONE"
	, "READ"
	, "WRITE"
	, "READWRITE"
};

struct p_ael_ste {
	int                 epfd;
	struct epoll_event events[ P_AEL_EPL_SLOTSZ ];
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
	lua_getiuservalue(L, aelpos, T_AEL_STEIDX );
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

	state->epfd = epoll_create1( 0 );
	if (state->epfd == -1)
		luaL_error( L, "couldn't create event socket for epoll loop" );
}


/**--------------------------------------------------------------------------
 * Epoll specific destruction of t_ael->state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  int.
 * --------------------------------------------------------------------------*/
void
p_ael_free_impl( lua_State *L, int aelpos )
{
	struct p_ael_ste *state = p_ael_getState( L, aelpos );
	close( state->epfd );
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
#if PRINT_DEBUGS == 1
	printf("+++++ ADDING DESCRIPTOR: %d: {%s + %s = %s}\n", fd,
			t_ael_msk_lst[ dnd->msk ],
			t_ael_msk_lst[ addmsk ],
			t_ael_msk_lst[ addmsk | dnd->msk ] );
#endif
	addmsk |= dnd->msk;   // Merge old events
	// If the fd was already monitored for some event, we need a MOD
	// operation. Otherwise we need an ADD operation.
	int                op    = (T_AEL_NO == dnd->msk)
	                           ? EPOLL_CTL_ADD
	                           : EPOLL_CTL_MOD;
	struct epoll_event ee    = {0,{0}}; // avoid valgrind warning

	ee.events = 0;
	if (addmsk & T_AEL_RD) ee.events |= EPOLLIN;
	if (addmsk & T_AEL_WR) ee.events |= EPOLLOUT;
	ee.data.fd = fd;
	if (-1 == epoll_ctl( state->epfd, op, fd, &ee ))
		return t_push_error( L, 1, 1, "Error %s descriptor [%d:%s] to set",
				(op == EPOLL_CTL_MOD) ? "modifying" : "adding",
				fd, t_ael_msk_lst[ addmsk ] );
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
#if PRINT_DEBUGS == 1
	printf( "----- REMOVING DESCRIPTOR: %d: {%s - %s = %s}\n", fd,
			t_ael_msk_lst[ dnd->msk ],
			t_ael_msk_lst[ delmsk ],
			t_ael_msk_lst[ dnd->msk & (~delmsk) ] );
#endif
	delmsk = dnd->msk & (~delmsk);
	int op = (T_AEL_NO != delmsk) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
	struct epoll_event ee    = {0,{0}}; // avoid valgrind warning
	ee.events = 0;
	if (delmsk & T_AEL_RD) ee.events |= EPOLLIN;
	if (delmsk & T_AEL_WR) ee.events |= EPOLLOUT;
	ee.data.fd = fd;
	if (-1 == epoll_ctl( state->epfd, op, fd, &ee ))
		return t_push_error( L, 1, 1, "Error %s descriptor [%d:%s] in set",
				(op == EPOLL_CTL_MOD) ? "modifying" : "removing",
				fd, t_ael_msk_lst[ delmsk ] );
	return 1;
}


/**--------------------------------------------------------------------------
 * Set up a epoll_wait() call for all events in the T.Loop
 * \param   L       Lua state.
 * \param   aelpos  int; position of t_ael loop struct on stack.
 * \param   timeout int; timeout for next fallthrough in milliseconds.
 * \return  int     number returns from select.
 * --------------------------------------------------------------------------*/
int
p_ael_poll_impl( lua_State *L, int timeout, int aelpos )
{
	struct p_ael_ste   *state = p_ael_getState( L, aelpos );
	struct epoll_event *e;
	int                 i,r,c = 0;
	int                 msk;

	//printf("EPOLL TIMEOUT: %lld -- ", timeout); t_stackDump(L);
	r = epoll_wait(
	   state->epfd,
	   state->events,
	   P_AEL_EPL_SLOTSZ,
	   (timeout > T_AEL_NOTIMEOUT)
	      ? timeout
	      : -1
	);
#if PRINT_DEBUGS == 1
	printf( "    &&&&&&&&&&&& POLL RETURNED: %d &&&&&&&&&&&&&&&&&&\n", r );
#endif
	if (r<0)
		return t_push_error( L, 1, 1, "epoll_wait() failed" );

	if (r > 0)
	{
		lua_getiuservalue( L, aelpos, T_AEL_DSCIDX );
		for (i=0; i<r; i++)
		{
			msk = T_AEL_NO;
			e   = state->events + i;

			if (e->events & EPOLLIN)  msk |= T_AEL_RD;
			if (e->events & EPOLLOUT || e->events & EPOLLERR || e->events & EPOLLHUP) msk |= T_AEL_WR;
			if (T_AEL_NO != msk)
			{
#if PRINT_DEBUGS == 1
				printf( "  _____ FD: %d triggered[%s]____\n", e->data.fd, t_ael_msk_lst[ msk ] );
#endif
				//printf("EPOLL DND  ");t_stackDump(L);
				lua_rawgeti( L, -1, e->data.fd );           //S: ael nds dnd
				t_ael_dnd_execute( L, t_ael_dnd_check_ud( L, -1, 1 ), msk );
				lua_pop( L, 1 );
				c++;
			}
		}
		lua_pop( L, 1 );
	}
	//printf("EPOLLED TIMEOUT: %lld -- ", timeout); t_stackDump(L);
	return c;
}

