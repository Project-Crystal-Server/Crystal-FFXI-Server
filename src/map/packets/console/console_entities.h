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

// Remaps every static-entity field of one packet (table in console_entities.cpp): today -> 2010 for
// outgoing packets, after the layout translator; 2010 -> today for incoming ones, after it.
auto remapEntitiesS2C(MapSession* PSession, CBasicPacket& packet) -> Result;
auto remapEntitiesC2S(MapSession* PSession, CBasicPacket& packet) -> Result;

} // namespace console
