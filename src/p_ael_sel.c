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

#include <string.h>           // memcpy
#include <stdlib.h>           // malloc, free


struct p_ael_ste {
	fd_set             rfds;
	fd_set             wfds;
	fd_set             rfds_w; ///<
	fd_set             wfds_w; ///<
};


/**--------------------------------------------------------------------------
 * select() specific initialization of t_ael->state.
 * \param   L      Lua state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  int.
 * --------------------------------------------------------------------------*/
int
p_ael_create_ud_impl( lua_State *L, struct t_ael *ael )
{
	struct p_ael_ste *state = (struct p_ael_ste *) malloc( sizeof( struct p_ael_ste ) );
	if (NULL == state)
		return luaL_error( L, "couldn't allocate memory for loop" );
	FD_ZERO( &state->rfds );
	FD_ZERO( &state->wfds );
	FD_ZERO( &state->rfds_w );
	FD_ZERO( &state->wfds_w );
	ael->state = state;
	return 1;
}


/**--------------------------------------------------------------------------
 * select() specific destruction of t_ael->state.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
void
p_ael_free_impl( struct t_ael *ael )
{
	struct p_ael_ste  *state = ael->state;
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
	UNUSED( L );
	struct p_ael_ste *state = (struct p_ael_ste *) ael->state;
	if (addmsk & T_AEL_RD)    FD_SET( fd, &state->rfds );
	if (addmsk & T_AEL_WR)    FD_SET( fd, &state->wfds );
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
	UNUSED( L );
	struct p_ael_ste *state = (struct p_ael_ste *) ael->state;
	if (delmsk & T_AEL_RD)    FD_CLR( fd, &state->rfds );
	if (delmsk & T_AEL_WR)    FD_CLR( fd, &state->wfds );
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
	struct p_ael_ste *state = (struct p_ael_ste *) ael->state;
	struct timeval   *tv    = (NULL != ael->tmHead)
	   ? ael->tmHead->tv
	   : NULL;
	int               i,r,c = 0;
	int                 msk;

	memcpy( &state->rfds_w, &state->rfds, sizeof( fd_set ) );
	memcpy( &state->wfds_w, &state->wfds, sizeof( fd_set ) );

	r = select( ael->fdMax+1, &state->rfds_w, &state->wfds_w, NULL, tv );
	//printf( "    &&&&&&&&&&&& POLL RETURNED: %d &&&&&&&&&&&&&&&&&&\n", r );

	if (r<0)
		return r;

	if (r>0)
	{
		for (i=0; r>0 && i <= ael->fdMax; i++)
		{
			if (NULL==ael->fdSet[ i ] || T_AEL_NO==ael->fdSet[ i ]->msk)
				continue;
			msk = T_AEL_NO;
			if (ael->fdSet[ i ]->msk & T_AEL_RD  &&  FD_ISSET( i, &state->rfds_w ))
				msk |= T_AEL_RD;
			if (ael->fdSet[ i ]->msk & T_AEL_WR  &&  FD_ISSET( i, &state->wfds_w ))
				msk |= T_AEL_WR;
			if (T_AEL_NO != msk)
			{
				ael->fdExc[ c++ ]      = i;
				ael->fdSet[ i ]->exMsk = msk;
				r--;
			}
		}
	}

	return c;
}

