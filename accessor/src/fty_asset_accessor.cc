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

#include "fty_asset_accessor.h"

#include <fty_common_messagebus.h>
#include <fty/convert.h>
#include <iostream>

#define RECV_TIMEOUT 5  // messagebus request timeout

namespace fty
{
    static constexpr const char *ASSET_AGENT = "asset-agent-ng";
    static constexpr const char *ASSET_AGENT_QUEUE = "FTY.Q.ASSET.QUERY";
    static constexpr const char *ACCESSOR_NAME = "fty-asset-accessor";
    static constexpr const char *ENDPOINT = "ipc://@/malamute";

    fty::Expected<uint32_t> AssetAccessor::assetInameToID(const std::string &iname)
    {

        std::unique_ptr<messagebus::MessageBus> interface(messagebus::MlmMessageBus(ENDPOINT, ACCESSOR_NAME));
        messagebus::Message msg;

        interface->connect();

        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        msg.metaData().emplace(messagebus::Message::SUBJECT, "GET_ID");
        msg.metaData().emplace(messagebus::Message::FROM, ACCESSOR_NAME);
        msg.metaData().emplace(messagebus::Message::TO, ASSET_AGENT);
        msg.metaData().emplace(messagebus::Message::REPLY_TO, ACCESSOR_NAME);

        msg.userData().push_back(iname);

        messagebus::Message ret;

        try
        {
            ret = interface->request(ASSET_AGENT_QUEUE, msg, RECV_TIMEOUT);
        }
        catch (messagebus::MessageBusException &e)
        {
            return fty::unexpected("MessageBus request failed: {}", e.what());
        }

        if (ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK)
        {
            return fty::unexpected("Request of ID from iname failed");
        }

        return fty::convert<uint32_t>(ret.userData().front());
    }

} // namespace fty
