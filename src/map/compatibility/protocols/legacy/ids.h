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
#include <span>

// These utilities provide a way for our translators to properly remap modern client data to legacy client data
// Each of these refer to a different client resource type
namespace compatibility::legacy::ids
{

// Modern Home Point event (post Dec 2013). The npc named "Home Point #1".
constexpr uint16 kHomePointEvent = 8700;

auto homePointEvent(uint16 zoneId) -> std::optional<uint16>;

auto dialog(uint16 zoneId, uint16 messageId) -> std::optional<uint16>;
auto hasZone(uint16 zoneId) -> bool;
auto mesNum(uint16 zoneId, uint16 mesNum) -> std::optional<uint16>;
auto hasItem(uint16 itemId) -> bool;
auto hasEvent(uint16 zoneId, uint16 eventId) -> bool;

// legacy map vendor utils
struct MapOption
{
    uint16 keyItem;
    uint8  slot;
};

auto isMapVendorEvent(uint16 zoneId, uint16 eventId) -> bool;
auto getMapVendorOptions(uint16 zoneId, uint16 eventId) -> std::span<const MapOption>;

// Static entities are NPCs, mobs and doors: UniqueNo is 0x01000000 | zone << 12 | index, and ActIndex is the index.
auto isStatic(uint32 uniqueNo) -> bool;

// Convert today's IDs to legacy client IDs
auto uniqueNoToClient(uint32 uniqueNo) -> std::optional<uint32>;
auto actIndexToClient(uint16 zoneId, uint16 actIndex) -> std::optional<uint16>;

// Convert legacy client IDs to today's IDs
auto uniqueNoFromClient(uint32 uniqueNo) -> std::optional<uint32>;
auto actIndexFromClient(uint16 zoneId, uint16 actIndex) -> std::optional<uint16>;

} // namespace compatibility::legacy::ids
