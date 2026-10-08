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

// Server->client packets 0x040-0x07F for the 2010 PS2 client.
//
// Everything not registered here was checked against the client's handler and is either identical
// (passes as is) or has no handler (dropped by translateS2C). See FFXI-PS2 docs/packets/s2c_40.md for
// the per-id layouts. Offsets below include the 4-byte header, as in the decompiled handlers.

#include "console_groups.h"

#include "map_session.h"

namespace console
{

namespace
{

// The client's entity table (0x5DB630) has 0x800 slots. Several handlers accept index 0x800 (they
// test `0x800 < idx`) or do not check at all, so anything >= 0x800 is dropped before it gets there.
constexpr uint16 kEntityCount = 0x800;

// The 2010 client knows jobs 1..20 (WAR..SCH). GEO/RUN/MON came later.
constexpr uint8 kMaxJob = 20;

// 0x044 EXTENDED_JOB (BLU, PUP, MON): handler Recv_extended_job_pup 0x2D3FC0.
// Copies 0x98 bytes from +0x04 into zone+0x2EF80 + IsSubJob*0x98 when Job (+0x04) matches the current
// main/sub job (FUN_003C4760) and IsSubJob (+0x05) <= 1. BLU and PUP line up; MON (job 23) cannot exist
// on this client.
auto s2c044ExtendedJob(MapSession*, CBasicPacket& packet) -> Result
{
    const auto job = packet.ref<uint8>(0x04);
    if (job == 0 || job > kMaxJob || packet.ref<uint8>(0x05) > 1)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x043 TALKNUMNAME: handler Recv_talknumname 0x301150. Unless MesNum (+0x0A) has bit 0x8000 set,
// ActIndex (+0x08) indexes the entity table after a `0x800 < idx` test, so 0x800 reads one past it.
auto s2c043TalkNumName(MapSession*, CBasicPacket& packet) -> Result
{
    if ((packet.ref<uint16>(0x0A) & 0x8000) == 0 && packet.ref<uint16>(0x08) >= kEntityCount)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x04B PBX_RESULT: handler RecvReqPostReplyCommon 0x304900. For Command (+0x04) 3/4/6/7/8/9 with
// Result (+0x0C) 1 or 2 the sub-handlers (0x3E4990, 0x3E4AF0, 0x3E4DA0, 0x3E4F00, 0x3E5100, 0x3E5260)
// write the box slot zone+0x1D2A4/0x1D4F0 + PostWorkNo*0x48 without checking PostWorkNo (+0x06).
// The boxes have 8 slots; commands 1/2/10/11 check the range themselves.
auto s2c04BPbxResult(MapSession*, CBasicPacket& packet) -> Result
{
    const auto command = packet.ref<uint8>(0x04);
    const auto result  = packet.ref<int8>(0x0C);
    const auto slot    = packet.ref<int8>(0x06);

    const bool unchecked = command == 3 || command == 4 || command == 6 || command == 7 || command == 8 || command == 9;
    if (unchecked && (result == 1 || result == 2) && (slot < 0 || slot >= 8))
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x050 EQUIP_LIST: handler RecvEquipList 0x39C410.
//   zone[0xBCB8 + EquipKind(+0x05)*8] = PropertyItemIndex(+0x04)
// No container: every equipped item is assumed to be in the inventory (container 0), and EquipKind is
// not checked against the 16-slot table.
// An item equipped from anywhere else (a wardrobe) cannot be shown: the index would point at whatever
// sits in that inventory slot. Send it as an empty slot instead (index 0 is the gil slot, which the
// client treats as "nothing equipped"). The server still applies the item.
auto s2c050EquipList(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint8>(0x05) >= 16)
    {
        return Result::Drop;
    }

    if (packet.ref<uint8>(0x06) != 0)
    {
        packet.ref<uint8>(0x04) = 0;
        packet.ref<uint8>(0x06) = 0;
        return Result::Rewritten;
    }

    return Result::Pass;
}

// 0x051 GRAP_LIST: handler RecvGrapList 0x2FBBA0 -> FUN_002F8A60(own index, +0x04). Same 9 x u16 table,
// race taken from the high byte of GrapIDTbl[0]. The client has no costume form: LSB signals a costume /
// monstrosity model by putting the model id in GrapIDTbl[0] and 0xFFFF in GrapIDTbl[8], which this
// client would read as race = model>>8, face = model&0xFF. Drop it (the look stays as it was).
auto s2c051GrapList(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint16>(0x14) == 0xFFFF)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x055 SCENARIOITEM: handler RecvScenarioItem 0x2F5C30. Same layout (TableIndex at +0x84 is read),
// but the client keeps only 4 key item tables and ignores index >= 4 (while still firing its refresh
// callback). Tables 4+ are post-2010 key items.
auto s2c055ScenarioItem(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint16>(0x84) >= 4)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x056 MISSION: handler RecvMissionItem 0x2F6010. Port (+0x24) 0xFFFF and the ranges 0x30-0x3F,
// 0x50-0xDF, 0xE0-0xEF are stored; anything else returns 0 (Adoulin 0xF0/0xF8, Coalition 0x100/0x108,
// TVR 0xFFFE).
auto s2c056Mission(MapSession*, CBasicPacket& packet) -> Result
{
    const auto port = packet.ref<uint16>(0x24);
    if (port == 0xFFFF || (port >= 0x30 && port <= 0x3F) || (port >= 0x50 && port <= 0xEF))
    {
        return Result::Pass;
    }

    return Result::Drop;
}

// 0x05A MOTIONMES: handler RecvEmotionMes 0x300A00. Same layout up to Mode (+0x16); the faith
// fields after it are ignored. MesNum (+0x10) must be < 0x4A and both ActIndexes (+0x0C, +0x0E) pass a
// `0x800 < idx` test, so 0x800 reads past the entity table.
auto s2c05AMotionMes(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint16>(0x10) >= 0x4A || packet.ref<uint16>(0x0C) >= kEntityCount)
    {
        return Result::Drop;
    }

    if (packet.ref<uint32>(0x08) != 0 && packet.ref<uint16>(0x0E) >= kEntityCount)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x05B WPOS / 0x065 WPOS2: handlers RecvWpos 0x2FF000 / Recv_wpos2 0x2FF010, both FUN_002FE4E0.
// Same layout (x, y, z, UniqueNo, ActIndex +0x14, Mode +0x16, dir +0x17); ActIndex passes `0x800 < idx`.
auto s2cWpos(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint16>(0x14) >= kEntityCount)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x061 CLISTATUS: handler RecvCliStatus 0x2F6B10 reads +0x04..+0x51 (hpmax .. myroom), exactly the
// LSB layout. Everything after myroom (su_lv, item levels, unity, mastery) is new: cut it off.
auto s2c061CliStatus(MapSession*, CBasicPacket& packet) -> Result
{
    packet.setSize(0x54);
    return Result::Rewritten;
}

// 0x063 MISCDATA: handler Recv_miscdata_unknown 0x2F6960 only knows type (+0x04) 2 (merits) and copies
// 4 bytes from +0x08 (limitPoints, merit bitfield). Every other type is post-2010.
auto s2c063MiscData(MapSession*, CBasicPacket& packet) -> Result
{
    if (packet.ref<uint16>(0x04) != 2)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x067: handler 0x2FB8E0. Sub-type = +0x04 & 0x3F, length = u16(+0x04) >> 6. None of the entity
// indexes it uses are checked.
//   2 (char sync, LSB CCharSyncPacket): index +0x06, needs length > 0x23. +0x12/+0x13 are read as two
//     nibbles (entity+0x1C4/0x1C5) and +0x14 as a u32 (entity+0x214); LSB puts the mount id (2014+) in
//     u16 +0x13, so zero it.
//   3 (entity name, LSB CEntitySetNamePacket): index +0x06, needs length > 0x13. The name is at +0x14
//     (0x18 bytes, copied when its first byte is > 0x20); LSB writes it at +0x18.
//   4 (pet): pet index at +0x0C, u32 +0x10 -> entity+0xCC. Not sent by LSB (see 0x068).
auto s2c067(MapSession*, CBasicPacket& packet) -> Result
{
    const auto kind = packet.ref<uint8>(0x04) & 0x3F;

    if (kind == 4)
    {
        return packet.ref<uint16>(0x0C) < kEntityCount ? Result::Pass : Result::Drop;
    }

    if (kind != 2 && kind != 3)
    {
        return Result::Drop;
    }

    if (packet.ref<uint16>(0x06) >= kEntityCount)
    {
        return Result::Drop;
    }

    if (kind == 2)
    {
        packet.ref<uint8>(0x13) = 0;
        packet.ref<uint8>(0x14) = 0;
        return Result::Rewritten;
    }

    // kind == 3: move the name from +0x18 to +0x14
    Rewrite rw(packet);
    rw.move(0x18, 0x14, 0x18);
    return Result::Rewritten;
}

// 0x068 PET SYNC (LSB CPetSyncPacket): the 2010 client has no 0x068; the same data went to 0x067
// sub-type 4 (pet index +0x0C, u32 +0x10 = TP). The layout is shared, so re-id it as 0x067.
// NOTE: translateS2C drops ids without a client handler before calling a translator, so this only runs
// once 0x068 is let through that check.
auto s2c068PetSync(MapSession*, CBasicPacket& packet) -> Result
{
    const auto pet = packet.ref<uint16>(0x0C);
    if (pet == 0 || pet >= kEntityCount)
    {
        return Result::Drop;
    }

    packet.ref<uint32>(0x10) = tpPercent(packet.ref<uint16>(0x10));
    packet.setType(0x067);
    return Result::Rewritten;
}

// 0x069 CHOCOBO_RACING: handler Recv_chocobo_racing 0x2CF020. Same layout (Mode +0x04, ParamIndex
// +0x05, ParamSize +0x06, data +0x08), but it memcpys ParamSize bytes to zone+0x2A948 + idx*0xC
// (mode 2, 8 x 12 bytes), zone+0x2A9A8 + idx*0xC (mode 3, 32 x 12 bytes) or zone+0x2AB28 (mode 4)
// without any check.
auto s2c069ChocoboRacing(MapSession*, CBasicPacket& packet) -> Result
{
    const auto   mode  = packet.ref<uint8>(0x04);
    const size_t index = packet.ref<uint8>(0x05);
    const size_t size  = packet.ref<uint8>(0x06);

    if (mode == 2 && index * 0x0C + size > 0x60)
    {
        return Result::Drop;
    }

    if (mode == 3 && index * 0x0C + size > 0x180)
    {
        return Result::Drop;
    }

    if (mode == 4 && size > 4)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

// 0x071 INFLUENCE: handler Recv_influence_colonization 0x2F6D60 only handles Mode (+0x04) 2 (campaign),
// in the LSB layout. Colonization (mode 3, Adoulin) is dropped.
auto s2c071Influence(MapSession*, CBasicPacket& packet) -> Result
{
    return packet.ref<uint8>(0x04) == 2 ? Result::Pass : Result::Drop;
}

// 0x074 CHOCOBO_LIST: handler GcItemRecvChocoboRaceInfo_ 0x38C2B0. Mode is the byte at +0x10; the byte
// at +0x11 (high byte of LSB's u16 Mode) is an unchecked index for modes 2/3, which copy a full
// 0x60/0xA0-byte table. LSB always sends 0 there.
auto s2c074ChocoboList(MapSession*, CBasicPacket& packet) -> Result
{
    const auto mode = packet.ref<uint8>(0x10);
    if ((mode == 2 || mode == 3) && packet.ref<uint8>(0x11) != 0)
    {
        return Result::Drop;
    }

    return Result::Pass;
}

} // namespace

void registerS2C_40(compat::Profile& p)
{
    p.s2c(0x043, s2c043TalkNumName);
    p.s2c(0x044, s2c044ExtendedJob);
    p.s2c(0x04B, s2c04BPbxResult);
    p.s2c(0x050, s2c050EquipList);
    p.s2c(0x051, s2c051GrapList);
    p.s2c(0x055, s2c055ScenarioItem);
    p.s2c(0x056, s2c056Mission);
    p.s2c(0x05A, s2c05AMotionMes);
    p.s2c(0x05B, s2cWpos);
    p.s2c(0x061, s2c061CliStatus);
    p.s2c(0x063, s2c063MiscData);
    p.s2c(0x065, s2cWpos);
    p.s2c(0x067, s2c067);
    p.s2c(0x068, s2c068PetSync);
    p.s2c(0x069, s2c069ChocoboRacing);
    p.s2c(0x071, s2c071Influence);
    p.s2c(0x074, s2c074ChocoboList);
}

} // namespace console
