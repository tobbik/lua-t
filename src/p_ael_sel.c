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

#include "t_ael_l.h"
#include "t_tim.h"            // t_tim_now, struct timeval

#ifdef DEBUG
#include "t_dbg.h"
#endif

// implementation specific is that this is limited in size to FD_SETSIZE which
// ususally is in the neighboorhood of 1024.  That keeps the size very
// reasonable and makes it static. This way we never have to resize and that
// allows it to be excluded from any resize efforts/necessities.  If there is a
// system where FD_SETSIZE is manipulated and much bigger, using the select()
// based Loop is probably the wrong choice to begin with.
struct p_ael_ste {
	fd_set rfds;
	fd_set wfds;
	fd_set rfds_w;                ///<
	fd_set wfds_w;                ///<
	int    fdMax;                 ///< max fd
	int    fd_set [ FD_SETSIZE ]; ///< fd_set[fd]==1 if fd is part of set; else ==0
};


/* --------------------------------------------------------------------------
 * get the state struct from the loop userdata.
 * \param   L     Lua state.
 * \param   ref   int; reference to state.
 * \return  state struct to state.
 * --------------------------------------------------------------------------*/
static inline struct p_ael_ste
*p_ael_getState( lua_State *L, int ref )
{
	struct p_ael_ste *state;
	lua_rawgeti( L, LUA_REGISTRYINDEX, ref );
	state = (struct p_ael_ste *) lua_touserdata( L, -1 );
	lua_pop( L, 1 );
	return state;
}


/* --------------------------------------------------------------------------
 * select() specific initialization of t_ael->state.
 * \param   L      Lua state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  int.
 * --------------------------------------------------------------------------*/
int
p_ael_create_ud_impl( lua_State *L )
{
	struct p_ael_ste *state;
	state = (struct p_ael_ste *) lua_newuserdata( L, sizeof( struct p_ael_ste ) );
	FD_ZERO( &state->rfds );
	FD_ZERO( &state->wfds );
	FD_ZERO( &state->rfds_w );
	FD_ZERO( &state->wfds_w );
	return luaL_ref( L, LUA_REGISTRYINDEX );
}


/* --------------------------------------------------------------------------
 * select() specific destruction of t_ael->state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
void
p_ael_free_impl( lua_State *L, int ref )
{
	UNUSED( L );
	UNUSED( ref );
}


/* --------------------------------------------------------------------------
 * Add a File/Socket event handler to the T.Loop.
 * \param  L      lua_State.
 * \param  ael    struct t_ael pointer to loop.
 * \param  dnd    struct assiciated with fd.
 * \param  fd     int  Socket/file descriptor.
 * \param  addmsk enum t_ael_msk - direction of descriptor to be observed.
 * \return int    success/fail;
 * --------------------------------------------------------------------------*/
int
p_ael_addhandle_impl( lua_State *L, struct t_ael *ael, struct t_ael_dnd *dnd, int fd, enum t_ael_msk addmsk )
{
	UNUSED( L );
	struct p_ael_ste *state = p_ael_getState( L, ael->sR );
#if PRINT_DEBUGS == 1
	printf("+++++ ADDING DESCRIPTOR: %d: {%s + %s = %s}\n", fd,
			t_ael_msk_lst[ dnd->msk ],
			t_ael_msk_lst[ addmsk ],
			t_ael_msk_lst[ addmsk | dnd->msk ] );
#else
	UNUSED( dnd );
#endif
	if (addmsk & T_AEL_RD)    FD_SET( fd, &state->rfds );
	if (addmsk & T_AEL_WR)    FD_SET( fd, &state->wfds );
	state->fdMax        = (fd > state->fdMax) ? fd : state->fdMax;
	state->fd_set[ fd ] = 1;
	return 1;
}


/* --------------------------------------------------------------------------
 * Remove a File/Socket event handler to the T.Loop.
 * \param  L      lua_State.
 * \param  ael    struct t_ael pointer to loop.
 * \param  dnd    struct assiciated with fd.
 * \param  fd     int  Socket/file descriptor.
 * \param  delmsk enum t_ael_msk - direction of descriptor to stop observing.
 * \return int    success/fail;
 * --------------------------------------------------------------------------*/
int
p_ael_removehandle_impl( lua_State *L, struct t_ael *ael, struct t_ael_dnd *dnd, int fd, enum t_ael_msk delmsk )
{
	UNUSED( L );
	struct p_ael_ste *state = p_ael_getState( L, ael->sR );
	int               i;
#if PRINT_DEBUGS == 1
	printf( "----- REMOVING DESCRIPTOR: %d: {%s - %s = %s}    MAX: %2d\n", fd,
			t_ael_msk_lst[ dnd->msk ],
			t_ael_msk_lst[ delmsk ],
			t_ael_msk_lst[ dnd->msk & (~delmsk) ],
			state->fdMax );
#else
	UNUSED( dnd );
#endif
	if (delmsk & T_AEL_RD)    FD_CLR( fd, &state->rfds );
	if (delmsk & T_AEL_WR)    FD_CLR( fd, &state->wfds );
	if (T_AEL_NO == (dnd->msk & (~delmsk)))
	{
		state->fd_set[ fd ] = 0;
		if (fd == state->fdMax)
		{
			for (i = state->fdMax-1; i >= 0; i--)
				if (state->fd_set[ i ]) break;
			state->fdMax = i;
		}
	}
	return 1;
}


/* --------------------------------------------------------------------------
 * Set up a select call for all events in the T.Loop
 * \param   L    Lua state.
 * \param   ael  struct t_ael;   The loop struct.
 * \return  int  number returns from select.
 * --------------------------------------------------------------------------*/
int
p_ael_poll_impl( lua_State *L, struct t_ael *ael )
{
	UNUSED( L );
	struct p_ael_ste *state = p_ael_getState( L, ael->sR );
	struct t_ael_dnd *dnd;
	struct timeval   *tv    = (NULL != ael->tmHead)
	   ? ael->tmHead->tv
	   : NULL;
	int               i,r,c = 0;
	int                 msk;

	memcpy( &state->rfds_w, &state->rfds, sizeof( fd_set ) );
	memcpy( &state->wfds_w, &state->wfds, sizeof( fd_set ) );

	r = select( state->fdMax+1, &state->rfds_w, &state->wfds_w, NULL, tv );
#if PRINT_DEBUGS == 1
	printf( "    &&&&&&&&&&&& POLL RETURNED[%d:%d]: %d &&&&&&&&&&&&&&&&&&\n", ael->fdCount, state->fdMax, r );
#endif

	if (r<0)
		return t_push_error( L, "select() failed" );

	if (r>0)
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, ael->dR );
		for (i=0; r>0 && i <= state->fdMax; i++)
		{
			lua_rawgeti( L, -1, i );
			if (lua_isnil( L, -1 ))
			{
				lua_pop( L, 1 );
				continue;
			}

			dnd = (struct t_ael_dnd*) lua_touserdata( L, -1 );
			msk = T_AEL_NO;
			if (dnd->msk & T_AEL_RD  &&  FD_ISSET( i, &state->rfds_w ))
				msk |= T_AEL_RD;
			if (dnd->msk & T_AEL_WR  &&  FD_ISSET( i, &state->wfds_w ))
				msk |= T_AEL_WR;
			if (T_AEL_NO != msk)
			{
				t_ael_executehandle( L, dnd, msk );
				r--;
			}
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );
	}

	return c;
}

