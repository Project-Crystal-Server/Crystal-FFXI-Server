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

// Server->client packets 0x080-0x10F for the 2010 PS2 client.
//
// Everything not registered here was checked against the client's handler(s) and is either identical
// (passes as is) or has no handler (dropped by translateS2C: 0x08D, 0x08E, 0x0AE). See FFXI-PS2
// docs/packets/s2c_80.md for the per-id layouts. Offsets below include the 4-byte header, as in the
// decompiled handlers. Where an id has two handlers, "B" runs first, then "A".
//
// Id remapping (items, spells, abilities, merits, zones, titles...) is not done here: see console_ids.h and
// the "Ids to remap" section of the doc.

#include "console_groups.h"

#include "map_session.h"

#include <optional>

namespace console
{

namespace
{

// The client's entity table (0x5DB630) and its per-entity flag array (0x5DD650) have 0x800 slots.
constexpr uint16 kEntityCount = 0x800;

// Treasure pool: zone+0xB7A4 + slot*0x58, 10 slots (the next field is zone+0xBB14).
constexpr uint8 kTreasureSlots = 10;

// Bazaar list: zone+0x29A04 + slot*0x30, cleared as 0xF30 bytes by RecvClose = 81 slots (0..80).
constexpr uint8 kBazaarSlots = 81;

// Guild shop list packets carry at most 30 entries of 8 bytes (+0x04..+0xF3).
constexpr uint8 kGuildListEntries = 30;

// Title message table (d_msg, 0x100-byte entries) the inspect handler indexes without a bound: its
// header in the title-screen RAM dump says 0x2E1 entries.
constexpr uint32 kTitleCount = 0x2E1;

// The 2010 group handlers pick their table as zone+0x4948 + ((Kind == 2) ? 0 : Kind) * 0x5F4 with no
// bound: 0 party, 1 linkshell, 2 alliance. Today's PartyKind is Party = 0, Alliance = 5.
auto groupKind(const uint8 kind) -> std::optional<uint8>
{
    switch (kind)
    {
        case 0:
        case 1:
        case 2:
            return kind;
        case 5:
            return static_cast<uint8>(2);
        default:
            return std::nullopt;
    }
}

// Group packets carry the member's zone as one byte in 2010 (all 2010 zone ids are below 0x100).
auto zoneByte(const uint16 zone) -> uint8
{
    return zone <= 0xFF ? static_cast<uint8>(zone) : 0;
}

// 0x083 GUILD_BUYLIST / 0x085 GUILD_SELLLIST: B handlers 0x4B88B0 / 0x4B8A10.
// Count (+0xF4) entries of 8 bytes from +0x04 are appended by FUN_004B87F0; the count is not checked
// against the 30 entries a packet holds. Same layout as today; clamp the count.
auto guildList(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0xF4) > kGuildListEntries)
    {
        packet.ref<uint8>(0xF4) = kGuildListEntries;
        return Result::Rewritten;
    }

    return Result::Pass;
}

// 0x08C MERIT: B handler RecvMerPtConf 0x4510E0. Count u16 (+0x04), then Count entries of 4 bytes
// from +0x08 ({u16 index (bit 0 = delete), u8 next, u8 count}), inserted into a heap-grown sorted list.
// Same layout as today. The handler trusts Count; keep it inside the packet.
auto merit(MapSession*, CBasicPacket& packet) -> Result
{
    const auto size = packet.getSize();
    const auto max  = size > 8 ? static_cast<uint16>((size - 8) / 4) : static_cast<uint16>(0);

    if (packet.ref<uint16>(0x04) > max)
    {
        packet.ref<uint16>(0x04) = max;
        return Result::Rewritten;
    }

    return Result::Pass;
}

// 0x0AA MAGIC_DATA: B handler RecvMagicData 0x44A5B0 copies 0x60 bytes from +0x04 (spells 0..767).
// Today's table is 0x80 bytes; the tail is spells the 2010 client does not have.
auto magicData(MapSession*, CBasicPacket& packet) -> Result
{
    packet.setSize(0x04 + 0x60);
    return Result::Rewritten;
}

