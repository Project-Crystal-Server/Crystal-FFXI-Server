/*
===========================================================================

  Copyright (c) 2026 LandSandBoat Dev Teams

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

#include <vector>

// Support for the retail PS2 client (SCUS-97266, patch 20100904_2).
//
// The client names itself with sPlatform "PS2" in its unencrypted 0x00A. For such a session every
// outgoing packet goes through translateS2C (in send_parse) and every incoming one through
// translateC2S (in parse), which rewrite between today's layouts and the 2010 client's.
//
// Ground truth for the 2010 layouts is the client itself: the handler for each packet id, decompiled
// from an EE RAM dump (FFXI-PS2 repo: docs/packet-handlers.md, decomp/). The "PS2:" notes in
// packets/s2c and packets/c2s come from XiPackets and describe the final (2016) PS2 client, which is
// newer than this one; where they disagree, the decompiled handler wins.

class CBasicPacket;
struct MapSession;

namespace ps2
{

enum class Result : uint8
{
    Pass,      // send / dispatch as is
    Rewritten, // the translator changed the packet in place (size included)
    Drop,      // do not send / do not dispatch
};

using Translator = Result (*)(MapSession* PSession, CBasicPacket& packet);

// sPlatform of the unencrypted 0x00A (4 bytes, not terminated when full)
auto isPS2Platform(const uint8* sPlatform) -> bool;

auto isPS2(const MapSession* PSession) -> bool;

// XI_PS2_TRACE=1 in the environment logs every PS2 packet (and resend), for debugging the translation.
auto tracing() -> bool;

// Rewrites one outgoing packet for a PS2 session. Never called for other sessions.
auto translateS2C(MapSession* PSession, CBasicPacket& packet) -> Result;

// Rewrites one incoming packet from a PS2 session before it is dispatched.
auto translateC2S(MapSession* PSession, CBasicPacket& packet) -> Result;

// A translator that has to turn one of today's packets into two for the 2010 client (0x0AC carries
// the traits the 2010 client takes in its own 0x0AB) queues the second one here. send_parse sends it
// right after the translated packet, in the same datagram and with the same sequence number.
// The packet must already be in the 2010 layout (it is not translated again).
void emitS2C(const uint8* packet, size_t size);
auto takeEmittedS2C() -> std::vector<std::vector<uint8>>;

// Used by the group files (ps2_s2c_*.cpp, ps2_c2s_*.cpp) from their register functions.
void registerS2C(uint16 id, Translator fn);
void registerC2S(uint16 id, Translator fn);

// Ids the 2010 client registers a handler for (tables A and B of gcZoneRecv). Anything else is
// dropped: ids >= 0x110 would crash it (table B is indexed without a bound), and unknown ids below
// that only log an error on the client.
auto clientHandles(uint16 id) -> bool;

} // namespace ps2
