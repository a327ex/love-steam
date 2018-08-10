#include <stdio.h>
#include "steam/steam_api.h"
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
}

lua_State *SL;

class GlobalSteam {
    public: 
        void GetNumberOfCurrentPlayers();
        GlobalSteam();

    private:
        void OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *pCallback, bool bIOFailure);
        CCallResult<GlobalSteam, NumberOfCurrentPlayers_t> m_NumberOfCurrentPlayersCallResult;
        STEAM_CALLBACK(GlobalSteam, OnGameOverlayActivated, GameOverlayActivated_t);
};

GlobalSteam::GlobalSteam() {

}

void GlobalSteam::OnGameOverlayActivated(GameOverlayActivated_t *pCallback) {
    bool status = pCallback->m_bActive;
    lua_getglobal(SL, "onGameOverlayActivated");
    lua_pushboolean(SL, status);
    lua_pcall(SL, 1, 0, 0);
}

void GlobalSteam::GetNumberOfCurrentPlayers() {
    SteamAPICall_t hSteamAPICall = SteamUserStats()->GetNumberOfCurrentPlayers();
    m_NumberOfCurrentPlayersCallResult.Set(hSteamAPICall, this, &GlobalSteam::OnGetNumberOfCurrentPlayers);
}

void GlobalSteam::OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *pCallback, bool bIOFailure) {
    bool status = !(bIOFailure || !pCallback->m_bSuccess);
    lua_getglobal(SL, "onGetNumberOfCurrentPlayers");
    if (status) lua_pushnumber(SL, pCallback->m_cPlayers);
    else lua_pushboolean(SL, false);
    lua_pcall(SL, 1, 0, 0);
}

GlobalSteam global_steam;

static int love_steam_init(lua_State *L) {
    SL = L;
    return 0;
}

static int love_steam_api_init(lua_State *L) {
    bool status = SteamAPI_Init();
    lua_pushboolean(L, status);
    return 1;
}

static int love_steam_api_restart(lua_State *L) {
    uint32 app_id = (uint32)luaL_checknumber(L, 1);
    bool status = SteamAPI_RestartAppIfNecessary(app_id);
    lua_pushboolean(L, status);
    return 1;
}

static int love_steam_api_run_callbacks(lua_State *L) {
    SteamAPI_RunCallbacks();
    return 0;
}

static int love_steam_userstats_get_number_of_current_players(lua_State *L) {
    global_steam.GetNumberOfCurrentPlayers();
    return 0;
}

static const struct luaL_reg love_steam_methods[] = {
    {"Init", love_steam_init},
    {"API_Init", love_steam_api_init},
    {"API_RestartAppIfNecessary", love_steam_api_restart},
    {"API_RunCallbacks", love_steam_api_run_callbacks},
    {"UserStats_GetNumberOfCurrentPlayers", love_steam_userstats_get_number_of_current_players},
    {NULL, NULL}
};

extern "C" {
__declspec(dllexport) int luaopen_love_steam(lua_State *L) {
    luaL_openlib(L, "love_steam", love_steam_methods, 0);
    return 1;
}
}
