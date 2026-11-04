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

#include "item_set.h"

#include "common/logging.h"

#include <cstring>
#include <fstream>

namespace compatibility
{

ItemSet::ItemSet(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    char          magic[8] = {};
    bits_.resize(0x2000);
    if (!f.read(magic, sizeof(magic)) || std::memcmp(magic, "PS2ITM01", 8) != 0 ||
        !f.read(reinterpret_cast<char*>(bits_.data()), static_cast<std::streamsize>(bits_.size())))
    {
        ShowWarningFmt("compatibility: {} missing or invalid; items are not filtered", path);
        bits_.clear();
        return;
    }
    ShowInfoFmt("compatibility: {} loaded", path);
}

auto ItemSet::has(const uint16 itemId) const -> bool
{
    if (itemId == 0 || itemId == 0xFFFF || bits_.empty())
    {
        return true;
    }
    return (bits_[itemId >> 3] & (1u << (itemId & 7))) != 0;
}

} // namespace compatibility
