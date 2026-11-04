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

#include "version.h"

#include "common/cbasetypes.h"

#include <string>
#include <string_view>
#include <vector>

// Releases: the client versions this server translates for, and the protocol each one speaks.
//
// Each platform lists its releases in compatibility/platforms/. A release is keyed on its version (which names
// its platform) and names its protocol ("legacy"). The map server turns a release into a profile
// (src/map/compatibility/profile.h), the search server into a search layout (src/search/compatibility/search_layout.h).
// Neither depends on the other, so nothing here may depend on map or search code.

namespace compatibility
{

struct Release
{
    Version                  version;     // also its folder: res/compatibility/<version>/
    std::string              description; // "July 2009 PC client"
    std::string              protocol;    // "legacy"
    std::vector<std::string> versions;    // version strings it serves: its own, then res/compatibility/<version>/versions.txt
    std::vector<uint32>      vers;        // 0x00A Ver values it serves (versions.txt)

    // res/compatibility/<version>/<file>
    auto path(std::string_view file) const -> std::string;
};

// A release for the platform factories; the version must parse
auto makeRelease(std::string_view version, std::string description, std::string protocol) -> Release;

// Every release of every platform, in registration order (the first one of a console platform is its fallback)
auto releases() -> const std::vector<Release>&;

// The release for a lobby version string, else for a platform's 0x00A Ver, else the first release of a console
// platform; nullptr for a PC version no release lists (today's layout)
auto findRelease(Platform platform, uint32 ver, std::string_view version = {}) -> const Release*;

} // namespace compatibility
