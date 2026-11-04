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

#include "protocol.h"

#include "packets/basic.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <utility>

namespace compatibility
{

auto combine(const Result a, const Result b) -> Result
{
    if (a == Result::Drop || b == Result::Drop)
    {
        return Result::Drop;
    }
    return (a == Result::Rewritten || b == Result::Rewritten) ? Result::Rewritten : Result::Pass;
}

Protocol::Protocol(std::string name)
: name_(std::move(name))
{
}

Protocol::Protocol(std::string name, const Protocol& base)
: Protocol(base)
{
    name_ = std::move(name);
    lineage_.insert(lineage_.begin(), base.name());
}

auto Protocol::describe() const -> std::string
{
    if (lineage_.empty())
    {
        return name_;
    }
    return fmt::format("{} (extends {})", name_, fmt::join(lineage_, " < "));
}

void Protocol::s2c(const uint16 id, const Translator fn)
{
    s2c_[id & 0x1FF] = fn;
}

void Protocol::c2s(const uint16 id, const Translator fn)
{
    c2s_[id & 0x1FF] = fn;
}

void Protocol::handles(const uint16* ids, const size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        handled_[ids[i] & 0x1FF] = true;
    }
}

void Protocol::handles(const std::initializer_list<uint16> ids)
{
    handles(ids.begin(), ids.size());
}

void Protocol::unhandles(const std::initializer_list<uint16> ids)
{
    for (const auto id : ids)
    {
        handled_[id & 0x1FF] = false;
    }
}

void Protocol::s2cPass(const Translator fn)
{
    s2cPasses_.push_back(fn);
}

void Protocol::c2sPass(const Translator fn)
{
    c2sPasses_.push_back(fn);
}

void Protocol::removeS2CPass(const Translator fn)
{
    std::erase(s2cPasses_, fn);
}

void Protocol::removeC2SPass(const Translator fn)
{
    std::erase(c2sPasses_, fn);
}

auto Protocol::clientHandles(const uint16 id) const -> bool
{
    return id < kIdCount && handled_[id];
}

auto Protocol::translateS2C(MapSession* PSession, CBasicPacket& packet) const -> Result
{
    auto result = Result::Pass;

    // A translator may re-id a packet the client knows under another id (0x068 pet sync is the legacy
    // protocol's 0x067 sub-type 4), so it runs before the unhandled-id drop, checked on the result.
    if (const auto fn = s2c_[packet.getType() & 0x1FF])
    {
        result = fn(PSession, packet);
    }

    if (result == Result::Drop || !clientHandles(packet.getType()))
    {
        return Result::Drop;
    }

    for (const auto pass : s2cPasses_)
    {
        result = combine(result, pass(PSession, packet));
        if (result == Result::Drop)
        {
            return Result::Drop;
        }
    }
    return result;
}

auto Protocol::translateC2S(MapSession* PSession, CBasicPacket& packet) const -> Result
{
    auto result = Result::Pass;

    if (const auto fn = c2s_[packet.getType() & 0x1FF])
    {
        result = fn(PSession, packet);
    }

    for (const auto pass : c2sPasses_)
    {
        if (result == Result::Drop)
        {
            return Result::Drop;
        }
        result = combine(result, pass(PSession, packet));
    }
    return result;
}

} // namespace compatibility