// 0x0AC COMMAND_DATA: B handler RecvCommandData 0x44A410 copies 0x80 bytes from +0x04 into
// zone+0x1D8B4: ONE bitfield of 0x400 indices into the client's ability file (file id 85, 1024
// records of 0x400 bytes; byte-identical in the 2010 PS2 install and today's PC install). Decoded,
// record n is:
//   0x000-0x1FF  menu commands 1-15 (Ranged, Weapon Abilities, Fish...), then job abilities by LSB
//                ability id (Mighty Strikes 16, Provoke 35, Fight 69, Deploy 138)
//   0x200-0x2FF  pet abilities, LSB ability id (Healing Ruby 512, Poison Nails 513)
//   0x300-0x3FF  weapon skills, 0x300 + LSB weapon skill id (Combo 769, Starlight 931)
// Today: WeaponSkills[64] +0x04 (bit = ws id), JobAbilities[64] +0x44 (bit = ability id),
// PetAbilities[64] +0x84 (bit = ability id - 512), Traits[32] +0xC4 (bit = trait id).
// So the 2010 table is JobAbilities[0..63] ++ PetAbilities[0..31] ++ WeaponSkills[0..31]; pet
// ability bits 256+ and weapon skill bits 256+ have no place in it.
//
// The traits go to the 2010 client's own 0x0AB FEAT_DATA (RecvFeatData 0x44A3C0: 0x14 bytes from +0x04
// into zone+0x1D8A0, right before the command table), bit = trait id as today; LSB keeps 18 bytes.
auto commandData(MapSession*, CBasicPacket& packet) -> Result
{
    Rewrite rw(packet);

    CBasicPacket feat;
    feat.setType(0x0AB);
    feat.setSize(0x04 + 0x14);
    std::memcpy(static_cast<uint8*>(feat) + 0x04, rw.src + 0xC4, 0x14);
    emitS2C(feat, feat.getSize());

    rw.clear(0x04 + 0x80);
    rw.move(0x44, 0x04, 0x40); // job abilities: bits 0x000-0x1FF
    rw.move(0x84, 0x44, 0x20); // pet abilities 512..767: bits 0x200-0x2FF
    rw.move(0x04, 0x64, 0x20); // weapon skills 0..255: bits 0x300-0x3FF
    return Result::Rewritten;
}

// 0x0B4 CONFIG: B handler RecvConf 0x451560 copies SAVE_CONF (+0x04, 12 bytes), then reads u16 +0x10
// and u8 +0x12. A handler Recv_config 0x366AB0 reads +0x05, +0x08, +0x0C. Today +0x10 is one byte
// (GmLevel, sent as 0) followed by PartyLanguages (+0x11) and padding, which the 2010 client would read
// as the high byte of +0x10 and as +0x12. Zero them.
auto config(MapSession*, CBasicPacket& packet) -> Result
{
    packet.ref<uint8>(0x11) = 0;
    packet.ref<uint8>(0x12) = 0;
    packet.ref<uint8>(0x13) = 0;
    return Result::Rewritten;
}

// 0x0C8 GROUP_TBL: B handler RecvGroupTbl 0x3B1830. Kind u8 +0x04 (table index, see groupKind), then
// 20 entries of 8 bytes from +0x08: {u32 UniqueNo, u16 ActIndex, u8 flags, u8 ZoneNo}; the handler
// always reads all 20 (0xA8 bytes). Today's entries are 12 bytes with a u16 ZoneNo at +8.
auto groupTbl(MapSession*, CBasicPacket& packet) -> Result
{
    const auto kind = groupKind(packet.ref<uint8>(0x04));
    if (!kind)
    {
        return Result::Drop;
    }

    Rewrite rw(packet);
    rw.clear(0xA8);
    rw.out<uint8>(0x04, *kind);

    for (size_t i = 0; i < 20; ++i)
    {
        const size_t from = 0x08 + i * 12;
        const size_t to   = 0x08 + i * 8;
        rw.move(from, to, 7); // UniqueNo, ActIndex, flags
        rw.out<uint8>(to + 7, zoneByte(rw.in<uint16>(from + 8)));
    }

    return Result::Rewritten;
}

