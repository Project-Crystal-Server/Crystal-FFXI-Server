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

#include "compatibility/search_layout.h"

// The legacy protocol's search packets. Layouts come from the legacy client's search parser (FUN_003bece0 in
// ffxi_pol on PS2 20100904_2).

namespace compatibility::protocols
{

auto legacySearchLayout() -> const SearchLayout&
{
    static const SearchLayout layout = []
    {
        SearchLayout l;
        l.name = "legacy";

        // Legacy clients read the area as 8 bits and have area names for 1-252 only. With today's 10 bits every
        // field after it is read 2 bits off, so the entry shows another zone and no job or level.
        l.areaBits = 8;
        l.maxArea  = 0xFF;

        // Legacy clients know the second flag word and the language field by the tags 0x14 and 0x15, and tag
        // the second flag word of their requests 0x14.
        l.flags2Tag   = 0x14;
        l.languageTag = 0x15;

        // A legacy client's name filter has an exact-match bit ("=Name") ahead of a 4-bit length. Names are
        // matched by prefix either way, so the bit is skipped.
        l.nameExactMatchBit = true;
        l.nameLengthBits    = 4;

        // Legacy clients have one linkshell. Their party/linkshell request is 12 bytes, with padding where
        // today's second linkshell id goes, and a member entry carries one rank and no ids.
        l.linkshells = 1;

        // A legacy client's auction history has no current price or category: its sales start right after the
        // item number, so with today's header every price, date and name would be read 6 bytes off.
        l.historyPriceAndCategory = false;

        // Items a legacy client's install lacks show as "." in its auction lists.
        l.filterItems = true;
        return l;
    }();
    return layout;
}

} // namespace compatibility::protocols
