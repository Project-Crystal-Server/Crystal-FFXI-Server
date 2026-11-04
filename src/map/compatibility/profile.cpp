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

#include "profile.h"

#include "guards.h"

#include "protocols/protocols.h"

#include "common/logging.h"

#include "map_session.h"
#include "packets/basic.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>

namespace compatibility
{

namespace
{

// A profile for every release whose protocol this server has
auto buildProfiles() -> std::vector<std::unique_ptr<Profile>>
{
    std::vector<std::unique_ptr<Profile>> out;
    for (const auto& r : releases())
    {
        const auto* protocol = protocols::find(r.protocol);
        if (protocol == nullptr)
        {
            ShowWarningFmt("compatibility: release {} speaks protocol {:?}, which the map server does not have; it is not translated", r.version.text, r.protocol);
            continue;
        }
        out.push_back(std::make_unique<Profile>(r, *protocol));
        ShowInfoFmt("compatibility: profile {} ({}) speaks {}", r.version.text, platformName(r.version.platform), protocol->describe());
    }
    return out;
}

thread_local const Profile* tActive = nullptr;

// Sets the active profile for the duration of one translation
struct ActiveScope
{
    const Profile* previous;

    explicit ActiveScope(const Profile* profile)
    : previous(std::exchange(tActive, profile))
    {
    }

    ~ActiveScope()
    {
        tActive = previous;
    }

    ActiveScope(const ActiveScope&)                    = delete;
    ActiveScope(ActiveScope&&)                         = delete;
    auto operator=(const ActiveScope&) -> ActiveScope& = delete;
    auto operator=(ActiveScope&&) -> ActiveScope&      = delete;
};

// send_parse runs on the map's network thread; translators and the send loop share it.
thread_local std::vector<std::vector<uint8>> tEmitted;

auto resultName(const Result r) -> const char*
{
    return r == Result::Drop ? "drop" : r == Result::Rewritten ? "rewritten"
                                                               : "pass";
}

} // namespace

Profile::Profile(const Release& release, const Protocol& protocol)
: release_(&release)
, protocol_(&protocol)
{
}

auto Profile::maps() const -> const IdMaps&
{
    if (!maps_)
    {
        maps_ = std::make_unique<IdMaps>("res/compatibility/" + name());
    }
    return *maps_;
}

auto Profile::translateS2C(MapSession* PSession, CBasicPacket& packet) const -> Result
{
    return protocol_->translateS2C(PSession, packet);
}

auto Profile::translateC2S(MapSession* PSession, CBasicPacket& packet) const -> Result
{
    return protocol_->translateC2S(PSession, packet);
}

auto profiles() -> const std::vector<std::unique_ptr<Profile>>&
{
    static const auto all = buildProfiles();
    return all;
}

auto findProfile(const Platform platform, const uint32 ver, const std::string& version) -> const Profile*
{
    const auto* release = findRelease(platform, ver, version);
    if (release == nullptr)
    {
        return nullptr;
    }
    for (const auto& p : profiles())
    {
        if (&p->release() == release)
        {
            return p.get();
        }
    }
    return nullptr;
}

auto zoneAvailable(const MapSession* PSession, const uint16 zoneId) -> bool
{
    if (PSession == nullptr || PSession->client.profile == nullptr || zoneId >= 1000)
    {
        return true;
    }
    return PSession->client.profile->maps().hasZoneData(zoneId);
}

auto active() -> const Profile&
{
    // Translators only run inside translateS2C / translateC2S, which set it
    return *tActive;
}

auto tracing() -> bool
{
    static const bool on = []
    {
        for (const auto* name : { "XI_COMPAT_TRACE", "XI_PS2_TRACE" })
        {
            const char* v = std::getenv(name);
            if (v != nullptr && *v != '\0' && *v != '0')
            {
                return true;
            }
        }
        return false;
    }();
    return on;
}

auto translateS2C(MapSession* PSession, CBasicPacket& packet) -> Result
{
    const auto* profile = PSession->client.profile;
    if (profile == nullptr)
    {
        return Result::Pass;
    }

    ActiveScope scope(profile);
    const auto  id   = packet.getType();
    const auto  size = packet.getSize();

    // The event an event packet carries, read before a translator can change or drop it
    uint16 eventId = 0;
    if (id == 0x032 || id == 0x033)
    {
        eventId = packet.ref<uint16>(0x0C);
    }
    else if (id == 0x034)
    {
        eventId = packet.ref<uint16>(0x2C);
    }

    auto result = profile->translateS2C(PSession, packet);
    if (guards::afterTranslateS2C(PSession, packet, id, eventId, result == Result::Drop))
    {
        result = Result::Rewritten;
    }
    if (tracing())
    {
        ShowInfoFmt("{} trace: s2c {:03X} size {:03X} seq {} -> {} {:03X} size {:03X}", profile->name(), id, size, packet.getSequence(), resultName(result), packet.getType(), result == Result::Drop ? 0 : packet.getSize());
    }
    return result;
}

auto translateC2S(MapSession* PSession, CBasicPacket& packet) -> Result
{
    const auto* profile = PSession->client.profile;
    if (profile == nullptr)
    {
        return Result::Pass;
    }

    ActiveScope scope(profile);
    const auto  id     = packet.getType();
    const auto  size   = packet.getSize();
    const auto  result = profile->translateC2S(PSession, packet);
    if (tracing() && id != 0x015) // position: several a second
    {
        ShowInfoFmt("{} trace: c2s {:03X} size {:03X} -> {}", profile->name(), id, size, resultName(result));
    }
    return result;
}

void emitS2C(const uint8* packet, const size_t size)
{
    tEmitted.emplace_back(packet, packet + size);
}

auto takeEmittedS2C() -> std::vector<std::vector<uint8>>
{
    return std::exchange(tEmitted, {});
}

} // namespace compatibility
