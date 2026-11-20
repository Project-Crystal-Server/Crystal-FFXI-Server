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

#include "search_layout.h"

#include "common/logging.h"

#include <memory>
#include <unordered_map>

namespace compatibility
{

namespace
{

// Every protocol with a search layout of its own. Add an extension's layout here beside its base.
auto layoutsByProtocol() -> const std::unordered_map<std::string, const SearchLayout*>&
{
    static const std::unordered_map<std::string, const SearchLayout*> all = {
        { "legacy", &protocols::legacySearchLayout() },
    };
    return all;
}

// Each release's items, loaded once (only for releases whose layout filters items)
auto itemsByRelease() -> const std::unordered_map<const Release*, std::unique_ptr<ItemSet>>&
{
    static const auto all = []
    {
        std::unordered_map<const Release*, std::unique_ptr<ItemSet>> out;
        for (const auto& r : releases())
        {
            if (searchLayoutFor(r.protocol).filterItems)
            {
                out.emplace(&r, std::make_unique<ItemSet>(r.path("items.bin")));
            }
        }
        return out;
    }();
    return all;
}

} // namespace

auto todaysSearchLayout() -> const SearchLayout&
{
    static const SearchLayout today;
    return today;
}

auto searchLayoutFor(const std::string_view protocol) -> const SearchLayout&
{
    const auto& all = layoutsByProtocol();
    if (const auto it = all.find(std::string(protocol)); it != all.end())
    {
        return *it->second;
    }
    return todaysSearchLayout();
}

auto searchClient(const std::string_view version) -> SearchClient
{
    SearchClient client;
    client.release = findRelease(platformOf(version), 0, version);
    if (client.release == nullptr)
    {
        return client;
    }

    client.layout = &searchLayoutFor(client.release->protocol);
    if (const auto it = itemsByRelease().find(client.release); it != itemsByRelease().end())
    {
        client.items = it->second.get();
    }
    return client;
}

} // namespace compatibility
