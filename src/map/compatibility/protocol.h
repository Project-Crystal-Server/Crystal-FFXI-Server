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

#include "common/cbasetypes.h"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

// Protocols: how one generation of clients lays out its packets, as a table of translators.
//
// A protocol holds a translator per packet id in each direction, the s2c ids its client has a handler for
// (anything else is dropped), and passes that run on every packet after the per-id translators.
//
// The base is the legacy protocol (compatibility/protocols/legacy/): every translator exists there. A protocol
// can extend another: it starts as a copy of its base's tables and overrides only what differs.
//
//     auto legacy2013() -> const Protocol&
//     {
//         static const Protocol p = []
//         {
//             Protocol p("legacy-2013", legacy()); // everything legacy does...
//             p.s2c(0x0XX, myLayout);              // ...except this packet, which this protocol lays out its own way
//             p.s2c(0x0YY, nullptr);               // ...and this one, which it sends as today's servers do
//             p.handles({ 0x0ZZ });                // ...and an id its client gained a handler for
//             return p;
//         }();
//         return p;
//     }
//
// A profile (compatibility/profile.h) pairs a client version with the protocol it speaks; which version uses
// which protocol is listed per platform in compatibility/platforms/.

class CBasicPacket;
struct MapSession;

namespace compatibility
{

enum class Result : uint8
{
    Pass,      // send / dispatch as is
    Rewritten, // the translator changed the packet in place (size included)
    Drop,      // do not send / do not dispatch
};

using Translator = Result (*)(MapSession* PSession, CBasicPacket& packet);

auto combine(Result a, Result b) -> Result;

class Protocol
{
public:
    static constexpr uint16 kIdCount = 0x200;

    explicit Protocol(std::string name);

    Protocol(std::string name, const Protocol& base);

    auto name() const -> const std::string&
    {
        return name_;
    }

    auto lineage() const -> const std::vector<std::string>&
    {
        return lineage_;
    }

    auto describe() const -> std::string;

    void s2c(uint16 id, Translator fn);
    void c2s(uint16 id, Translator fn);

    // s2c ids the client has a handler for; unhandles takes one away (a base's included)
    void handles(const uint16* ids, size_t count);
    void handles(std::initializer_list<uint16> ids);
    void unhandles(std::initializer_list<uint16> ids);

    // Passes run in registration order, after the per-id translator, on every packet it kept
    void s2cPass(Translator fn);
    void c2sPass(Translator fn);
    void removeS2CPass(Translator fn);
    void removeC2SPass(Translator fn);

    auto clientHandles(uint16 id) const -> bool;
    auto translateS2C(MapSession* PSession, CBasicPacket& packet) const -> Result;
    auto translateC2S(MapSession* PSession, CBasicPacket& packet) const -> Result;

private:
    std::string                      name_;
    std::vector<std::string>         lineage_;
    std::array<Translator, kIdCount> s2c_{};
    std::array<Translator, kIdCount> c2s_{};
    std::array<bool, kIdCount>       handled_{};
    std::vector<Translator>          s2cPasses_;
    std::vector<Translator>          c2sPasses_;
};

} // namespace compatibility
