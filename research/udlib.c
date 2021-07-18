// vim: ts=4 sw=4 st=4 sta tw=80 list
//
// README:  compare regular linked list with uservalue based linked list
//
// the uservalue based list has the advantage of automatically __gc-ing
// all its members when it gets destroyed
//
// preliminary result shows, walking down a uservalue based list takes twice as
// long, which is very respectable and IMHO well worth the benefits (automatic
// garbagecollection). One exception found is this code runs ten times slower
// than the simple linked list on a Chromebook (in termux). That chromebook is
// ARM powered, I have no other chromebooks to test it. It seems to be not
// architecture related since on a RaspberryPi I was able to run it with the
// same twice as long runtime as the linked list. Maybe the reason is in
// Androids BIONIC libc implementation or other memory protecting mechanisms.

#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"

#include "t_dbg.h"
#include <sys/time.h>

// gcc -O0 -g -shared -fpic -o udlib.so udlib.c
// gcc -I src -O0 -shared -g -fpic -o udlib.so udlib.c
// gcc -I src -O0 -shared -g -fpic -o udlib.so udlib.c src/t_dbg.o
//

#define timesub(a, b, result)                            \
	do {                                                 \
	  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;      \
	  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;   \
	  if ((result)->tv_usec < 0) {                       \
	    --(result)->tv_sec;                              \
	    (result)->tv_usec += 1000000;                    \
	  }                                                  \
	} while (0)

static int __gc_calls = 0;

struct tux {
	int         num;
	struct tux *nxt;
};


static int ud( lua_State *L )
{
	int n = 0;
	struct timeval tv1, tv2;
	struct tux *udn;
	struct tux *ud  = (struct tux *) lua_newuserdata( L, sizeof( struct tux ) );
	luaL_getmetatable( L, "udlib" );
	lua_setmetatable( L, -2 );
	ud->num = 0;
	gettimeofday( &tv1, 0 );

	lua_pushvalue( L, -1 );                                              //S: hed pud
	for (; n<luaL_checkinteger( L, 1 ); n+=2)
	{
		udn = (struct tux *) lua_newuserdata( L, sizeof( struct tux ) ); //S: hed pud nud
		luaL_getmetatable( L, "udlib" );
		lua_setmetatable( L, -2 );
		ud->nxt  = udn;
		udn->num = n + 2;
		lua_pushvalue( L, -1 );                                          //S: hed pud nud nud
		lua_setuservalue( L, -3 );                                       //S: hed pud nud
		lua_remove( L, -2 );   // pop previous                           //S: hed nud
		ud = udn;                                                        //S: hed pud
	}
	lua_pop( L, 1 );
	gettimeofday( &tv2, 0 );
	timesub( &tv2, &tv1, &tv2 );
	printf("CREATE: %d  {%2ld:%06ld} \n", n/2 + 1, tv2.tv_sec, tv2.tv_usec );
	return 1;
}


static int walk( lua_State *L )   // Wake down the classic pointer based linked list
{
	struct tux *ud  = (struct tux *) lua_touserdata( L, 1 );
	struct timeval tv1, tv2;
	int          n = 1;
	int p = lua_toboolean( L, 2 );
	if (2 == lua_gettop( L )) lua_pop( L, 1 );
	if (0 != p)
		{ printf("ud->num: %4i -- ", ud->num); t_stackDump( L ); }

	gettimeofday( &tv1, 0 );
	while (NULL != (ud = ud->nxt))
	{
		if (0 != p)
			{ printf("ud->num: %4i -- ", ud->num); t_stackDump( L ); }
		n++;
	}
	gettimeofday( &tv2, 0 );
	timesub( &tv2, &tv1, &tv2 );
	printf("WALK:   %d  {%2ld:%06ld} \n", n, tv2.tv_sec, tv2.tv_usec );
	return 0;
}


static int walkv( lua_State *L )   // Wake down the Lua uservalue based linked list
{
	struct tux __attribute__ ((unused)) *ud = (struct tux *) lua_touserdata( L, 1 );
	struct timeval tv1, tv2;
	int          n = 1;
	int p = lua_toboolean( L, 2 );
	if (2 == lua_gettop( L )) lua_pop( L, 1 );
	if (0 != p)
		{ printf("ud->num: %4i -- ", ud->num); t_stackDump( L ); }

	gettimeofday( &tv1, 0 );
	while (LUA_TNIL != lua_getuservalue( L, -1 ))
	{
		ud = (struct tux *) lua_touserdata( L, -1 );   // this accounts for 30-50% of the time panelty!
		if (0 != p)
			{ printf("ud->num: %4i -- ", ud->num); t_stackDump( L ); }
		lua_remove( L, -2);
		n++;
	}
	gettimeofday( &tv2, 0 );
	timesub( &tv2, &tv1, &tv2 );
	printf("WALKV:  %d  {%2ld:%06ld} \n", n, tv2.tv_sec, tv2.tv_usec );
	return 0;
}


static int adjust( lua_State *L )
{
	struct tux *ud;
	int          n = 1;
	struct timeval tv1, tv2;
	lua_Integer  a = luaL_checkinteger( L, 2 );
	lua_pop( L, 1 );

	gettimeofday( &tv1, 0 );
	while (! lua_isnil( L, -1 ))
	{
		ud = (struct tux *) lua_touserdata( L, -1 );
		ud->num += a;
		n++;
		lua_getuservalue( L, -1 );
		lua_remove( L, -2);
	}
	gettimeofday( &tv2, 0 );
	timesub( &tv2, &tv1, &tv2 );
	printf("ADJUST: %d  {%2ld:%06ld} \n", n, tv2.tv_sec, tv2.tv_usec );
	return 0;
}