// 0x0C9 EQUIP_INSPECT: B handler RecvEquipInspect 0x39C540, switched on OptionFlag (+0x0A).
// Mode 3 (EQUIPMENT: count +0x0B, 0x1C-byte entries from +0x0C) and modes 0/2 match today's layout.
// Mode 1 (GENERAL) is laid out differently in 2010:
//   +0x0E u16 ItemNo, +0x10 u16 sComColor (4 nibbles), +0x12 u8 job[2], +0x14 sComLinkName (15),
//   +0x23 u8 lvl[2], +0x28 u32 chevrons, +0x2C u8 flags (bit 1: print MesNo), +0x2E u16,
//   +0x30 u32 MesNo, +0x34 s32 Params[5].
// Today: +0x0E ItemNo, +0x10 sComLinkName[16], +0x20 sComColor, +0x22 job[2], +0x24 lvl[2], +0x26 mjob,
//   +0x27 mlvl, +0x28 mflags, +0x2C chevrons, +0x30 flags, +0x32 u16, +0x34 MesNo, +0x38 Params[5].
auto equipInspect(MapSession*, CBasicPacket& packet) -> Result
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
    rw.move(0x10, 0x14, 15);   // sComLinkName (15 packed bytes)
    rw.move(0x24, 0x23, 2);    // lvl[2]
    rw.move(0x2C, 0x28, 4);    // BallistaChevronCount
    rw.move(0x30, 0x2C, 1);    // BallistaChevronFlags
    rw.move(0x32, 0x2E, 2);    // BallistaFlags
    rw.move(0x34, 0x30, 4);    // MesNo
    rw.move(0x38, 0x34, 0x14); // Params[5]
    return Result::Rewritten;
}

// 0x0CA INSPECT_MESSAGE: A handler 0x3AE4E0, B handler RecvInspectMessage 0x4BCB70. Same layout as
// today. B uses DesignationNo (+0x90) as an unchecked index into the title message table
// (FUN_00506590: header + 0x40 + 0x100 * n).
auto inspectMessage(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint32>(0x90) >= kTitleCount)
    {
        packet.ref<uint32>(0x90) = 0;
        return Result::Rewritten;
    }

    return Result::Pass;
}

// 0x0CC LINKSHELL_MESSAGE: B handler RecvComlinkMessage 0x3B21E0. Same layout as today. The 2010 client
// has one linkshell; today's linkshell_index (+0x05 bits 6-7) = 1 is Linkshell 2, which it would show as
// its only linkshell.
auto linkshellMessage(MapSession*, CBasicPacket& packet) -> Result
{
    if (((packet.ref<uint8>(0x05) >> 6) & 3) != 0)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x0D2 TROPHY_LIST: B handler RecvTrophyList 0x38CEF0. Same layout as today. TrophyItemIndex (+0x14)
// indexes the 10-slot pool unchecked; TargetActIndex (+0x12) indexes the 0x800-entry flag array at
// 0x5DD650 unchecked (FUN_002E3900 & co.).
auto trophyList(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x14) >= kTreasureSlots)
    {
        return Result::Drop;
    }

    if (packet.ref<uint16>(0x12) >= kEntityCount)
    {
        packet.ref<uint16>(0x12) = 0;
        packet.ref<uint8>(0x38)  = 0;
        return Result::Rewritten;
    }

    return Result::Pass;
}

// 0x0D3 TROPHY_SOLUTION: B handler RecvTrophySolution 0x38D5E0. Same layout as today;
// TrophyItemIndex (+0x14) indexes the pool (and each member's +0x38 + slot*2) unchecked.
auto trophySolution(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x14) >= kTreasureSlots)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x0DC GROUP_SOLICIT_REQ: B handler 0x3B13D0 (stores UniqueNo +0x04, ActIndex +0x08, Kind +0x0B),
// A handler 0x4DB250 (memcpy 16 bytes of sName +0x0C, then prints it with %s; Kind 0 party, 2 alliance).
// Same layout as today; map Kind and keep sName terminated.
auto groupSolicitReq(MapSession*, CBasicPacket& packet) -> Result
{
    const auto kind = groupKind(packet.ref<uint8>(0x0B));
    if (!kind)
    {
        return Result::Drop;
    }

    packet.ref<uint8>(0x0B) = *kind;
    packet.ref<uint8>(0x1B) = 0;
    return Result::Rewritten;
}

