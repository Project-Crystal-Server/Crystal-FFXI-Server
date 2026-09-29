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

#pragma once

#include "ps2_client.h"

#include "packets/basic.h"

#include <cstring>

// Translator groups. Each ps2_<dir>_<group>.cpp defines one register function; registerGroups calls them all.

namespace ps2
{

void registerS2C_00(); // ps2_s2c_00.cpp: 0x000-0x01B
void registerS2C_1C(); // ps2_s2c_1c.cpp: 0x01C-0x03F
void registerS2C_40(); // ps2_s2c_40.cpp: 0x040-0x07F
void registerS2C_80(); // ps2_s2c_80.cpp: 0x080-0x10F
void registerC2S();    // ps2_c2s.cpp

inline void registerGroups()
{
    registerS2C_00();
    registerS2C_1C();
    registerS2C_40();
    registerS2C_80();
    registerC2S();
}

// Helpers for translators: work on a copy of the payload so fields can move in either direction.
struct Rewrite
{
    CBasicPacket& packet;
    uint8         src[0x200]{};

    explicit Rewrite(CBasicPacket& p)
    : packet(p)
    {
        std::memcpy(src, static_cast<uint8*>(p), 0x1FF);
    }

    template <typename T>
    auto in(const size_t off) const -> T
    {
        T v;
        std::memcpy(&v, src + off, sizeof(T));
        return v;
    }

    template <typename T>
    void out(const size_t off, const T v)
    {
        std::memcpy(static_cast<uint8*>(packet) + off, &v, sizeof(T));
    }

    // copy n bytes from the original offset `from` to the new offset `to`
    void move(const size_t from, const size_t to, const size_t n)
    {
        std::memcpy(static_cast<uint8*>(packet) + to, src + from, n);
    }

    // zero the payload after the 4-byte header, then set the new size (a multiple of 4)
    void clear(const size_t newSize)
    {
        std::memset(static_cast<uint8*>(packet) + 4, 0, 0x1FF - 4);
        packet.setSize(newSize);
    }
};

} // namespace ps2
