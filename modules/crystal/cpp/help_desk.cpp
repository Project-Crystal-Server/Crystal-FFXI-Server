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

#include "map/entities/char_entity.h"
#include "map/enums/chat_message_type.h"
#include "map/packets/basic.h"
#include "map/packets/c2s/0x0d3_faq_gmcall.h"
#include "map/packets/c2s/0x0d4_faq_gmparam.h"
#include "map/packets/s2c/0x017_chat_std.h"
#include "map/utils/moduleutils.h"

// In-game GM calls are turned off. Players who open the help desk or send a GM call
// are pointed at the support channel on the Crystal FFXI Discord instead.
class CrystalHelpDesk : public CPPModule
{
    void OnInit() override
    {
    }

    auto OnIncomingPacket(MapSession* PSession, CCharEntity* PChar, CBasicPacket& data) -> bool override
    {
        // Opening the help desk menu: show the notice and let the server answer as usual.
        if (data.getType() == static_cast<uint16>(GP_CLI_COMMAND_FAQ_GMPARAM::packetId))
        {
            sendNotice(PChar);
            return false;
        }

        if (data.getType() != static_cast<uint16>(GP_CLI_COMMAND_FAQ_GMCALL::packetId))
        {
            return false;
        }

        const auto* packet = data.as<GP_CLI_COMMAND_FAQ_GMCALL>();
        if (static_cast<GP_CLI_COMMAND_FAQ_GMCALL_TYPE>(packet->type) != GP_CLI_COMMAND_FAQ_GMCALL_TYPE::GMCall)
        {
            return false;
        }

        // A GM call spans several packets. Drop all of them and show the notice after the last one.
        if (packet->eos == 1)
        {
            sendNotice(PChar);
        }

        return true;
    }

    static void sendNotice(CCharEntity* PChar)
    {
        PChar->pushPacket<GP_SERV_COMMAND_CHAT_STD>(PChar, MESSAGE_SYSTEM_3, "-------------------------------");
        PChar->pushPacket<GP_SERV_COMMAND_CHAT_STD>(PChar, MESSAGE_SYSTEM_3, "In game GM tickets have been disabled.");
        PChar->pushPacket<GP_SERV_COMMAND_CHAT_STD>(PChar, MESSAGE_SYSTEM_3, "For support, please ask in the Crystal FFXI Discord:");
        PChar->pushPacket<GP_SERV_COMMAND_CHAT_STD>(PChar, MESSAGE_SYSTEM_3, "https://discord.com/channels/1256239459922608228/1558819497274638398");
        PChar->pushPacket<GP_SERV_COMMAND_CHAT_STD>(PChar, MESSAGE_SYSTEM_3, "-------------------------------");
    }
};

REGISTER_CPP_MODULE(CrystalHelpDesk);
