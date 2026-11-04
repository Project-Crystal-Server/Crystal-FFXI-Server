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

#include "client.h"
#include "id_maps.h"
#include "protocol.h"
#include "compatibility/releases.h"
#include "compatibility/version.h"

#include "common/cbasetypes.h"

#include <memory>
#include <string>
#include <vector>

// Translation profiles: one per client version whose packets or data differ from today's.
//
// A profile is keyed on a version (compatibility/version.h), which also names its platform. It pairs that
// version with the protocol its client speaks (compatibility/protocol.h) and with its id maps
// (compatibility/id_maps.h) from res/compatibility/<version>/.
//
// Profiles are built once, by the platform factories (compatibility/platforms/). A session is matched on the
// version string the lobby stored for it, then on the Ver of its 0x00A (further versions and Ver values a
// profile serves are listed in res/compatibility/<version>/versions.txt); failing both, the first profile
// registered for a console platform serves it; a PC version no profile lists gets today's layout.
// For a session with a profile, send_parse calls translateS2C on every outgoing packet
// and parse calls translateC2S on every incoming one before it is dispatched.

class CBasicPacket;
struct MapSession;

namespace compatibility
{

class Profile
{
public:
    Profile(const Release& release, const Protocol& protocol);

    // The version this profile was made for, e.g. "30100317_3"; also its res/compatibility/ folder
    auto release() const -> const Release&
    {
        return *release_;
    }

    // The release's version, e.g. "30100317_3"; also its res/compatibility/ folder
    auto name() const -> const std::string&
    {
        return release_->version.text;
    }

    auto version() const -> const Version&
    {
        return release_->version;
    }

    auto platform() const -> Platform
    {
        return release_->version.platform;
    }

    auto description() const -> const std::string&
    {
        return release_->description;
    }

    // The protocol this version's client speaks
    auto protocol() const -> const Protocol&
    {
        return *protocol_;
    }

    // Version strings and 0x00A Ver values this profile serves (its release's)
    auto versions() const -> const std::vector<std::string>&
    {
        return release_->versions;
    }

    auto vers() const -> const std::vector<uint32>&
    {
        return release_->vers;
    }

    // Does the client have a handler for this s2c id? (its protocol's)
    auto clientHandles(uint16 id) const -> bool
    {
        return protocol_->clientHandles(id);
    }

    // The version's id maps (res/compatibility/<version>/), loaded on first use
    auto maps() const -> const IdMaps&;

    auto translateS2C(MapSession* PSession, CBasicPacket& packet) const -> Result;
    auto translateC2S(MapSession* PSession, CBasicPacket& packet) const -> Result;

private:
    const Release*                  release_;
    const Protocol*                 protocol_;
    mutable std::unique_ptr<IdMaps> maps_;
};

auto profiles() -> const std::vector<std::unique_ptr<Profile>>&;
auto findProfile(Platform platform, uint32 ver, const std::string& version = {}) -> const Profile*;
auto active() -> const Profile&;

auto zoneAvailable(const MapSession* PSession, uint16 zoneId) -> bool;

auto translateS2C(MapSession* PSession, CBasicPacket& packet) -> Result;
auto translateC2S(MapSession* PSession, CBasicPacket& packet) -> Result;

auto tracing() -> bool;
void emitS2C(const uint8* packet, size_t size);
auto takeEmittedS2C() -> std::vector<std::vector<uint8>>;

} // namespace compatibility
