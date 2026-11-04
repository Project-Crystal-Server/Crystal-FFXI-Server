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

#include <string>
#include <vector>

// The item ids a release's install has: res/compatibility/<version>/items.bin, "PS2ITM01" followed by a
// 0x10000-bit bitset (bit = item id). Shared by the map server (IdMaps) and the search server (auction lists).
// A missing or invalid file allows every item, with a warning.

namespace compatibility
{

class ItemSet
{
public:
    ItemSet() = default;
    explicit ItemSet(const std::string& path);

    // Item 0 and 0xFFFF (gil) always; every item without the file
    auto has(uint16 itemId) const -> bool;

    auto loaded() const -> bool
    {
        return !bits_.empty();
    }

private:
    std::vector<uint8> bits_; // 0x2000 bytes, empty without the file
};

} // namespace compatibility
