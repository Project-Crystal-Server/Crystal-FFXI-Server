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

// Translators for the final console clients: PS2 patch 20160203_0 and Xbox 360 40160203_0 (one netcode).
//
// The 2016 client reads today's layout for almost every packet; the translators here are the ones where it
// does not, plus guards for values it uses as unchecked table indices. Which of the 2010 translators still
// apply is decided in console_profile.cpp. Layouts and evidence, per id: FFXI-PS2 docs/packets2016/<dir>_<id>.md
// (handlers decompiled from the unpacked ffxi_pol.pex, loaded at 0x280000). Offsets include the 4-byte header.

#include "console_2016.h"
#include "console_groups.h"
#include "console_ids.h"

#include <algorithm>
#include <cstring>

namespace console::v2016
{

namespace
{

constexpr uint8 kContainers = 10; // inventory .. safe 2: item records hold containers 0-9 (s2c_01C.md)
constexpr uint8 kSlots      = 81; // slots 0-80 per container

// The entity table (0x663600) and the per-entity flag array (0x665A20) have 0x900 slots (2010: 0x800); handlers
// test `0x900 < idx`, so 0x900 itself is one past the table.
constexpr uint16 kEntityCount = 0x900;

// Titles in the 2016 title DATs (PS2 54784, Xbox 55704): 0x45C (2010: 0x2E1, today's PC: 0x47E).
constexpr uint32 kTitleCount = 0x45C;

constexpr uint8 kTreasureSlots = 10;

auto itemSlotValid(const uint8 container, const uint8 slot) -> bool
{
    return container < kContainers && slot < kSlots;
}

// Bits of the 0x028 stream (LSB first from +5), read and written in place
auto getBits(const uint8* d, const size_t pos, const uint8 n) -> uint32
{
    uint32 v = 0;
    for (uint8 i = 0; i < n; ++i)
    {
        v |= static_cast<uint32>((d[(pos + i) >> 3] >> ((pos + i) & 7)) & 1) << i;
    }
    return v;
}

void setBits(uint8* d, const size_t pos, const uint8 n, const uint32 v)
{
    for (uint8 i = 0; i < n; ++i)
    {
        const auto bit = static_cast<uint8>(1u << ((pos + i) & 7));
        if ((v >> i) & 1)
        {
            d[(pos + i) >> 3] |= bit;
        }
        else
        {
            d[(pos + i) >> 3] &= static_cast<uint8>(~bit);
        }
    }
}

// 0x01E GP_SERV_ITEM_NUM / 0x01F GP_SERV_ITEM_LIST / 0x020 GP_SERV_ITEM_ATTR -- today's layout; the container
// and slot index the item records unchecked.
auto itemNum(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return itemSlotValid(packet.ref<uint8>(0x08), packet.ref<uint8>(0x09)) ? Result::Pass : Result::Drop;
}

auto itemList(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return itemSlotValid(packet.ref<uint8>(0x0A), packet.ref<uint8>(0x0B)) ? Result::Pass : Result::Drop;
}

auto itemAttr(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return itemSlotValid(packet.ref<uint8>(0x0E), packet.ref<uint8>(0x0F)) ? Result::Pass : Result::Drop;
}

// 0x028 GP_SERV_BATTLE2 -- today's bit widths. Only the caster and target ids change: static entities are
// renumbered per build (console_ids.h), and one the build does not have drops the action. result_sum > 8
// overflows the client's result[8].
auto battle2(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    const size_t size = packet.getSize();
    if (size <= 5)
    {
        return Result::Drop;
    }
    auto*        d    = static_cast<uint8*>(packet);
    const size_t end  = size * 8;
    size_t       pos  = 5 * 8;
    bool         diff = false;

    const auto need  = [&](const size_t n) { return pos + n <= end; };
    const auto remap = [&]() -> bool
    {
        if (!need(32))
        {
            return false;
        }
        const auto was = getBits(d, pos, 32);
        const auto now = ids::uniqueNoToClient(was);
        if (!now)
        {
            return false;
        }
        if (*now != was)
        {
            setBits(d, pos, 32, *now);
            diff = true;
        }
        pos += 32;
        return true;
    };

    if (!remap() || !need(6 + 4 + 4 + 32 + 32))
    {
        return Result::Drop;
    }
    const uint32 trgSum = getBits(d, pos, 6);
    pos += 6 + 4 + 4 + 32 + 32; // trg_sum, res_sum, cmd_no, cmd_arg, info

    for (uint32 t = 0; t < trgSum; ++t)
    {
        if (!remap() || !need(4))
        {
            return Result::Drop;
        }
        const uint32 resultSum = getBits(d, pos, 4);
        pos += 4;
        if (resultSum > 8)
        {
            return Result::Drop;
        }
        for (uint32 r = 0; r < resultSum; ++r)
        {
            // miss 3, kind 2, sub_kind 12, info 5, scale 5, value 17, message 10, bit 31
            if (!need(85 + 1))
            {
                return Result::Drop;
            }
            pos += 85;
            if (getBits(d, pos++, 1)) // proc: kind 6, info 4, value 17, message 10
            {
                if (!need(37))
                {
                    return Result::Drop;
                }
                pos += 37;
            }
            if (!need(1))
            {
                return Result::Drop;
            }
            if (getBits(d, pos++, 1)) // react: kind 6, info 4, value 14, message 10
            {
                if (!need(34))
                {
                    return Result::Drop;
                }
                pos += 34;
            }
        }
    }
    return diff ? Result::Rewritten : Result::Pass;
}

// 0x043 GP_SERV_TALKNUMNAME -- ActIndex (+0x08) indexes the entity table unless MesNum bit 15 is set.
auto talkNumName(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if ((packet.ref<uint16>(0x0A) & 0x8000) == 0 && packet.ref<uint16>(0x08) >= kEntityCount)
    {
        return Result::Drop;
    }
    return Result::Pass;
}

// 0x055 GP_SERV_SCENARIOITEM -- the client keeps 7 key-item tables (2010: 4).
auto scenarioItem(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return packet.ref<uint16>(0x84) < 7 ? Result::Pass : Result::Drop;
}

// 0x017 GP_SERV_CHAT_STD -- B 0x3C0500. Today's header (Kind, Attr, Data, sName[15]) but the message starts at
// 0x18, not 0x17. Attr bit 3 makes the client parse a hex prefix off the message: never set it. Kind 2 and the
// assist channels (0x22/0x23) are not formatted by this client.
auto chatStd(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    Rewrite    rw(packet);
    const auto kind = rw.in<uint8>(0x04);
    if (kind == 2 || kind > 0x23)
    {
        return Result::Drop;
    }

    const size_t oldSize = packet.getSize();
    const size_t avail   = oldSize > 0x17 ? std::min<size_t>(oldSize - 0x17, 0x96) : 0;
    const size_t mesLen  = strnlen(reinterpret_cast<const char*>(rw.src + 0x17), avail);

    rw.clear((0x18 + mesLen + 1 + 3) & ~size_t{ 3 });
    rw.move(0x04, 0x04, 0x13);                                 // Kind, Attr, Data, sName
    rw.out<uint8>(0x04, kind >= 0x22 ? uint8{ 0x01 } : kind); // assist -> shout
    rw.out<uint8>(0x05, rw.in<uint8>(0x05) & ~0x08);
    rw.move(0x17, 0x18, mesLen);
    return Result::Rewritten;
}

// 0x01C GP_SERV_ITEM_MAX -- B 0x3AF2B0. Ten containers: sizes u8[10] at 0x04, usable sizes u16[10] at 0x14
// (today's ItemNum2 is at 0x24). Containers 10+ (wardrobes 2-8, recycle bin) have no storage in this client.
auto itemMax(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    Rewrite rw(packet);
    rw.clear(0x28);
    for (uint8 c = 0; c < kContainers; ++c)
    {
        rw.out<uint8>(0x04 + c, std::min<uint8>(rw.in<uint8>(0x04 + c), kSlots));
        rw.out<uint16>(0x14 + c * 2, std::min<uint16>(rw.in<uint16>(0x24 + c * 2), kSlots));
    }
    return Result::Rewritten;
}

// 0x03C GP_SERV_SHOP_LIST -- B 0x3BE3B0 / A 0x50BA10. Today's 12-byte entries; ShopIndex (entry +6) indexes an
// 80-row table unchecked, and a packet without entries makes the client's count underflow.
auto shopList(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    const size_t size  = packet.getSize();
    const size_t count = size > 8 ? (size - 8) / 12 : 0;
    auto*        data  = static_cast<uint8*>(packet);
    size_t       kept  = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const uint8* e = data + 8 + i * 12;
        uint16       index;
        std::memcpy(&index, e + 6, 2);
        if (index >= 80)
        {
            continue;
        }
        if (kept != i)
        {
            std::memmove(data + 8 + kept * 12, e, 12);
        }
        ++kept;
    }
    if (kept == 0)
    {
        return Result::Drop;
    }
    if (kept == count)
    {
        return Result::Pass;
    }
    packet.setSize(8 + kept * 12);
    return Result::Rewritten;
}

// 0x050 GP_SERV_EQUIP_LIST -- B 0x3BF990. Today's layout, container included. EquipKind indexes a 16-entry
// table unchecked; a container or slot the client does not have shows the equip slot empty.
auto equipList(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x05) >= 16)
    {
        return Result::Drop;
    }
    if (packet.ref<uint8>(0x06) < kContainers && packet.ref<uint8>(0x04) < kSlots)
    {
        return Result::Pass;
    }
    packet.ref<uint8>(0x04) = 0;
    packet.ref<uint8>(0x06) = 0;
    return Result::Rewritten;
}

