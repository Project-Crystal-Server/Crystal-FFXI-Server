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

namespace compatibility::platforms::pc
{

void addReleases(std::vector<Release>& out)
{
    // Note: If the platform is detected to be PC 3xxxxxxxxx and doesn't have a version-specific release, it will use the *first* release registered.
    out.push_back(makeRelease("30090710_2", "July 2009 PC client", "legacy"));
    out.push_back(makeRelease("30100317_3", "Mar 2010 PC client", "legacy"));
}

} // namespace compatibility::platforms::pc
