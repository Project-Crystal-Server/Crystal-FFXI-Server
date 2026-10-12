-----------------------------------
-- func: hp2010
-- desc: Home Points of the 2010 PS2 client (today's Home Point #1 per zone); list them and teleport to each.
--   !hp2010            list
--   !hp2010 <n>        go to entry n
--   !hp2010 next|prev  step from the last entry visited
-----------------------------------
require('modules/module_utils')
-----------------------------------

-- zone, 2010 event (5820 + zone), position of Home Point #1 (data/zones, era/data/rov/npc_list where relocated)
local homePoints =
{
    { zone = xi.zone.TAVNAZIAN_SAFEHOLD,     event = 250,   pos = {   -1.250, -27.907,  107.425 } },
    { zone = xi.zone.AL_ZAHBI,               event = 2,     pos = nil }, -- cutscene_only here, no position
    { zone = xi.zone.AHT_URHGAN_WHITEGATE,   event = 501,   pos = {  -21.130,   0.000,  -20.944 } },
    { zone = xi.zone.NASHMAU,                event = 238,   pos = {  -19.860,   0.000,  -25.441 } },
    { zone = xi.zone.SOUTHERN_SAN_DORIA_S,   event = 1,     pos = {  -85.468,   1.000,  -66.454 } },
    { zone = xi.zone.BASTOK_MARKETS_S,       event = 1,     pos = { -293.048, -10.000, -102.558 } },
    { zone = xi.zone.WINDURST_WATERS_S,      event = 1,     pos = {  -32.022,  -5.000,  131.741 } },
    { zone = xi.zone.SOUTHERN_SAN_DORIA,     event = 596,   pos = {  -85.468,   1.000,  -66.454 } },
    { zone = xi.zone.NORTHERN_SAN_DORIA,     event = 599,   pos = { -178.101,   4.000,   71.279 } },
    { zone = xi.zone.PORT_SAN_DORIA,         event = 592,   pos = {  -66.077,  -4.000, -104.948 } },
    { zone = xi.zone.BASTOK_MINES,           event = 80,    pos = {   39.189,   0.000,  -42.618 } },
    { zone = xi.zone.BASTOK_MARKETS,         event = 1,     pos = { -293.791, -10.000, -102.539 } },
    { zone = xi.zone.PORT_BASTOK,            event = 94,    pos = {   53.645,   7.500,  -28.682 } },
    { zone = xi.zone.WINDURST_WATERS,        event = 10010, pos = {  -32.022,  -5.000,  131.741 } },
    { zone = xi.zone.PORT_WINDURST,          event = 10001, pos = {  -67.980,  -4.000,  110.675 } },
    { zone = xi.zone.WINDURST_WOODS,         event = 10001, pos = {    9.088,  -2.500,   -0.383 } },
    { zone = xi.zone.UPPER_JEUNO,            event = 10001, pos = {  -98.981,   0.000,  167.569 } },
    { zone = xi.zone.LOWER_JEUNO,            event = 10001, pos = {  -98.588,   0.000, -183.416 } },
    { zone = xi.zone.PORT_JEUNO,             event = 10000, pos = {   37.076,   0.000,    8.831 } },
    { zone = xi.zone.RABAO,                  event = 1000,  pos = {  -29.276,   0.000,  -76.585 } },
    { zone = xi.zone.SELBINA,                event = 1000,  pos = {   36.117, -10.729,   34.635 } },
    { zone = xi.zone.MHAURA,                 event = 1000,  pos = {  -12.750, -15.791,   87.286 } },
    { zone = xi.zone.KAZHAM,                 event = 10010, pos = {   77.654, -13.000,  -94.457 } },
    { zone = xi.zone.NORG,                   event = 1000,  pos = {  -26.910,   0.296,  -47.164 } },
}

local lastVar = '[ps2]hp2010'
local offset  = 2 -- yalms off the crystal on x and z

local function zoneName(zoneId)
    local zone = GetZone(zoneId)
    return zone and zone:getName() or tostring(zoneId)
end

local function say(player, text)
    player:printToPlayer(text, xi.msg.channel.SYSTEM_3)
end

local function list(player)
    for i, hp in ipairs(homePoints) do
        say(player, string.format('%2d  %-26s zone %3d  event %5d%s', i, zoneName(hp.zone), hp.zone, hp.event, hp.pos and '' or '  (not usable)'))
    end
end

local function go(player, i)
    local hp = homePoints[i]
    player:setCharVar(lastVar, i)
    if not hp.pos then
        say(player, string.format('%d/%d %s: no usable Home Point on this server, skipping', i, #homePoints, zoneName(hp.zone)))
        return false
    end

    say(player, string.format('%d/%d %s: Home Point #1, expect 2010 event %d', i, #homePoints, zoneName(hp.zone), hp.event))
    player:setPos(hp.pos[1] + offset, hp.pos[2], hp.pos[3] + offset, 0, hp.zone)
    return true
end

local function step(player, dir)
    local i = player:getCharVar(lastVar)
    for _ = 1, #homePoints do
        i = (i - 1 + dir) % #homePoints + 1
        if homePoints[i].pos then
            return go(player, i)
        end
        player:setCharVar(lastVar, i)
    end
end

---@type TCommand
local commandObj = {}

commandObj.cmdprops =
{
    permission = 1,
    parameters = 's',
}

commandObj.onTrigger = function(player, arg)
    if arg == nil or arg == '' or arg == 'list' then
        list(player)
        say(player, '!hp2010 <n> | next | prev')
    elseif arg == 'next' then
        step(player, 1)
    elseif arg == 'prev' then
        step(player, -1)
    else
        local i = tonumber(arg)
        if not i or not homePoints[i] then
            say(player, string.format('!hp2010 <1-%d> | next | prev | list', #homePoints))
            return
        end

        go(player, i)
    end
end

xi.module.registerCommand('hp2010', commandObj)
