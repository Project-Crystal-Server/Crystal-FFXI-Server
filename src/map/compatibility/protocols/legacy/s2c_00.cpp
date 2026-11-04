/*
===========================================================================

  Copyright (c) 2026 Project Crystal Dev Team

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

#include "groups.h"

#include "map_session.h"

#include <algorithm>
#include <cstring>

namespace compatibility::legacy
{

void registerS2C_00(compatibility::Protocol& p);

namespace
{

// Legacy protocol is targeted at Wings of the Goddess
// We need to ensure that SoA jobs are never sent
constexpr uint8  kLastJob  = 20; // SCH
constexpr uint32 kJobMask  = (1u << (kLastJob + 1)) - 1;
constexpr uint8  kFallback = 1;

auto clampMainJob(const uint8 job) -> uint8
{
    return job > kLastJob ? kFallback : job;
}

auto clampSubJob(const uint8 job) -> uint8
{
    return job > kLastJob ? 0 : job;
}

// Mounts do not exist yet, so we must turn everything into
// 1: rental chocobo
// 2: raised chocobo
auto clampChocoboIndex(const uint8 flags) -> uint8
{
    if ((flags >> 5) > 2)
    {
        return static_cast<uint8>((flags & 0x1F) | (1 << 5));
    }

    return flags;
}

// 0x00A - GP_SERV_LOGIN
// Same layout as today. Only values legacy clients can't show are fixed.
auto login(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    packet.ref<uint8>(0x20) = clampChocoboIndex(packet.ref<uint8>(0x20));

    // Remove Monstrosity data
    packet.ref<uint16>(0x7E) = 0;

    // Remap Mog House models which do not exist on legacy clients
    auto& myroom = packet.ref<uint16>(0xAA);
    if (myroom == 745)
    {
        myroom = 0xBD; // San d'Oria [S]
    }
    else if (myroom == 0x124 || (myroom >= 0x267 && myroom <= 0x26A) || myroom == 0x2D9)
    {
        myroom = 0x100; // Adoulin, Mog House 2F, Feretory
    }

    // The server sends the Mog House exit as an index (1 San d'Oria to 5 Aht Urhgan).
    // Legacy clients read it as a bit mask, one bit per Mog House.
    auto& exitBit = packet.ref<uint8>(0xAE);
    exitBit       = (exitBit >= 1 && exitBit <= 5) ? static_cast<uint8>(1u << (exitBit - 1)) : 0;

    packet.ref<uint8>(0xB4)  = clampMainJob(packet.ref<uint8>(0xB4));
    packet.ref<uint8>(0xB7)  = clampSubJob(packet.ref<uint8>(0xB7));
    packet.ref<uint32>(0xB8) = packet.ref<uint32>(0xB8) & kJobMask;

    return Result::Rewritten;
}

// 0x00D - GP_SERV_CHAR_PC
// Removes Monstrosity, Geomancy, hitbox, and flags6 fields.
// Moves the look data forward 10 bytes and resizes the packet
auto charPc(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    Rewrite rw(packet);

    // Ensure the Entity List is bounds checked for legacy clients.
    const auto actIndex = rw.in<uint16>(0x08);
    if (actIndex < 0x400 || actIndex >= 0x700)
    {
        return Result::Drop;
    }

    const auto sendFlg = rw.in<uint8>(0x0A);

    // The client reads up to the pet index, plus the model when it's sent.
    size_t newSize = 0x40;
    if (sendFlg & 0x10)
    {
        newSize = 0x50;
    }

    // The client takes the name length from the packet size, up to 15 bytes.
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

// 0x00E - GP_SERV_CHAR_NPC
auto charNpc(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    const auto actIndex = packet.ref<uint16>(0x08);
    if (!(actIndex < 0x400 || (actIndex >= 0x700 && actIndex < 0x800)))
    {
        return Result::Drop;
    }

    // Gathering points use the resource node models (2422-2424), added around 2014. Legacy clients
    // have no files for them and draw nothing, so send their generic point model (52) instead.
    // Standard look: size (0 = standard) at 0x30, model id at 0x32.
    constexpr uint16 kOldPointModel = 52;
    if (packet.getSize() >= 0x34 && packet.ref<uint16>(0x30) == 0)
    {
        const auto model = packet.ref<uint16>(0x32);
        if (model >= 2422 && model <= 2424)
        {
            packet.ref<uint16>(0x32) = kOldPointModel;
            return Result::Rewritten;
        }
    }

    return Result::Pass;
}

// 0x012 - GP_SERV_GM
// 0x013 - GP_SERV_COMMAND_GMCOMMAND
// Resize into 160-byte buffer and clamp the size to prevent overflows.
// Preventative, the server doesn't send these today.
auto gm(MapSession* /* PSession */, CBasicPacket& packet) -> Result
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

auto chatKind(const uint8 kind) -> uint8
{
    if (kind <= 0x19)
    {
        return kind;
    }

    switch (kind)
    {
        case 26:      // yell
            return 1; // shout
        case 27:      // linkshell 2
        case 30:      // linkshell 3
            return 5; // linkshell
        case 28:      // linkshell 2, no speaker
        case 31:      // linkshell 3, no speaker
        case 32:
            return 16; // linkshell, no speaker
        case 29:       // system 3
            return 23;
        case 33:      // unity
        case 34:      // assist (J)
        case 35:      // assist (E)
            return 1; // shout: a server-wide "name : text" line
        default:
            return 0xFF;
    }
}

// 0x017 - GP_SERV_CHAT_STD
// Legacy clients have a shorter header: GM flag, name, then chat type, with no zone or assist data.
auto chatStd(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    Rewrite rw(packet);

    const auto kind = chatKind(rw.in<uint8>(0x04));
    if (kind == 0xFF)
    {
        return Result::Drop;
    }

    // Text sizes must be clamped to 150 bytes.
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

// 0x01B - JOB_INFO
// Removes the Mentor and Job Mastery fields
auto jobInfo(MapSession* /* PSession */, CBasicPacket& packet) -> Result
{
    packet.ref<uint8>(0x08)  = clampMainJob(packet.ref<uint8>(0x08));
    packet.ref<uint8>(0x0B)  = clampSubJob(packet.ref<uint8>(0x0B));
    packet.ref<uint32>(0x0C) = packet.ref<uint32>(0x0C) & kJobMask;

    // Ensure we don't send SoA Job Data
    for (size_t job = kLastJob + 1; job < 0x18; ++job)
    {
        packet.ref<uint8>(0x48 + job) = 0;
    }

    packet.setSize(0x64);

    return Result::Rewritten;
}

} // namespace

void registerS2C_00(compatibility::Protocol& p)
{
    p.s2c(0x00A, login);
    p.s2c(0x00D, charPc);
    p.s2c(0x00E, charNpc);
    p.s2c(0x012, gm);
    p.s2c(0x013, gm);
    p.s2c(0x017, chatStd);
    p.s2c(0x01B, jobInfo);
}

} // namespace compatibility::legacy