// 0x0C8 GP_SERV_GROUP_TBL -- B 0x3DA2C0. Today's 12-byte entries; Kind (5 folds to 0) picks a member table
// unchecked.
auto groupTbl(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    const auto kind = packet.ref<uint8>(0x04);
    return kind == 0 || kind == 5 ? Result::Pass : Result::Drop;
}

// 0x0C9 GP_SERV_EQUIP_INSPECT -- mode 1 (general) only: the 2010 rearrangement, but with the full 16-byte
// linkshell name at 0x14 and the levels at 0x24. Other modes are today's layout.
auto equipInspect(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x0A) != 1)
    {
        return Result::Pass;
    }

    Rewrite rw(packet);
    rw.clear(0x48);
    rw.move(0x04, 0x04, 0x08); // UniqNo, ActIndex, OptionFlag, +0x0B
    rw.move(0x0E, 0x0E, 2);    // ItemNo
    rw.move(0x20, 0x10, 2);    // sComColor
    rw.move(0x22, 0x12, 2);    // job[2]
    rw.move(0x10, 0x14, 16);   // sComLinkName
    rw.move(0x24, 0x24, 2);    // lvl[2]
    rw.move(0x2C, 0x28, 4);    // BallistaChevronCount
    rw.move(0x30, 0x2C, 1);    // BallistaChevronFlags
    rw.move(0x32, 0x2E, 2);    // BallistaFlags
    rw.move(0x34, 0x30, 4);    // MesNo
    rw.move(0x38, 0x34, 0x14); // Params[5]
    return Result::Rewritten;
}

