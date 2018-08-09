#include "steam/steam_api.h"
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
}

static int love_steam_init(lua_State *L) {
    bool status = SteamAPI_Init();
    lua_pushboolean(L, status);
    return 1;
}

static const struct luaL_reg love_steam_methods[] = {
    {"API_Init", love_steam_init},
    {NULL, NULL}
};

extern "C" {
__declspec(dllexport) int luaopen_love_steam(lua_State *L) {
    luaL_openlib(L, "love_steam", love_steam_methods, 0);
    return 1;
}
}
