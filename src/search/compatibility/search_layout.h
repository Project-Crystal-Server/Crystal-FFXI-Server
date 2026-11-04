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

#include "compatibility/item_set.h"
#include "compatibility/releases.h"

#include "common/cbasetypes.h"

#include <string>
#include <string_view>

// How a protocol lays out the search server's packets (search lists, party and linkshell lists, auction history).
//
// Today's layout is the default; a protocol whose search packets differ has its own SearchLayout, defined in
// src/search/compatibility/protocols/<name>/search_layout.cpp. An extension copies its base's layout and
// changes only what differs, like the map server's Protocol (src/map/compatibility/protocol.h). The rest of the
// search server reads the fields and knows nothing about platforms, versions or protocols.

namespace compatibility
{

struct SearchLayout
{
    std::string name = "today";

    // Player entries (search, party and linkshell lists) and search requests
    uint8  areaBits    = 10;    // width of an area (zone) field
    uint16 maxArea     = 0x3FF; // a zone above this is sent as 0 ("unknown")
    uint8  flags2Tag   = 0x16;  // entry tag of the second flag word
    uint8  languageTag = 0x17;  // entry tag of the language field

    // Search requests
    bool  nameExactMatchBit = false; // a "=Name" bit precedes the name length
    uint8 nameLengthBits    = 5;

    // Linkshells a client has: 2 today. With 1 the party/linkshell request has no second id and a linkshell
    // member entry carries a single rank and no ids.
    uint8 linkshells = 2;

    // Auction history: today's header carries the current price and category before the sales
    bool historyPriceAndCategory = true;

    // Keep items the release's install lacks (its items.bin) out of auction lists
    bool filterItems = false;

    auto area(const uint16 zone) const -> uint16
    {
        return zone <= maxArea ? zone : 0;
    }
};

auto todaysSearchLayout() -> const SearchLayout&;

// The search layout of a protocol by name; today's for a protocol without one
auto searchLayoutFor(std::string_view protocol) -> const SearchLayout&;

// The search side of a client, from the version string its lobby login stored: its release (nullptr: today's
// client), the layout it reads and the items its install has.
struct SearchClient
{
    const Release*      release = nullptr;
    const SearchLayout* layout  = &todaysSearchLayout();
    const ItemSet*      items   = nullptr; // nullptr unless the layout filters items

    auto hasItem(const uint16 itemId) const -> bool
    {
        return items == nullptr || items->has(itemId);
    }
};

auto searchClient(std::string_view version) -> SearchClient;

namespace protocols
{

// src/search/compatibility/protocols/legacy/search_layout.cpp
auto legacySearchLayout() -> const SearchLayout&;

} // namespace protocols

} // namespace compatibility