// 0x0DD GP_SERV_GROUP_LIST / 0x0E2 GP_SERV_GROUP_LIST2 -- today's layout up to 0x25, but no master-level
// bytes: the name starts at 0x26, not 0x28. Kind picks one of 7 member tables unchecked.
auto groupList(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x1C) >= 7)
    {
        return Result::Drop;
    }
    const size_t size = packet.getSize();
    if (size <= 0x28)
    {
        return Result::Drop;
    }
    auto* data = static_cast<uint8*>(packet);
    std::memmove(data + 0x26, data + 0x28, size - 0x28);
    data[size - 2] = 0;
    data[size - 1] = 0;
    return Result::Rewritten;
}

// 0x0DF GP_SERV_GROUP_ATTR -- today's layout through 0x23; Kind picks a member table unchecked.
auto groupAttr(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return packet.ref<uint8>(0x18) < 7 ? Result::Pass : Result::Drop;
}

// 0x0E0 GP_SERV_GROUP_COMLINK -- today's layout; LinkshellNum indexes arrays that are written, unchecked.
auto groupComlink(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return packet.ref<uint8>(0x04) <= 2 ? Result::Pass : Result::Drop;
}

// 0x117 GP_SERV_EQUIPSET_RES -- today's layout; Count is not bounded by the client.
auto equipsetRes(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x04) <= 16)
    {
        return Result::Pass;
    }
    packet.ref<uint8>(0x04) = 16;
    return Result::Rewritten;
}

