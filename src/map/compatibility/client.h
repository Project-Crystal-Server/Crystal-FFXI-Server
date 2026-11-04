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

#pragma once

#include "compatibility/version.h"

#include "common/cbasetypes.h"

#include <string>

namespace compatibility
{

class Profile;

struct ClientInfo
{
    Platform       platform = Platform::PC; // from the version, else the 0x00A tag; PC until the 0x00A arrives
    Version        version;                 // the lobby's version string, parsed (text empty if the lobby stored none)
    uint32         ver        = 0;          // Ver of the 0x00A
    char           tag[5]     = {};         // sPlatform as sent (4 bytes, not terminated when full)
    uint32         expansions = 0;          // expansions the client says it has installed (lobby login)
    const Profile* profile    = nullptr;    // translation profile; nullptr: today's layout, nothing translated

    // Anything but a PC client
    auto console() const -> bool
    {
        return platform != Platform::PC;
    }

    auto translated() const -> bool
    {
        return profile != nullptr;
    }

    // "PS2 20100904_2 Ver 00000000 expansions 1E (20100904_2)", for logs and GM tools
    auto describe() const -> std::string;
};

// sPlatform (4 bytes) and Ver of an 0x00A, plus what the lobby stored for the session (accounts_sessions
// client_version / client_expansions; empty and 0 without it) -> the client and the profile that serves it.
// The platform comes from the version when it has one, from sPlatform otherwise.
auto identify(const uint8* sPlatform, uint32 ver, const std::string& version = {}, uint32 expansions = 0) -> ClientInfo;

} // namespace compatibility
