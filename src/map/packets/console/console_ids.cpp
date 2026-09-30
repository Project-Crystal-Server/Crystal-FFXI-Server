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

#include "console_ids.h"

#include "packets/compat/profile.h"

namespace console::ids
{

namespace
{

auto maps() -> const compat::IdMaps&
{
    return compat::active().maps();
}

} // namespace

auto dialog(const uint16 zoneId, const uint16 messageId) -> std::optional<uint16>
{
    return maps().dialog(zoneId, messageId);
}

auto hasZone(const uint16 zoneId) -> bool
{
    return maps().hasZone(zoneId);
}

auto hasItem(const uint16 itemId) -> bool
{
    return maps().hasItem(itemId);
}

auto hasEvent(const uint16 zoneId, const uint16 eventId) -> bool
{
    return maps().hasEvent(zoneId, eventId);
}

auto mesNum(const uint16 zoneId, const uint16 mesNum) -> std::optional<uint16>
{
    return maps().mesNum(zoneId, mesNum);
}

auto isStatic(const uint32 uniqueNo) -> bool
{
    return compat::IdMaps::isStatic(uniqueNo);
}

auto uniqueNoToClient(const uint32 uniqueNo) -> std::optional<uint32>
{
    return maps().uniqueNoToOld(uniqueNo);
}

auto uniqueNoFromClient(const uint32 uniqueNo) -> std::optional<uint32>
{
    return maps().uniqueNoFromOld(uniqueNo);
}

auto actIndexToClient(const uint16 zoneId, const uint16 actIndex) -> std::optional<uint16>
{
    return maps().actIndexToOld(zoneId, actIndex);
}

auto actIndexFromClient(const uint16 zoneId, const uint16 actIndex) -> std::optional<uint16>
{
    return maps().actIndexFromOld(zoneId, actIndex);
}

} // namespace console::ids
