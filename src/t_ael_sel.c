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
#include <stdlib.h>           // malloc, free

struct t_ael_ste {
	fd_set             rfds;
	fd_set             wfds;
	fd_set             rfds_w;   ///<
	fd_set             wfds_w;   ///<
};

/**--------------------------------------------------------------------------
 * Select() specific initialization of t_ael.
 * \param   struct t_ael * pointer to new userdata on Lua Stack
 * \return  void
 * --------------------------------------------------------------------------*/
void
t_ael_create_ud_impl( lua_State *L, struct t_ael *ael )
{
	struct t_ael_ste *state = (struct t_ael_ste *) malloc( sizeof( struct t_ael_ste ) );
	if (NULL == state)
		luaL_error( L, "couldn't allocate memory for loop" );
	FD_ZERO( &state->rfds );
	FD_ZERO( &state->wfds );
	FD_ZERO( &state->rfds_w );
	FD_ZERO( &state->wfds_w );
	ael->state = state;
}


void t_ael_free_impl( struct t_ael *ael )
{
	struct t_ael_ste  *state = ael->state;
	free(  state );
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
	struct t_ael_ste *state = (struct t_ael_ste *) ael->state;
	if (t & T_AEL_RD)    FD_SET( fd, &state->rfds );
	if (t & T_AEL_WR)    FD_SET( fd, &state->wfds );
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
	struct t_ael_ste *state = (struct t_ael_ste *) ael->state;
	if (t & T_AEL_RD)    FD_CLR( fd, &state->rfds );
	if (t & T_AEL_WR)    FD_CLR( fd, &state->wfds );
}


/**--------------------------------------------------------------------------
 * Add an Timer event handler to the T.Loop.
 * \param   L    The lua state.
 * \lparam  ud   T.Loop userdata instance.                   // 1
 * \lparam  ud   T.Time userdata instance.                   // 2
 * \lparam  func to be executed when event handler fires.    // 4
 * \lparam  ...  parameters to function when executed.       // 5 ...
 * \return  int  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
//void t_ael_addtimer_impl( struct t_ael *ael, struct timeval *tv )
//{
//}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the T.Loop
 * \param   L              Lua state.
 * \param   struct t_ael   The loop struct.
 * \return  int            number returns from select.
 * --------------------------------------------------------------------------*/
int
t_ael_poll_impl( lua_State *L, struct t_ael *ael )
{
	int               i,r;
	struct timeval   *tv;
	struct timeval    rt;           ///< timer to calculate runtime over this poll
	enum t_ael_t      t;            ///< handle action per fd (read/write/either)
	struct t_ael_ste *state = (struct t_ael_ste *) ael->state;

	t_tim_now( &rt, 0 );
	tv  = (NULL != ael->tm_head) ? ael->tm_head->tv : NULL;

	memcpy( &state->rfds_w, &state->rfds, sizeof( fd_set ) );
	memcpy( &state->wfds_w, &state->wfds, sizeof( fd_set ) );

	r = select( ael->max_fd+1, &state->rfds_w, &state->wfds_w, NULL, tv );
	//printf("RESULT: %d\n",r);
	if (r<0)
		return r;

	if (0==r) // deal with timer
		t_ael_executetimer( L, ael, &rt );
	else      // deal with sockets/file handles
		for (i=0; r>0 && i <= ael->max_fd; i++)
		{
			if (NULL == ael->fd_set[ i ])
				continue;
			t = T_AEL_NO;
			if (ael->fd_set[ i ]->t & T_AEL_RD  &&  FD_ISSET( i, &state->rfds_w ))
				t |= T_AEL_RD;
			if (ael->fd_set[ i ]->t & T_AEL_WR  &&  FD_ISSET( i, &state->wfds_w ))
				t |= T_AEL_WR;
			if (T_AEL_NO != t)
			{
				t_ael_executehandle( L, ael, i, t );
				r--;
			}
		}

	return r;
}

