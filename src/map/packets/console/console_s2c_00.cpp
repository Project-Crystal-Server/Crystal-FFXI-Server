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

#include "console_groups.h"

#include "map_session.h"

#include <algorithm>
#include <cstring>

// Server->client ids 0x000-0x01B for the 2010 PS2 client. Layouts and reasoning: FFXI-PS2 docs/packets/s2c_00.md.
//
// Needs no translator (the 2010 handler reads today's layout): 0x005 packetcontrol, 0x006 naraku, 0x008 enterzone
// (the client copies the first 0x20 bytes), 0x009 message, 0x00B logout.

namespace console
{

void registerS2C_00(compat::Profile& p);

namespace
{

// Highest job the 2010 client knows (SCH). GEO 21, RUN 22 and MON 23 came later.
constexpr uint8  kLastJob  = 20;
constexpr uint32 kJobMask  = (1u << (kLastJob + 1)) - 1;
constexpr uint8  kFallback = 1; // WAR

auto clampMainJob(const uint8 job) -> uint8
{
    return job > kLastJob ? kFallback : job;
}

auto clampSubJob(const uint8 job) -> uint8
{
    return job > kLastJob ? 0 : job;
}

// Flags1 byte 0 (packet offset 0x20 in 0x00A / 0x00D): bits 5-7 are ChocoboIndex. The 2010 client knows 1 (a rented
// chocobo) and 2/3 (a raised chocobo, looks read from offset 0x34). Today's 3-7 are the later mounts; show a chocobo.
auto clampChocoboIndex(const uint8 flags) -> uint8
{
    if ((flags >> 5) > 2)
    {
        return static_cast<uint8>((flags & 0x1F) | (1 << 5));
    }

    return flags;
}

// 0x00A GP_SERV_LOGIN -- RecvLogIn 0x2D14F0 (table B).
// Same layout as today's, all 0x104 bytes (it even reads 0x9C/0x9E, 0xA8/0xA9 and 0x100, which XiPackets calls new).
// Only values the 2010 client cannot represent are fixed.
auto login(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    // Flags1 byte 0: LSB puts the Mounted effect's subPower here; keep ChocoboIndex in the 2010 range.
    packet.ref<uint8>(0x20) = clampChocoboIndex(packet.ref<uint8>(0x20));

    // IsMonstrosity (0x7E): the 2010 client reads 0x7C as a u32 ShipEnd, so a non-zero value here breaks ship timing.
    packet.ref<uint16>(0x7E) = 0;

    // MyroomMapNumber (0xAA): model ids FUN_002f4ee0 knows are 0x100-0x102, 0x120-0x123, 0xBD, 0xC7, 0xD6, 0xDB.
    auto& myroom = packet.ref<uint16>(0xAA);
    if (myroom == 745)
    {
        myroom = 0xBD; // San d'Oria [S]: today's model 745, the 2010 client's 189
    }
    else if (myroom == 0x124 || (myroom >= 0x267 && myroom <= 0x26A) || myroom == 0x2D9)
    {
        myroom = 0x100; // Adoulin, Mog House 2F, Feretory: no such model in 2010
    }

    // MyRoomExitBit (0xAE): LSB sends an index (1 San d'Oria .. 5 Aht Urhgan, 9 Adoulin), the 2010 client tests it as a
    // bit mask per Mog House (FUN_002f4ee0: bit 0 San d'Oria, 1 Bastok, 2 Windurst, 3 Jeuno, 4 Aht Urhgan, 5-7 [S]).
    auto& exitBit = packet.ref<uint8>(0xAE);
    exitBit       = (exitBit >= 1 && exitBit <= 5) ? static_cast<uint8>(1u << (exitBit - 1)) : 0;

    // Dancer (0xB0, 0x44 bytes, copied whole into the zone object)
    packet.ref<uint8>(0xB4)  = clampMainJob(packet.ref<uint8>(0xB4));
    packet.ref<uint8>(0xB7)  = clampSubJob(packet.ref<uint8>(0xB7));
    packet.ref<uint32>(0xB8) = packet.ref<uint32>(0xB8) & kJobMask;

    return Result::Rewritten;
}

// 0x00D GP_SERV_CHAR_PC -- RecvCharPc 0x2F6EF0 (table A), plus RecvUnknown 0x2D2AC0 (table B, SendFlg bookkeeping).
// 0x04-0x3D are unchanged (POS_HEAD, CostumeId 0x30, BallistaInfo 0x32, 0x33, raised-chocobo look 0x34, PetActIndex
// 0x3C). Today's 0x3E-0x47 (Monstrosity, Geo, hitbox, Flags6) did not exist: GrapIDTbl moves 0x48 -> 0x3E and the
// name 0x5A -> 0x50.
auto charPc(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    Rewrite rw(packet);

    // The client only takes PCs at 0x400-0x6FF (its entity table has 0x800 slots)
    const auto actIndex = rw.in<uint16>(0x08);
    if (actIndex < 0x400 || actIndex >= 0x700)
    {
        return Result::Drop;
    }

    const auto sendFlg = rw.in<uint8>(0x0A);

    // Everything but a despawn reads up to 0x3D (PetActIndex); Model reads GrapIDTbl 0x3E-0x4F.
    size_t newSize = 0x40;
    if (sendFlg & 0x10)
    {
        newSize = 0x50;
    }

    // Name: the client copies min(size - 0x54, 15) bytes from 0x50 (FUN_002d58a0), so the size is 0x54 + length.
    size_t nameLen = 0;
    if (sendFlg & 0x08)
    {
        nameLen = strnlen(reinterpret_cast<const char*>(rw.src + 0x5A), 15);
        newSize = std::max<size_t>(0x50, 0x54 + nameLen);
    }

    rw.clear(newSize);
    rw.move(0x04, 0x04, 0x3A);
    rw.move(0x48, 0x3E, 0x12);
    if (nameLen > 0)
    {
        rw.move(0x5A, 0x50, nameLen);
    }

    rw.out<uint8>(0x20, clampChocoboIndex(rw.in<uint8>(0x20)));

    return Result::Rewritten;
}

// 0x00E GP_SERV_CHAR_NPC -- RecvCharNpc 0x2F8CB0 (table A), plus RecvUnknown_B00E 0x2D2C00 (table B).
// Same layout as today's (SubKind 0x30, model/Grap 0x32, name 0x34 or 0x35, Name2 at 0x44). The client takes
// ActIndex 0x000-0x3FF and 0x700-0x7FF; LSB's dynamic entities go up to 0x8FF, which the client cannot hold.
auto charNpc(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    const auto actIndex = packet.ref<uint16>(0x08);
    if (actIndex < 0x400 || (actIndex >= 0x700 && actIndex < 0x800))
    {
        return Result::Pass;
    }

    return Result::Drop;
}

// 0x012 GP_SERV_GM -- RecvGm 0x3386C0; 0x013 GP_SERV_GMCOMMAND -- RecvGmCommand 0x338730.
// Both copy (size - 6) bytes into a 160-byte stack buffer and terminate it there: anything over 0xA4 bytes smashes
// the stack. LSB does not send either today; cap them in case it starts.
auto gmText(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    constexpr size_t kMaxSize = 0xA4;

    if (packet.getSize() <= kMaxSize)
    {
        return Result::Pass;
    }

    packet.setSize(kMaxSize);
    packet.ref<uint8>(kMaxSize - 1) = 0;

    return Result::Rewritten;
}

// Today's CHAT_MESSAGE_TYPE -> the 2010 client's Kind (RecvStdChat switches on 0x00-0x19, anything else is an error).
// 0xFF = drop.
auto chatKind(const uint8 kind) -> uint8
{
    if (kind <= 0x19)
    {
        return kind;
    }

    switch (kind)
    {
        case 26: // yell
            return 1; // shout
        case 27: // linkshell 2
        case 30: // linkshell 3
            return 5; // linkshell
        case 28: // linkshell 2, no speaker
        case 31: // linkshell 3, no speaker
        case 32:
            return 16; // linkshell, no speaker
        case 29: // system 3
            return 6; // system 1
        case 33: // unity
        case 34: // assist (J)
        case 35: // assist (E)
            return 1; // shout: a server-wide "name : text" line
        default:
            return 0xFF;
    }
}

// 0x017 GP_SERV_CHAT_STD -- RecvStdChat 0x39CDA0 (table B).
// 2010: 0x04 Attr (bit 0 = [GM]), 0x05 sName[15], 0x14 Kind, 0x15 Mes (size - 0x15 bytes, at most 0x96).
// Today: 0x04 Kind, 0x05 Attr, 0x06 Data (zone / assist ranks, dropped), 0x08 sName[15], 0x17 Mes[150].
auto chatStd(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    Rewrite rw(packet);

    const auto kind = chatKind(rw.in<uint8>(0x04));
    if (kind == 0xFF)
    {
        return Result::Drop;
    }

    const size_t oldSize = packet.getSize();
    const size_t avail   = oldSize > 0x17 ? std::min<size_t>(oldSize - 0x17, 0x96) : 0;
    const size_t mesLen  = strnlen(reinterpret_cast<const char*>(rw.src + 0x17), avail);

    rw.clear(0x15 + mesLen);
    rw.out<uint8>(0x04, rw.in<uint8>(0x05) & 0x01);
    rw.move(0x08, 0x05, 15);
    rw.out<uint8>(0x14, kind);
    rw.move(0x17, 0x15, mesLen);

    return Result::Rewritten;
}

// 0x01B GP_SERV_JOB_INFO -- Recv_job_info 0x3C3F50 (table A).
// Same layout up to 0x63: Dancer 0x04 (0x44 bytes), 24 job levels at 0x48, encumbrance at 0x60. The mentor / mastery
// fields after 0x64 did not exist and are cut.
auto jobInfo(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    packet.ref<uint8>(0x08)  = clampMainJob(packet.ref<uint8>(0x08));
    packet.ref<uint8>(0x0B)  = clampSubJob(packet.ref<uint8>(0x0B));
    packet.ref<uint32>(0x0C) = packet.ref<uint32>(0x0C) & kJobMask;

    for (size_t job = kLastJob + 1; job < 0x18; ++job)
    {
        packet.ref<uint8>(0x48 + job) = 0;
    }

    packet.setSize(0x64);

    return Result::Rewritten;
}

} // namespace

void registerS2C_00(compat::Profile& p)
{
    p.s2c(0x00A, login);
    p.s2c(0x00D, charPc);
    p.s2c(0x00E, charNpc);
    p.s2c(0x012, gmText);
    p.s2c(0x013, gmText);
    p.s2c(0x017, chatStd);
    p.s2c(0x01B, jobInfo);
}

} // namespace console
