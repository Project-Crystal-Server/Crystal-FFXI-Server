/*
===========================================================================

  Copyright (c) 2026 LandSandBoat Dev Teams

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

===========================================================================
*/

#include "ps2_ids.h"

#include "common/logging.h"

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace ps2::ids
{

namespace
{

constexpr uint16 kNone = 0xFFFF;

// A res/ps2 per-zone index map: 8-byte magic, u32 zones, then per zone u16 zone, u32 n, u16 map[n]
// (0xFFFF = no counterpart). `reverse` holds the inverse (2010 -> today) when asked for.
struct ZoneMaps
{
    std::unordered_map<uint16, std::vector<uint16>> zones;
    std::unordered_map<uint16, std::vector<uint16>> reverse;

    ZoneMaps(const char* path, const char* magicExpected, const bool withReverse)
    {
        std::ifstream f(path, std::ios::binary);
        char          magic[8] = {};
        uint32        count    = 0;
        if (!f.read(magic, sizeof(magic)) || std::memcmp(magic, magicExpected, 8) != 0 || !f.read(reinterpret_cast<char*>(&count), 4))
        {
            ShowWarning("ps2: %s missing or invalid; its ids are not remapped for PS2 clients", path);
            return;
        }
        for (uint32 i = 0; i < count; ++i)
        {
            uint16 zone = 0;
            uint32 n    = 0;
            if (!f.read(reinterpret_cast<char*>(&zone), 2) || !f.read(reinterpret_cast<char*>(&n), 4) || n > 0x10000)
            {
                ShowWarning("ps2: %s truncated after %u zones", path, i);
                return;
            }
            auto& map = zones[zone];
            map.resize(n);
            f.read(reinterpret_cast<char*>(map.data()), static_cast<std::streamsize>(n) * 2);

            if (withReverse)
            {
                auto& rev = reverse[zone];
                for (uint32 k = 0; k < n; ++k)
                {
                    if (map[k] == kNone)
                    {
                        continue;
                    }
                    if (rev.size() <= map[k])
                    {
                        rev.resize(map[k] + 1, kNone);
                    }
                    if (rev[map[k]] == kNone)
                    {
                        rev[map[k]] = static_cast<uint16>(k);
                    }
                }
            }
        }
        ShowInfo("ps2: %s: %u zones", path, count);
    }

    static auto lookup(const std::unordered_map<uint16, std::vector<uint16>>& maps, const uint16 zoneId, const uint16 id) -> std::optional<uint16>
    {
        const auto it = maps.find(zoneId);
        if (it == maps.end() || id >= it->second.size() || it->second[id] == kNone)
        {
            return std::nullopt;
        }
        return it->second[id];
    }
};

auto dialogMaps() -> const ZoneMaps&
{
    static const ZoneMaps maps("res/ps2/dialog_map.bin", "PS2DLG01", false);
    return maps;
}

auto entityMaps() -> const ZoneMaps&
{
    static const ZoneMaps maps("res/ps2/entity_map.bin", "PS2ENT01", true);
    return maps;
}

constexpr uint16 kFirstDynamicIndex = 0x400; // players from here, then pets/trusts

auto remapUniqueNo(const uint32 uniqueNo, const bool toPS2) -> std::optional<uint32>
{
    if (!isStatic(uniqueNo))
    {
        return uniqueNo;
    }
    const auto zone  = static_cast<uint16>((uniqueNo >> 12) & 0xFFF);
    const auto index = static_cast<uint16>(uniqueNo & 0xFFF);
    const auto other = toPS2 ? actIndexToPS2(zone, index) : actIndexFromPS2(zone, index);
    if (!other)
    {
        return std::nullopt;
    }
    return (uniqueNo & ~0xFFFu) | *other;
}

} // namespace

auto dialog(const uint16 zoneId, const uint16 messageId) -> std::optional<uint16>
{
    return ZoneMaps::lookup(dialogMaps().zones, zoneId, messageId);
}

auto hasZone(const uint16 zoneId) -> bool
{
    return dialogMaps().zones.contains(zoneId);
}

auto hasItem(const uint16 itemId) -> bool
{
    // res/ps2/items.bin: "PS2ITM01", then 0x10000 bits (bit = item id)
    static const std::vector<uint8> bits = []
    {
        constexpr auto    path = "res/ps2/items.bin";
        std::ifstream     f(path, std::ios::binary);
        char              magic[8] = {};
        std::vector<uint8> data(0x2000);
        if (!f.read(magic, sizeof(magic)) || std::memcmp(magic, "PS2ITM01", 8) != 0 ||
            !f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size())))
        {
            ShowWarning("ps2: %s missing or invalid; items are not filtered for PS2 clients", path);
            return std::vector<uint8>{};
        }
        ShowInfo("ps2: %s loaded", path);
        return data;
    }();

    if (itemId == 0 || itemId == 0xFFFF || bits.empty())
    {
        return true;
    }
    return (bits[itemId >> 3] & (1u << (itemId & 7))) != 0;
}

auto mesNum(const uint16 zoneId, const uint16 mesNum) -> std::optional<uint16>
{
    const auto line = dialog(zoneId, mesNum & 0x7FFF);
    if (!line || *line > 0x7FFF)
    {
        return std::nullopt;
    }
    return static_cast<uint16>((mesNum & 0x8000) | *line);
}

auto isStatic(const uint32 uniqueNo) -> bool
{
    return (uniqueNo >> 24) == 0x01 && (uniqueNo & 0xFFF) < kFirstDynamicIndex;
}

auto uniqueNoToPS2(const uint32 uniqueNo) -> std::optional<uint32>
{
    return remapUniqueNo(uniqueNo, true);
}

auto uniqueNoFromPS2(const uint32 uniqueNo) -> std::optional<uint32>
{
    return remapUniqueNo(uniqueNo, false);
}

auto actIndexToPS2(const uint16 zoneId, const uint16 actIndex) -> std::optional<uint16>
{
    if (actIndex >= kFirstDynamicIndex)
    {
        return actIndex;
    }
    return ZoneMaps::lookup(entityMaps().zones, zoneId, actIndex);
}

auto actIndexFromPS2(const uint16 zoneId, const uint16 actIndex) -> std::optional<uint16>
{
    if (actIndex >= kFirstDynamicIndex)
    {
        return actIndex;
    }
    return ZoneMaps::lookup(entityMaps().reverse, zoneId, actIndex);
}

} // namespace ps2::ids
