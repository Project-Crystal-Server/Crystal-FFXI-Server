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

#include "platforms.h"

namespace compatibility::platforms::ps2
{

void addReleases(std::vector<Release>& out)
{
    // Note: If the platform is detected to be PS2 2xxxxxxxxx and doesn't have a version-specific release, it will use the *first* release registered.
    out.push_back(makeRelease("20100904_2", "retail PS2 client, SCUS-97266", "legacy"));
}

} // namespace compatibility::platforms::ps2
