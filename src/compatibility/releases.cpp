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

#include "releases.h"

#include "platforms/platforms.h"

#include "common/logging.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace compatibility
{

namespace
{

// res/compatibility/<version>/versions.txt: further version strings and 0x00A Ver values the release serves, one
// per line (a Ver in hex with 0x, or decimal; # starts a comment). The login logs every client's version and Ver,
// so a new one is pinned without a rebuild.
void readVersionsFile(Release& r)
{
    std::ifstream f(r.path("versions.txt"));
    std::string   line;
    while (std::getline(f, line))
    {
        if (const auto hash = line.find('#'); hash != std::string::npos)
        {
            line.resize(hash);
        }

        const auto b = line.find_first_not_of(" \t\r");
        if (b == std::string::npos)
        {
            continue;
        }
        auto entry = line.substr(b);
        entry.resize(entry.find_last_not_of(" \t\r") + 1);

        // a whole number (0x... or decimal) is a Ver; anything else ("20100904_2") a lobby version string
        size_t used = 0;
        try
        {
            const auto value = std::stoul(entry, &used, 0);
            if (used == entry.size())
            {
                r.vers.push_back(static_cast<uint32>(value));
                continue;
            }
        }
        catch (const std::exception&)
        {
        }

        if (platformOf(entry) != r.version.platform)
        {
            ShowWarningFmt("compatibility: {} lists {}, a {} version; ignored", r.path("versions.txt"), entry, platformName(platformOf(entry)));
            continue;
        }
        if (std::find(r.versions.begin(), r.versions.end(), entry) == r.versions.end())
        {
            r.versions.push_back(std::move(entry));
        }
    }
}

auto buildReleases() -> std::vector<Release>
{
    std::vector<Release> out;
    platforms::addReleases(out);
    for (auto& r : out)
    {
        readVersionsFile(r);
        ShowInfoFmt("compatibility: release {} ({}) speaks {}, serves {} version string(s), {} Ver value(s)", r.version.text,
                    platformName(r.version.platform), r.protocol, r.versions.size(), r.vers.size());
    }
    return out;
}

} // namespace

auto Release::path(const std::string_view file) const -> std::string
{
    return fmt::format("res/compatibility/{}/{}", version.text, file);
}

auto makeRelease(const std::string_view version, std::string description, std::string protocol) -> Release
{
    auto parsed = Version::parse(version);
    if (!parsed)
    {
        throw std::invalid_argument(fmt::format("compatibility: release version {:?} does not parse", version));
    }

    Release r;
    r.version     = std::move(*parsed);
    r.description = std::move(description);
    r.protocol    = std::move(protocol);
    r.versions.push_back(r.version.text);
    return r;
}

auto releases() -> const std::vector<Release>&
{
    static const auto all = buildReleases();
    return all;
}

auto findRelease(const Platform platform, const uint32 ver, const std::string_view version) -> const Release*
{
    // A version string names one release, whatever the platform
    if (!version.empty())
    {
        for (const auto& r : releases())
        {
            if (std::find(r.versions.begin(), r.versions.end(), version) != r.versions.end())
            {
                return &r;
            }
        }
    }

    const Release* byVer    = nullptr;
    const Release* fallback = nullptr;
    for (const auto& r : releases())
    {
        if (r.version.platform != platform)
        {
            continue;
        }
        if (byVer == nullptr && std::find(r.vers.begin(), r.vers.end(), ver) != r.vers.end())
        {
            byVer = &r;
        }
        // A PC release serves only the versions it lists: every other PC client speaks today's layout
        if (fallback == nullptr && platform != Platform::PC)
        {
            fallback = &r;
        }
    }
    return byVer ? byVer : fallback;
}

} // namespace compatibility
