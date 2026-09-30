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

#include "common/cbasetypes.h"

#include <string>

// Which client a session talks to. Every client names its platform (sPlatform) and build (Ver) in its
// unencrypted 0x00A; recv_parse identifies it there, once per zone-in, and keeps the result on the
// MapSession. The PC client names itself "WIN" and speaks today's layout; any other platform is a
// console (PS2, Xbox 360), served by the translation profile registered for its build (compat/profile.h).

namespace compat
{

class Profile;

enum class Platform : uint8
{
    Unknown, // a console tag this server does not know
    PC,      // "WIN"
    PS2,
    Xbox360,
};

auto platformName(Platform platform) -> const char*;

struct ClientInfo
{
    Platform       platform = Platform::PC; // until the 0x00A names it
    uint32         version  = 0;       // Ver of the 0x00A
    char           tag[5]   = {};      // sPlatform as sent (4 bytes, not terminated when full)
    const Profile* profile  = nullptr; // translation profile; nullptr: today's layout, nothing translated

    // Anything but the PC client ("WIN")
    auto console() const -> bool
    {
        return platform != Platform::PC;
    }

    auto translated() const -> bool
    {
        return profile != nullptr;
    }

    // "PS2 Ver 01328DE0 (ps2-20100904)", for logs and GM tools
    auto describe() const -> std::string;
};

// sPlatform (4 bytes) and Ver of an 0x00A -> the client and the profile that serves it
auto identify(const uint8* sPlatform, uint32 version) -> ClientInfo;

} // namespace compat
