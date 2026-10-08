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

#include "packets/compat/profile.h"

// Support for the retail PS2 client (SCUS-97266, patch 20100904_2): profile "ps2-20100904"
// (console_profile.cpp), served to sessions whose 0x00A names sPlatform "PS2" (compat/client.h).
//
// Ground truth for the 2010 layouts is the client itself: the handler for each packet id, decompiled
// from an EE RAM dump (FFXI-PS2 repo: docs/packet-handlers.md, decomp/). The "PS2:" notes in
// packets/s2c and packets/c2s come from XiPackets and describe the final (2016) PS2 client, which is
// newer than this one; where they disagree, the decompiled handler wins.

class CBasicPacket;
struct MapSession;

namespace console
{

using Result     = compat::Result;
using Translator = compat::Translator;
using compat::combine;
using compat::emitS2C;

} // namespace console
