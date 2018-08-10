Steam = require("love_steam")

function love.load()
    Steam.Init()
    Steam.API_Init()
end

function love.update(dt)
    Steam.API_RunCallbacks()
end

function love.draw()
    
end

function love.keypressed(key)
    if key == 'q' then
        print('Getting number of players...')
        Steam.UserStats_GetNumberOfCurrentPlayers()
    end
end

function onGameOverlayActivated(status)
    print('Overlay status: ' .. status)
end

function onGetNumberOfCurrentPlayers(players)
    print('Number of players: ' .. players)
end
