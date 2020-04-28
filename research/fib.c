#define LUA_LIB
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

typedef struct GeneratorState {
    int a;
    int b;
} GeneratorState;

int nextFib(lua_State* L, int status, lua_KContext ctx) {
    if (1 != status) return 0;
    GeneratorState* state = (GeneratorState*)ctx;
    int c = state->a + state->b;
    if (c > 1000000000) {
        return 0;
    }
    state->a = state->b;
    state->b = c;
    lua_pushinteger(L, state->a);
    return lua_yieldk(L, 1, (lua_KContext)state, nextFib);
}

int genFib(lua_State* L) {
    GeneratorState* state = lua_newuserdata(L, sizeof(GeneratorState));
    state->a = 0;
    state->b = 1;
    lua_pushinteger(L, state->a);
    return lua_yieldk(L, 1, (lua_KContext)state, nextFib);
}

int luaopen_fib(lua_State* L) {
    lua_pushcfunction(L, genFib);
    return 1;
}

