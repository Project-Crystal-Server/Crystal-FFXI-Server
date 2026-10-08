/*
===========================================================================

  Copyright (c) 2010-2015 Darkstar Dev Teams

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
#include "data_loader.h"

// How a player entry is laid out on the wire. Today's clients read the area as 10 bits and know the flags2 and
// language fields by the tags 0x16 and 0x17. The 2010 PS2 client (parser FUN_003bece0 in ffxi_pol) reads the area as
// 8 bits (names exist for 1-252 only) and knows flags2 and language by the tags 0x14 and 0x15; with the wider area
// every field after it is read 2 bits off, so it shows another zone and no job or level.
enum class SearchListLayout : uint8
{
    Current,
    Ps2_2010,
};

class CSearchListPacket
{
public:
    CSearchListPacket(uint32 Total, SearchListLayout layout = SearchListLayout::Current);

    auto AddPlayer(const SearchEntity& player) -> bool;
    void SetFinal();

    auto GetData() -> uint8*;
    auto GetSize() const -> uint16;

private:
    uint32           m_offset{};
    uint8            m_data[1024]{};
    SearchListLayout m_layout{ SearchListLayout::Current };
};
