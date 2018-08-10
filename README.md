# love-steam

Steamworks integration for LÖVE

## Usage

Place `love_steam.dll` next to LÖVE's or your game's executable and then do this:

```lua
Steam = require("love_steam")

function love.load()
    Steam.Init()
    if not Steam.API_Init() then 
        error("Fatal error - Steam must be running to play this game (SteamAPI_init() failed).\n") 
    end
end

function love.update(dt)
    Steam.API_RunCallbacks()
end
```

And then you can call the implemented functions from the Steamworks API. If you're testing you'll also need the `steam_appid.txt` file next to the executable. This only works for Windows 32-bit because that's what 
I will personally support, but if you want to make it work for Win64, Linux or OSX then read the [Building](#building) section.

All implemented functions are being tested in `main.lua`, so you can check that file to see what is currently working. For now 
it's a very simple implementation that only covers the basics and ensures callbacks work. As I need to implement features for my
game more of them will come.

<br>

## Motivation

The main motivation for this repository is to hopefully have a central solution for Steamworks integration with LÖVE. 
The main solution that people always point to also doesn't work because it doesn't handle Steamworks' callbacks system 
properly, which means that you can't use the majority of the API. 

And going through the forums I'd say about 10 people released their LÖVE games on Steam and each one of them had to come up
with their own way of solving the problem. This is a big waste of effort and it'd be much better if everyone could use 
and contribute to a single version that is used in many games. The hope for me is that everyone who wants to release their 
games on Steam can come to this repository and use the already implemented functions as well as contribute with implementing 
parts of the API that they need for their games that aren't already implemented.

<br>

## How it works

Steamworks' callback system expects you to use a C++ class, so the simplest way of getting it to work in Lua that I know of
is by making a wrapper around it and then communicating that wrapper with LÖVE via the Lua API. The way I decided to do this
is by making a program `love_steam.cpp` which will implement what I want from the Steamworks API in C++ and then I also make
a few Lua functions (that will be exposed to the LÖVE script) that will call the C++ functions.

The setup in the `love_steam.cpp` file looks like this:

```c++
#include "steam/steam_api.h"
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
}

static const struct luaL_reg love_steam_methods[] = {
    {NULL, NULL}
};

extern "C" {
__declspec(dllexport) int luaopen_love_steam(lua_State *L) {
    luaL_openlib(L, "love_steam", love_steam_methods, 0);
    return 1;
}
}
```

And if we wanted to add the `SteamAPI_Init` call to our library, we would something like this:

```c++
static int love_steam_api_init(lua_State *L) {
    bool status = SteamAPI_Init();
    lua_pushboolean(L, status);
    return 1;
}

static const struct luaL_reg love_steam_methods[] = {
    {"API_Init", love_steam_api_init},
    {NULL, NULL}
};
```

And so with this we would be able to call `Steam.API_Init()` on the Lua code, provided we initialized the module
to the `Steam` variable. This is a pretty straightforward usage of the Lua API. Whenever the user on the Lua side
calls `Steam.API_Init()`, the `love_steam_api_init` function is called and passed in the Lua context (this is the
context that was initialized by LÖVE). And then all we do inside that function is call `SteamAPI_Init`, which is
the Steamworks function for initializing the integration, and then we return the result as a boolean. If you can't
understand exactly what is going on in the function above read up on how the Lua API works, it's very simple :D

And now, for callbacks, following the example from the [Steamworks API Overview](https://partner.steamgames.com/doc/sdk/api) 
page we can do this:

```c++
class GlobalSteam {
    public: 
        GlobalSteam();

    private:
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
```

Here I'm just creating a class, like the page mentions, and then a function to handle the callback that's being implemented,
in this case `GameOverlayActivated_t` (pressing Shift+TAB on a Steam game). When the user presses Shift+TAB the callback will
be called, we get the result (if the overlay was activated or deactivated), and then we call `onGameOverlayActivated` while
passing that result. `onGameOverlayActivated` then is a Lua function that should be defined if you care about that callback
that will be called whenever the overlay is activated or deactivated. 

```lua
function onGameOverlayActivated(status)
    print('Overlay status: ' .. status)
end
```

One important thing to mention here is that I'm referring to `SL`, which is nothing more than a pointer to
the Lua context that was created by LÖVE. This context is set in the `love_steam_init` function and is why you need to call 
`Steam.Init()` before doing anything else.

And finally, the other type of callback we might want to add is one that fetches results asynchronously. And following
the same example as the Steamworks page mentions we could do this:

```c++
class GlobalSteam {
    public: 
        void GetNumberOfCurrentPlayers();
        GlobalSteam();

    private:
        void OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *pCallback, bool bIOFailure);
        CCallResult<GlobalSteam, NumberOfCurrentPlayers_t> m_NumberOfCurrentPlayersCallResult;
        STEAM_CALLBACK(GlobalSteam, OnGameOverlayActivated, GameOverlayActivated_t);
};

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

static int love_steam_userstats_get_number_of_current_players(lua_State *L) {
    global_steam.GetNumberOfCurrentPlayers();
    return 0;
}
```

Here we use the macros that the Steamworks page mentions we should use on the class and define the class methods we need to 
define. Additionally, we define a Lua-only function, in this case called `love_steam_userstats_get_number_of_current_players`, 
which will do nothing more than call `GetNumberOfCurrentPlayers` on our `global_steam` object. On the Lua side we should do two
things: call `love_steam_userstats_get_number_of_current_players`, which is called `UserStats_GetNumberOfCurrentPlayers`, and
then define the callback `onGetNumberOfCurrentPlayers`, which will be called when the result has been fetched by Steam's 
backend:

```lua
function love.keypressed(key)
    if key == 'q' then
        print('Getting number of players...')
        Steam.UserStats_GetNumberOfCurrentPlayers()
    end
end

function onGetNumberOfCurrentPlayers(players)
    print('Number of players: ' .. players)
end
```

And that's it! As far as I can tell, there are these 3 types of functions that we have to deal with: normal functions, callbacks
and asynchronous callbacks. Knowing how to do these should cover the entire API, although I don't know that for sure yet.

<br>

## Building

The following instructions will work for Windows. If you're on Linux/OSX and you end up contributing to this project, please
also change this section to add how to do it on Linux/OSX, since I won't support those systems myself.

To build this project on Windows you'll need to first install 
[Visual Studio 2017 Community](https://visualstudio.microsoft.com/downloads/). After you've installed it you should have
`Developer Command Prompt for VS 2017` available. Open that and then run the `build.bat` from this folder after making the 
following changes:

* Change `C:\Users\gen\Desktop\dev\love-steam\` to the directory where you downloaded this repository;
* Change `C:\Users\gen\Desktop\LOVE11\` to the directory of your installation of LÖVE.

This will build the `love_steam.dll` for you and copy it to your LÖVE installation directory. This works for 32bit LÖVE only!!!

### 64bit 

If you want to make this work for 64bit LÖVE you'll need to change the `lua51.lib` file for a `lua51.lib` that was built for
64bit systems. I built that file from luajit's source, so you can download that and also build it yourself. Similarly, you'll
need to change `steam_api.lib` for the 64bit version, which comes included in the SDK.

### Linux/OSX

[Change this if you're someone who has made this work on those systems]

<br>
