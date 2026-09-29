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

#include "ps2_client.h"
#include "ps2_entities.h"
#include "ps2_groups.h"

#include "map_session.h"
#include "packets/basic.h"

#include <array>
#include <cstring>

namespace ps2
{

namespace
{

// Every id gcZoneRecvCallBack / gcZoneRecvCallBack2 registers in the 2010 client (FFXI-PS2 tools/handlers.md).
constexpr uint16 kHandled[] = {
    0x005, 0x006, 0x008, 0x009, 0x00A, 0x00B, 0x00D, 0x00E, 0x011, 0x012, 0x013, 0x014, 0x016, 0x017, 0x01B, 0x01C,
    0x01D, 0x01E, 0x01F, 0x020, 0x021, 0x022, 0x023, 0x024, 0x025, 0x026, 0x027, 0x028, 0x029, 0x02A, 0x02B, 0x02C,
    0x02D, 0x02E, 0x02F, 0x030, 0x031, 0x032, 0x033, 0x034, 0x036, 0x037, 0x038, 0x039, 0x03A, 0x03B, 0x03C, 0x03D,
    0x03E, 0x03F, 0x041, 0x042, 0x043, 0x044, 0x047, 0x04B, 0x04C, 0x04D, 0x04F, 0x050, 0x051, 0x052, 0x053, 0x054,
    0x055, 0x056, 0x057, 0x058, 0x059, 0x05A, 0x05B, 0x05C, 0x05D, 0x05E, 0x05F, 0x060, 0x061, 0x062, 0x063, 0x064,
    0x065, 0x067, 0x069, 0x06F, 0x070, 0x071, 0x073, 0x074, 0x078, 0x079, 0x081, 0x082, 0x083, 0x084, 0x085, 0x086,
    0x08C, 0x096, 0x097, 0x098, 0x099, 0x09A, 0x09B, 0x09C, 0x09D, 0x09E, 0x0A0, 0x0AA, 0x0AB, 0x0AC, 0x0AD, 0x0B4,
    0x0B5, 0x0B6, 0x0B7, 0x0BF, 0x0C8, 0x0C9, 0x0CA, 0x0CC, 0x0D2, 0x0D3, 0x0DC, 0x0DD, 0x0DE, 0x0DF, 0x0E0, 0x0E1,
    0x0E2, 0x0E6, 0x0F4, 0x0F5, 0x0F6, 0x0F9, 0x0FA, 0x105, 0x106, 0x107, 0x108, 0x109, 0x10A, 0x10E, 0x10F,
};

constexpr uint16 kIdCount = 0x200;

struct Tables
{
    std::array<Translator, kIdCount> s2c{};
    std::array<Translator, kIdCount> c2s{};
    std::array<bool, kIdCount>       handled{};

    Tables()
    {
        for (const auto id : kHandled)
        {
            handled[id] = true;
        }
    }
};

auto tables() -> Tables&
{
    static Tables t;
    return t;
}

auto registered() -> Tables&
{
    static const bool once = []
    {
        registerGroups();
        return true;
    }();
    std::ignore = once;
    return tables();
}

auto combine(const Result a, const Result b) -> Result
{
    if (a == Result::Drop || b == Result::Drop)
    {
        return Result::Drop;
    }
    return (a == Result::Rewritten || b == Result::Rewritten) ? Result::Rewritten : Result::Pass;
}

} // namespace

auto isPS2Platform(const uint8* sPlatform) -> bool
{
    return std::memcmp(sPlatform, "PS2", 3) == 0 && (sPlatform[3] == 0 || sPlatform[3] == ' ');
}

auto isPS2(const MapSession* PSession) -> bool
{
    return PSession && PSession->isPS2Client;
}

void registerS2C(const uint16 id, const Translator fn)
{
    tables().s2c[id & 0x1FF] = fn;
}

void registerC2S(const uint16 id, const Translator fn)
{
    tables().c2s[id & 0x1FF] = fn;
}

auto clientHandles(const uint16 id) -> bool
{
    return id < kIdCount && tables().handled[id];
}

auto translateS2C(MapSession* PSession, CBasicPacket& packet) -> Result
{
    auto&      t  = registered();
    const auto id = packet.getType();

    // A translator may re-id a packet the 2010 client knows under another id (0x068 pet sync is
    // its 0x067 sub-type 4), so it runs before the unhandled-id drop, and the result is checked again.
    auto result = Result::Pass;

    // A translator may re-id a packet the 2010 client knows under another id (0x068 pet sync is
    // its 0x067 sub-type 4), so it runs before the unhandled-id drop, which is checked on the result.
    if (const auto fn = t.s2c[id])
    {
        result = fn(PSession, packet);
    }

    if (result == Result::Drop || !t.handled[packet.getType()])
    {
        return Result::Drop;
    }

    return combine(result, remapEntitiesS2C(PSession, packet));
}

auto translateC2S(MapSession* PSession, CBasicPacket& packet) -> Result
{
    auto& t      = registered();
    auto  result = Result::Pass;

    if (const auto fn = t.c2s[packet.getType()])
    {
        result = fn(PSession, packet);
    }

    if (result == Result::Drop)
    {
        return Result::Drop;
    }

    return combine(result, remapEntitiesC2S(PSession, packet));
}

} // namespace ps2
