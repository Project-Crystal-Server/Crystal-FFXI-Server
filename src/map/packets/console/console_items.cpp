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

#include "console_items.h"
#include "console_groups.h"
#include "console_ids.h"

#include <algorithm>
#include <cstring>

// Items the 2010 install does not have. Its item DATs keep a "." record for every id they did not
// have yet, so the client would show those as "." (and items past its DATs' ranges as nothing).
// Offsets are the 2010 layouts, i.e. after the s2c translators (FFXI-PS2 docs/packets/*.md):
//   * a packet that fills a slot (inventory, trade) sends the slot empty instead, so no stale item is
//     left in it;
//   * a list (shop, guild shop, inspected equipment) loses the unknown entries;
//   * a packet about one item (treasure pool, bazaar, delivery box, guild item, /translate,
//     synthesis result) is dropped.
// Item ids inside messages (0x009/0x029/0x02A/0x053 parameters) are not filtered: the message still
// shows, with the item's name from the 2010 DAT.

namespace console
{

namespace
{

auto known(const CBasicPacket& packet, const size_t offset) -> bool
{
    return ids::hasItem(packet.ref<uint16>(offset));
}

// Slot packets: 0x01F ITEM_LIST (+4 count, +8 ItemNo), 0x020 ITEM_ATTR (+4 count, +8 price,
// +0x0C ItemNo, +0x11 exdata[0x18]), 0x023 ITEM_TRADE_LIST (+4 count, +0x0A ItemNo),
// 0x025 ITEM_TRADE_MYLIST (+4 count, +8 ItemNo).
auto emptySlot(CBasicPacket& packet, const size_t itemNoOffset) -> Result
{
    if (known(packet, itemNoOffset))
    {
        return Result::Pass;
    }
    packet.ref<uint32>(0x04)          = 0;
    packet.ref<uint16>(itemNoOffset) = 0;
    if (packet.getType() == 0x020)
    {
        packet.ref<uint32>(0x08) = 0;
        std::memset(static_cast<uint8*>(packet) + 0x11, 0, 0x18);
    }
    return Result::Rewritten;
}

// Remove unknown entries from a list of `count` entries of `stride` bytes at `first`, ItemNo at +0
// of the entry unless `itemNoAt` says otherwise. Returns the number kept.
auto compact(CBasicPacket& packet, const size_t first, const size_t stride, const size_t count, const size_t itemNoAt) -> size_t
{
    auto*  data = static_cast<uint8*>(packet);
    size_t kept = 0;
    for (size_t i = 0; i < count && first + (i + 1) * stride <= PACKET_SIZE; ++i)
    {
        uint8* entry = data + first + i * stride;
        if (!ids::hasItem(*reinterpret_cast<uint16*>(entry + itemNoAt)))
        {
            continue;
        }
        if (kept != i)
        {
            std::memmove(data + first + kept * stride, entry, stride);
        }
        ++kept;
    }
    if (kept < count)
    {
        const size_t end = std::min<size_t>(first + count * stride, PACKET_SIZE);
        std::memset(data + first + kept * stride, 0, end - (first + kept * stride));
    }
    return kept;
}

} // namespace

auto filterItemsS2C(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    switch (packet.getType())
    {
        case 0x01F:
            return emptySlot(packet, 0x08);
        case 0x020:
            return emptySlot(packet, 0x0C);
        case 0x023:
            return emptySlot(packet, 0x0A);
        case 0x025:
            return emptySlot(packet, 0x08);

        case 0x03C: // SHOP_LIST: 8-byte entries from +8 {u32 price, u16 ItemNo, u8 ShopIndex, u8}
        {
            const size_t size  = packet.getSize();
            const size_t count = size > 8 ? (size - 8) / 8 : 0;
            const size_t kept  = compact(packet, 0x08, 8, count, 4);
            if (kept == count)
            {
                return Result::Pass;
            }
            if (kept == 0)
            {
                return Result::Drop; // an empty 0x03C underflows the client's count (s2c_1c.md)
            }
            packet.setSize(0x08 + kept * 8);
            return Result::Rewritten;
        }

        case 0x083: // GUILD_BUYLIST / GUILD_SELLLIST: count u8 +0xF4, 8-byte entries from +4
        case 0x085:
        {
            const size_t count = packet.ref<uint8>(0xF4);
            const size_t kept  = compact(packet, 0x04, 8, count, 0);
            if (kept == count)
            {
                return Result::Pass;
            }
            packet.ref<uint8>(0xF4) = static_cast<uint8>(kept);
            return Result::Rewritten;
        }

        case 0x0C9: // EQUIP_INSPECT mode 3: count u8 +0x0B, 0x1C-byte entries from +0x0C
        {
            if (packet.ref<uint8>(0x0A) != 3)
            {
                return Result::Pass;
            }
            const size_t count = packet.ref<uint8>(0x0B);
            const size_t kept  = compact(packet, 0x0C, 0x1C, count, 0);
            if (kept == count)
            {
                return Result::Pass;
            }
            packet.ref<uint8>(0x0B) = static_cast<uint8>(kept);
            return Result::Rewritten;
        }

        case 0x06F: // COMBINE_ANS / COMBINE_INF: result +8, lost items u16[8] +0x0A
        case 0x070:
        {
            if (!known(packet, 0x08))
            {
                return Result::Drop;
            }
            bool changed = false;
            for (size_t i = 0; i < 8; ++i)
            {
                if (!known(packet, 0x0A + i * 2))
                {
                    packet.ref<uint16>(0x0A + i * 2) = 0;
                    changed                          = true;
                }
            }
            return changed ? Result::Rewritten : Result::Pass;
        }

        case 0x05A: // MOTIONMES: weapon for /hurray and /aim at +0x12 (nation for /salute: always < 4)
            if (!known(packet, 0x12))
            {
                packet.ref<uint16>(0x12) = 0;
                return Result::Rewritten;
            }
            return Result::Pass;

        case 0x047: // /translate item
        case 0x082: // GUILD_BUY
        case 0x084: // GUILD_SELL
            return known(packet, 0x04) ? Result::Pass : Result::Drop;
        case 0x0D2: // TROPHY_LIST
            return known(packet, 0x10) ? Result::Pass : Result::Drop;
        case 0x105: // BAZAAR_LIST
            return known(packet, 0x0E) ? Result::Pass : Result::Drop;
        case 0x10A: // BAZAAR_SALE
            return known(packet, 0x08) ? Result::Pass : Result::Drop;
        case 0x04B: // PBX_RESULT, full form (0x58): box slot ItemNo +0x30
            return packet.getSize() < 0x58 || known(packet, 0x30) ? Result::Pass : Result::Drop;

        default:
            return Result::Pass;
    }
}

} // namespace console
