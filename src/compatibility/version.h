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

#include "common/cbasetypes.h"

#include <optional>
#include <string>
#include <string_view>

// Platforms and versions.
//
// A client is described by its platform (PC, PS2, Xbox 360) and its version: the version string of its
// patch.ver, which the lobby stores for the session (accounts_sessions.client_version). The first digit of a
// version names the platform; the rest is the patch date and a revision:
//
//     30100317_3   PC        2010-03-17, revision 3
//     20100904_2   PS2       2010-09-04, revision 2
//     4........_.  Xbox 360
//
// So a version alone says which platform a client is; profiles are keyed on versions, never on names.

namespace compatibility
{

enum class Platform : uint8
{
    Unknown, // a version or tag this server does not know
    PC,      // versions 3xxxxxxx, 0x00A sPlatform "WIN"
    PS2,     // versions 2xxxxxxx, "PS2"
    Xbox360, // versions 4xxxxxxx, "X..."
};

auto platformName(Platform platform) -> const char*;
auto platformOf(std::string_view version) -> Platform;

struct Version
{
    std::string text;                         // as sent / listed, e.g. "30100317_3"
    Platform    platform = Platform::Unknown; // from the first digit
    uint32      date     = 0;                 // 20100317
    uint32      revision = 0;                 // 3

    static auto parse(std::string_view text) -> std::optional<Version>;

    auto describe() const -> std::string;
};

} // namespace compatibility