// 0x0DD GROUP_LIST: B handler RecvGroupList 0x3B1C40. 0x0E2 GROUP_LIST2: B handler 0x3B1E50.
// 2010: +0x04 UniqueNo, +0x08 Hp, +0x0C Mp, +0x10 Tp, +0x14 GAttr, +0x18 ActIndex, +0x1A MemberNumber,
// +0x1B MoghouseFlg, +0x1C Kind, +0x1D Hpp, +0x1E Mpp, +0x1F u8 ZoneNo, +0x20 Name; the name is
// Size - 0x24 bytes (so the packet keeps its 4 trailing bytes). Today +0x1F is padding, ZoneNo is a u16
// at +0x20, jobs at +0x22, Name at +0x28: shift the name down 8 bytes.
auto groupList(MapSession*, CBasicPacket& packet) -> Result
{
    const auto kind = groupKind(packet.ref<uint8>(0x1C));
    const auto size = packet.getSize();
    if (!kind || size <= 0x2C) // no name: the client drops it anyway
    {
        return Result::Drop;
    }

    Rewrite rw(packet);
    rw.clear(size - 8);
    rw.move(0x04, 0x04, 0x1B); // UniqueNo .. Mpp
    rw.out<uint8>(0x1C, *kind);
    rw.out<uint32>(0x10, tpPercent(rw.in<uint32>(0x10)));
    rw.out<uint8>(0x1F, zoneByte(rw.in<uint16>(0x20)));
    rw.move(0x28, 0x20, 16);
    return Result::Rewritten;
}

// 0x0DF GROUP_ATTR: B handler RecvGroupAttr 0x3B1440. 2010 reads +0x04..+0x1B: UniqueNo, Hp, Mp, Tp,
// ActIndex +0x14, Hpp +0x16, Mpp +0x17, Kind +0x18, MoghouseFlg +0x19, u16 ZoneNo +0x1A: today's layout
// up to there. The rest (Monstrosity, jobs) is new; map Kind and cut it.
auto groupAttr(MapSession*, CBasicPacket& packet) -> Result
{
    const auto kind = groupKind(packet.ref<uint8>(0x18));
    if (!kind)
    {
        return Result::Drop;
    }

    packet.ref<uint8>(0x18) = *kind;
    packet.ref<uint32>(0x10) = tpPercent(packet.ref<uint32>(0x10));
    packet.setSize(0x1C);
    return Result::Rewritten;
}

// 0x0E0 GROUP_COMLINK: B handler RecvComlink 0x3B2130 reads only +0x04 (non-zero: linkshell equipped).
// The 2010 client has one linkshell; today's LinkshellNum 2 is Linkshell 2.
auto groupComlink(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x04) == 2)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x0FA MYROOM_OPERATION: A handler Recv_myroom_operation 0x3C41E0 returns 1 and reads nothing.
auto myroomOperation(MapSession*, CBasicPacket&) -> Result
{
    return Result::Drop;
}

// 0x105 BAZAAR_LIST: B handler RecvList_B105 0x4BA260. Same layout as today; ItemIndex (+0x10) indexes
// the 81-slot list unchecked.
auto bazaarList(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x10) >= kBazaarSlots)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x109 BAZAAR_SELL: B handler RecvSell 0x4BA370 (A 0x4B50D0 is empty). Same layout as today;
// ItemIndex (+0x20) indexes the 81-slot list unchecked.
auto bazaarSell(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x20) >= kBazaarSlots)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

} // namespace

void registerS2C_80(compat::Profile& p)
{
    p.s2c(0x083, guildList);
    p.s2c(0x085, guildList);
    p.s2c(0x08C, merit);
    p.s2c(0x0AA, magicData);
    p.s2c(0x0AC, commandData);
    p.s2c(0x0B4, config);
    p.s2c(0x0C8, groupTbl);
    p.s2c(0x0C9, equipInspect);
    p.s2c(0x0CA, inspectMessage);
    p.s2c(0x0CC, linkshellMessage);
    p.s2c(0x0D2, trophyList);
    p.s2c(0x0D3, trophySolution);
    p.s2c(0x0DC, groupSolicitReq);
    p.s2c(0x0DD, groupList);
    p.s2c(0x0DF, groupAttr);
    p.s2c(0x0E0, groupComlink);
    p.s2c(0x0E2, groupList);
    p.s2c(0x0FA, myroomOperation);
    p.s2c(0x105, bazaarList);
    p.s2c(0x109, bazaarSell);
}

} // namespace console
