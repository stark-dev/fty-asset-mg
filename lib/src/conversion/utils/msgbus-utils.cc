/*  =========================================================================
    fty_asset_accessor

    Copyright (C) 2016 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#include "conversion/utils/msgbus-utils.h"

#include <cxxtools/serializationinfo.h>
#include <fty_common.h>
#include <fty_common_messagebus.h>
#include <fty/convert.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#define RECV_TIMEOUT 5  // messagebus request timeout

static constexpr const char *ASSET_AGENT = "asset-agent-ng";
static constexpr const char *ASSET_AGENT_QUEUE = "FTY.Q.ASSET.QUERY";
static constexpr const char *ACCESSOR_NAME = "fty-asset-conversion";
static constexpr const char *ENDPOINT = "ipc://@/malamute";

static messagebus::Message sendCommand(const std::string& command, messagebus::UserData data)
{
    // generate unique ID interface
    std::stringstream ss;
    ss << ACCESSOR_NAME << "-" << std::setfill('0') << std::setw(sizeof(pid_t)*2) << std::hex << std::this_thread::get_id();

    std::string clientName = ss.str();

    std::unique_ptr<messagebus::MessageBus> interface(messagebus::MlmMessageBus(ENDPOINT, clientName));
    messagebus::Message msg;

    interface->connect();

    msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
    msg.metaData().emplace(messagebus::Message::SUBJECT, command);
    msg.metaData().emplace(messagebus::Message::FROM, clientName);
    msg.metaData().emplace(messagebus::Message::TO, ASSET_AGENT);
    msg.metaData().emplace(messagebus::Message::REPLY_TO, clientName);

    msg.userData() = data;

    return interface->request(ASSET_AGENT_QUEUE, msg, RECV_TIMEOUT);
}

fty::Expected<uint32_t> assetInameToID(const std::string &iname)
{
    messagebus::Message ret;

    try
    {
        ret = sendCommand("GET_ID", {iname});
    }
    catch (messagebus::MessageBusException &e)
    {
        return fty::unexpected("MessageBus request failed: {}", e.what());
    }

    if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK)
    {
        return fty::unexpected("Request of ID from iname failed");
    }

    cxxtools::SerializationInfo si;
    JSON::readFromString(ret.userData().front(), si);
    std::string data;

    si >>= data;

    return fty::convert<uint32_t>(data);
}

fty::Expected<std::string> assetIDToIname(const uint32_t id)
{
    messagebus::Message ret;

    try
    {
        ret = sendCommand("GET_INAME", {fty::convert<std::string>(id)});
    }
    catch (messagebus::MessageBusException &e)
    {
        return fty::unexpected("MessageBus request failed: {}", e.what());
    }

    if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK)
    {
        return fty::unexpected("Request of iname from ID failed");
    }

    cxxtools::SerializationInfo si;
    JSON::readFromString(ret.userData().front(), si);
    std::string data;

    si >>= data;

    return data;
}
