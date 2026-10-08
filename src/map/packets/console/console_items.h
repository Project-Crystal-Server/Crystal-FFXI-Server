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

#include "console_client.h"

namespace console
{

// Keeps items the 2010 install does not have (ids::hasItem) away from the PS2 client, which would
// show them as "." placeholders. Runs on the 2010 layout, after the s2c translators (console_items.cpp).
auto filterItemsS2C(MapSession* PSession, CBasicPacket& packet) -> Result;

} // namespace console