// 0x05B GP_SERV_WPOS / 0x065 GP_SERV_WPOS2 -- today's layout; ActIndex (+0x14) indexes the entity table.
auto wpos(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    return packet.ref<uint16>(0x14) < kEntityCount ? Result::Pass : Result::Drop;
}

// 0x0CA GP_SERV_INSPECT_MESSAGE -- DesignationNo (+0x90) looks up a title without a bound.
auto inspectMessage(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint32>(0x90) < kTitleCount)
    {
        return Result::Pass;
    }
    packet.ref<uint32>(0x90) = 0;
    return Result::Rewritten;
}

// 0x0D2 GP_SERV_TROPHY_LIST -- a 10-slot pool indexed by +0x14, and the dropping entity (+0x12) indexes the
// per-entity flag array.
auto trophyList(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x14) >= kTreasureSlots)
    {
        return Result::Drop;
    }
    if (packet.ref<uint16>(0x12) < kEntityCount)
    {
        return Result::Pass;
    }
    packet.ref<uint16>(0x12) = 0;
    packet.ref<uint8>(0x38)  = 0;
    return Result::Rewritten;
}

auto drop(MapSession* /* PSession */, CBasicPacket& /* packet */) -> Result
{
    return Result::Drop;
}

// c2s 0x0E2 GP_CLI_COMMAND_SET_LSMSG -- today's layout, but the builder leaves byte 0x05 (level bits) stale
// unless it is a level change (0x04 bit 5), and a stale writeLevel fails the server's validation.
auto lsMessage(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x04) & 0x20)
    {
        return Result::Pass;
    }
    packet.ref<uint8>(0x05) = 0;
    return Result::Rewritten;
}

// c2s 0x0FA GP_CLI_COMMAND_MYROOM_LAYOUT -- today's 0x10 layout, but the "leave layout" builder (ItemNo 0) never
// writes FloorFlg (0x08), and a stale value fails the server's range check.
auto myroomLayout(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.getSize() != 0x10)
    {
        return Result::Drop;
    }
    if (packet.ref<uint16>(0x04) != 0)
    {
        return Result::Pass;
    }
    packet.ref<uint8>(0x08) = 0;
    return Result::Rewritten;
}

// c2s 0x11C GP_CLI_COMMAND_PARTY_REQUEST -- today's fields, but 0xC bytes where today's struct is 0x10.
auto partyRequest(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    if (packet.getSize() >= 0x10)
    {
        return Result::Pass;
    }
    Rewrite rw(packet);
    rw.clear(0x10);
    rw.move(0x04, 0x04, 0x07); // UniqueNo, ActIndex, Kind
    return Result::Rewritten;
}

} // namespace

void registerTranslators(compat::Profile& p)
{
    p.s2c(0x017, chatStd);
    p.s2c(0x01C, itemMax);
    p.s2c(0x01E, itemNum);
    p.s2c(0x01F, itemList);
    p.s2c(0x020, itemAttr);
    p.s2c(0x028, battle2);
    p.s2c(0x03C, shopList);
    p.s2c(0x043, talkNumName);
    p.s2c(0x050, equipList);
    p.s2c(0x055, scenarioItem);
    p.s2c(0x05B, wpos);
    p.s2c(0x065, wpos);
    p.s2c(0x072, drop); // BATTLEFIELD reply: LSB never sends it, layout unknown
    p.s2c(0x0C8, groupTbl);
    p.s2c(0x0C9, equipInspect);
    p.s2c(0x0CA, inspectMessage);
    p.s2c(0x0D2, trophyList);
    p.s2c(0x0DD, groupList);
    p.s2c(0x0DF, groupAttr);
    p.s2c(0x0E0, groupComlink);
    p.s2c(0x0E2, groupList);
    p.s2c(0x117, equipsetRes);
    p.s2c(0x11B, drop); // not a server packet LSB has

    p.c2s(0x0E2, lsMessage);
    p.c2s(0x0FA, myroomLayout);
    p.c2s(0x11C, partyRequest);
}

} // namespace console::v2016
