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

#include "compatibility/releases.h"

#include <vector>

// Each platform's releases (compatibility/releases.h): the versions it has a translation for and the protocol each
// one speaks. Data only: the map and search servers both read these.

namespace compatibility::platforms
{

namespace pc
{

void addReleases(std::vector<Release>& out);

}

namespace ps2
{

void addReleases(std::vector<Release>& out);

}

namespace xbox360
{

void addReleases(std::vector<Release>& out);

}

inline void addReleases(std::vector<Release>& out)
{
    ps2::addReleases(out);
    pc::addReleases(out);
    xbox360::addReleases(out);
}

} // namespace compatibility::platforms
