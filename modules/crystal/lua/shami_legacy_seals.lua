-----------------------------------
-- Shami's seal trade for clients before 20110510_0
-- Their event 321 names one seal: p0 = 0 (Beastmen's) or 1 (Kindred's), p1 = the new balance.
-- Today's params (a count << 16 per seal type) leave it naming no item, and the client locks up.
-----------------------------------
require('modules/module_utils')
-----------------------------------
local m = Module:new('crystal_shami_legacy_seals')

local legacySeals =
{
    [0] = xi.item.BEASTMENS_SEAL,
    [1] = xi.item.KINDREDS_SEAL,
}

m:addOverride('xi.zones.Port_Jeuno.npcs.Shami.onTrade', function(player, npc, trade)
    if player:isClientVersionAfter('20110510_0', true) then
        return super(player, npc, trade)
    end

    for sealId, itemId in pairs(legacySeals) do
        local count = trade:getItemQty(itemId)
        if count > 0 and count == trade:getItemCount() then
            trade:confirmItem(itemId, count)
            player:addSeals(count, sealId)
            player:confirmTrade()
            player:startEvent(321, sealId, player:getSeals(sealId))

            return
        end
    end

    -- Crests and mixed seals have no legacy event; the trade is handed back
    for itemId, _ in pairs(xi.seals.sealItems) do
        if trade:getItemQty(itemId) > 0 then
            return
        end
    end

    super(player, npc, trade)
end)
