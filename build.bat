@echo off
cd C:\Users\gen\Desktop\dev\love-steam\

call cl /LD love_steam.cpp /I lua /I steam /link /LIBPATH:lua /LIBPATH:steam lua51.lib steam_api.lib /OUT:C:\Users\gen\Desktop\LOVE11\love_steam.dll
call del *.obj
call del *.exp
call del *.lib
