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

#include "compatibility/protocol.h"

#include <string_view>

namespace compatibility::protocols
{

// Core legacy protocol design for client versions between 2008 and 2010
auto legacy() -> const Protocol&;

// A protocol by the name a release gives it (compatibility/platforms/), or nullptr
auto find(std::string_view name) -> const Protocol*;

} // namespace compatibility::protocols