static int insert( lua_State *L )
{
	struct timeval tv1, tv2;
	struct tux  *ud  = (struct tux *) lua_touserdata( L, 1 );
	struct tux  *udp;            // reference to previous ud
	struct tux *ins  = (struct tux *) lua_newuserdata( L, sizeof( struct tux ) );
	gettimeofday( &tv1, 0 );
	ins->num         = luaL_checkinteger( L, 2 );      //S: hed n ins
	luaL_getmetatable( L, "udlib" );                   //S: hed n ins lib
	lua_setmetatable( L, -2 );                         //S: hed n ins
	lua_rotate( L, -3, -1 );                           //S: n ins hed
	lua_remove( L, 1 );                                //S: ins hed
	// special case for the head
	//printf("ud->num: %d  %p ", ud->num, ud); t_stackDump(L);
	if (ud->num >= ins->num)
	{
		ins->nxt = ud;
		lua_setuservalue( L, -2 );                     //S: ins
	}
	else                                               //S: ins hed
	{
		lua_pushvalue( L, -1 );                        //S: ins hed rud
		while (LUA_TNIL != lua_getuservalue( L, -1 ))
		{                                              //S: ins hed rud xud
			ud  = (struct tux *) lua_touserdata( L, -1 );
			//printf("ud->num: %d  %p ", ud->num, ud); t_stackDump(L);
			if (ud->num >= ins->num)
				break;
			lua_remove( L, -2 );                       //S: ins hed xud
			udp = ud;
		}
		//printf("final:"); t_stackDump(L);
		if (! lua_isnil( L, -1 ))                      //S: ins hed pud xud
		{
			lua_setuservalue( L, -4 );                 //S: ins hed pud        // link xud to ins
			ins->nxt = ud;
		}
		else                                           //S: ins hed pud nil
		{
			lua_pop( L, 1 );                           //S: ins hed pud
			ins->nxt = NULL;
		}
		lua_rotate( L, -3, -1 );                       //S: hed pud ins
		lua_setuservalue( L, -2 );                     //S: hed pud
		udp->nxt = ins;
		lua_pop( L, 1 );                               //S: hed
	}
	gettimeofday( &tv2, 0 );
	timesub( &tv2, &tv1, &tv2 );
	printf("INSERT: %d  {%2ld:%06ld} \n", ins->num, tv2.tv_sec, tv2.tv_usec );
	return 1;
}


// this is just reference for how to insert into a uservalue only list
static int insertv( lua_State *L )
{
	struct tux  *ud  = (struct tux *) lua_touserdata( L, 1 );
	struct tux *ins  = (struct tux *) lua_newuserdata( L, sizeof( struct tux ) );
	ins->num         = luaL_checkinteger( L, 2 );      //S: hed n ins
	luaL_getmetatable( L, "udlib" );                   //S: hed n ins lib
	lua_setmetatable( L, -2 );                         //S: hed n ins
	lua_rotate( L, -3, -1 );                           //S: n ins hed
	lua_remove( L, 1 );                                //S: ins hed
	// special case for the head
	//printf("ud->num: %d  %p ", ud->num, ud); t_stackDump(L);
	if (ud->num >= ins->num)
		lua_setuservalue( L, -2 );                     //S: ins
	else                                               //S: ins hed
	{
		lua_pushvalue( L, -1 );                        //S: ins hed rud
		while (LUA_TNIL != lua_getuservalue( L, -1 ))
		{                                              //S: ins hed rud xud
			ud  = (struct tux *) lua_touserdata( L, -1 );
			//printf("ud->num: %d  %p ", ud->num, ud); t_stackDump(L);
			if (ud->num >= ins->num)
				break;
			lua_remove( L, -2 );                        //S: ins hed xud
		}
		//printf("final:"); t_stackDump(L);
		if (! lua_isnil( L, -1 ))                       //S: ins hed pud xud
			lua_setuservalue( L, -4 );                  //S: ins hed pud        // link xud to ins
		else                                            //S: ins hed pud nil
			lua_pop( L, 1 );                            //S: ins hed pud
		lua_rotate( L, -3, -1 );                        //S: hed pud ins
		lua_setuservalue( L, -2 );                      //S: hed pud
		lua_pop( L, 1 );                                //S: hed
	}
	return 1;
}

static int gc_calls( lua_State *L )
{
	lua_pushinteger( L, __gc_calls );
	return 1;
}


static int
lt_udl__gc( lua_State *L )
{
	struct tux __attribute__ ((unused)) *ud  = (struct tux *) lua_touserdata( L, 1 );
	//printf("Running __gc on: %d\n", ud->num );
	ud->nxt = NULL;
	__gc_calls++;
	return 0;
}

/**--------------------------------------------------------------------------
 * Uservalue lib object method library definition
 * Assigns Lua available names to C-functions on T.Numarray instances
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_udl_m [] = {
	  { "__gc"    , lt_udl__gc }
	, { NULL      , NULL }
};

static const luaL_Reg udlib[] = {
	{ "ud"       , ud       },
	{ "walk"     , walk     },
	{ "walkv"    , walkv    },
	{ "adjust"   , adjust   },
	{ "insert"   , insert   },
	{ "insertv"  , insertv  },
	{ "gccount"  , gc_calls },
	{ NULL       , NULL     }          // mandatory sentinel
};

// MUST reflect the filename (luaopen_<filename> if you want to use require( )
int luaopen_udlib( lua_State *L ) {
	// udlib metatable
	luaL_newmetatable( L, "udlib" );
	luaL_setfuncs( L, t_udl_m, 0 );
	lua_pop( L, 1 );        // remove metatable from stack

	luaL_newlib( L, udlib );
	return 1;
}

