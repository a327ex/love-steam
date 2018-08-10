@echo off
cd C:\Users\gen\Desktop\dev\love-steam\

call cl /EHsc /LD love_steam.cpp /I lua /I steam /link /LIBPATH:lua /LIBPATH:steam lua51.lib steam_api.lib /OUT:love_steam.dll
copy love_steam.dll C:\Users\gen\Desktop\LOVE11\love_steam.dll
del *.obj
del *.exp
del *.lib
