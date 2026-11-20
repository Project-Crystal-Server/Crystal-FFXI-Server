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

#include "version.h"

#include <fmt/format.h>

namespace compatibility
{

namespace
{

auto isDigit(const char c) -> bool
{
    return c >= '0' && c <= '9';
}

} // namespace

auto platformName(const Platform platform) -> const char*
{
    switch (platform)
    {
        case Platform::PC:
            return "PC";
        case Platform::PS2:
            return "PS2";
        case Platform::Xbox360:
            return "Xbox 360";
        case Platform::Unknown:
        default:
            return "unknown";
    }
}

auto platformOf(const std::string_view version) -> Platform
{
    if (version.empty())
    {
        return Platform::Unknown;
    }

    switch (version.front())
    {
        case '2':
            return Platform::PS2;
        case '3':
            return Platform::PC;
        case '4':
            return Platform::Xbox360;
        default:
            return Platform::Unknown;
    }
}

auto Version::parse(const std::string_view text) -> std::optional<Version>
{
    // <platform digit><yy mm dd as 7 digits>[_<revision>]: "30100317_3" is 3 + 0100317 -> 2010-03-17
    if (text.size() < 8)
    {
        return std::nullopt;
    }

    for (size_t i = 0; i < 8; ++i)
    {
        if (!isDigit(text[i]))
        {
            return std::nullopt;
        }
    }

    Version v;
    v.text     = std::string(text);
    v.platform = platformOf(text);
    if (v.platform == Platform::Unknown)
    {
        return std::nullopt;
    }

    // digits 1..7 are the date with its century cut to 0: "0100317" -> 20000000 + 100317
    uint32 digits = 0;
    for (size_t i = 1; i < 8; ++i)
    {
        digits = digits * 10 + static_cast<uint32>(text[i] - '0');
    }
    v.date = 20000000 + digits;

    if (text.size() > 8)
    {
        if (text[8] != '_' || text.size() == 9)
        {
            return std::nullopt;
        }
        for (size_t i = 9; i < text.size(); ++i)
        {
            if (!isDigit(text[i]))
            {
                return std::nullopt;
            }
            v.revision = v.revision * 10 + (text[i] - '0');
        }
    }

    return v;
}

auto Version::describe() const -> std::string
{
    return fmt::format("{} {}", platformName(platform), text);
}

} // namespace compatibility
