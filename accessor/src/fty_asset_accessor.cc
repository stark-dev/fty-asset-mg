#include "fty_asset_accessor.h"

#include <fty_common_messagebus.h>
#include <fty_log.h>
#include <iostream>

#define RECV_TIMEOUT 10

namespace fty
{
    static constexpr const char *ASSET_AGENT = "asset-agent-ng";
    static constexpr const char *ASSET_AGENT_QUEUE = "FTY.Q.ASSET.QUERY";
    static constexpr const char *ACCESSOR_NAME = "fty-asset-accessor";
    static constexpr const char *ENDPOINT = "ipc://@/malamute";

    AssetAccessor::AssetAccessor()
    {
    }

    AssetAccessor::~AssetAccessor()
    {
    }

    uint32_t AssetAccessor::assetInameToID(const std::string &iname)
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

        messagebus::Message ret = interface->request("FTY.Q.ASSET.QUERY", msg, 10);

        log_debug("Received: %s", ret.userData().front().c_str());

        if(ret.metaData().at(messagebus::Message::STATUS) != messagebus::STATUS_OK) {
            return 0;
        }
        return static_cast<uint32_t>(std::stoul(ret.userData().front()));
    }

} // namespace fty
