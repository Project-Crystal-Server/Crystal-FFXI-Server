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

#include "protocols.h"

namespace compatibility::protocols
{

auto find(const std::string_view name) -> const Protocol*
{
    // Every protocol the map server has. Add an extension here beside its base.
    for (const auto* p : { &legacy() })
    {
        if (p->name() == name)
        {
            return p;
        }
    }
    return nullptr;
}

} // namespace compatibility::protocols
