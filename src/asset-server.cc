#include "asset-server.h"
#include "fty_asset_classes.h"
#include <fty_common_messagebus.h>
#include <malamute.h>
#include <mlm_client.h>

// REMOVE as soon as old interface is not needed anymore
// fwd declaration
void send_create_or_update_asset(
    const fty::AssetServer& config, const std::string& asset_name, const char* operation, bool read_only);

namespace fty {

// ===========================================================================================================

template <typename T>
struct identify
{
    using type = T;
};

template <typename K, typename V>
static V value(const std::map<K, V>& map, const typename identify<K>::type& key,
    const typename identify<V>::type& def = {})
{
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return def;
}

// ===========================================================================================================

void AssetServer::destroyMlmClient(mlm_client_t* client)
{
    mlm_client_destroy(&client);
}


AssetServer::AssetServer()
    : m_mailboxClient(mlm_client_new(), &destroyMlmClient)
    , m_streamClient(mlm_client_new(), &destroyMlmClient)
{
    m_limitations.max_active_power_devices = -1;
    m_limitations.global_configurability   = 1;
}

void AssetServer::createMailboxClientNg()
{
    m_assetMsgQueue.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg));
    log_debug("New mailbox client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        m_agentNameNg.c_str());
}

void AssetServer::resetMailboxClientNg()
{
    log_debug("Mailbox client %s deleted", m_agentNameNg.c_str());
    m_assetMsgQueue.reset();
}

void AssetServer::connectMailboxClientNg()
{
    m_assetMsgQueue->connect();
}

void AssetServer::receiveMailboxClientNg(const std::string& query)
{
    m_assetMsgQueue->receive(query, [&](messagebus::Message m) {
        this->handleAssetManipulationReq(m);
    });
}

void AssetServer::createPublisherClientNg()
{
    m_publisherCreate.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-create"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-create").c_str());

    m_publisherUpdate.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-update"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-update").c_str());

    m_publisherDelete.reset(messagebus::MlmMessageBus(m_mailboxEndpoint, m_agentNameNg + "-delete"));
    log_debug("New publisher client registered to endpoint %s with name %s", m_mailboxEndpoint.c_str(),
        (m_agentNameNg + "-delete").c_str());
}

void AssetServer::resetPublisherClientNg()
{
    m_publisherCreate.reset();
    m_publisherUpdate.reset();
    m_publisherDelete.reset();
}

void AssetServer::connectPublisherClientNg()
{
    m_publisherCreate->connect();
    m_publisherUpdate->connect();
    m_publisherDelete->connect();
}

// create response (data is a single string)
static messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
    const std::string& from, const std::string& to, const std::string& status, const std::string& data)
{
    messagebus::Message msg;

    if (!subject.empty()) {
        msg.metaData().emplace(messagebus::Message::SUBJECT, subject);
    }
    if (!from.empty()) {
        msg.metaData().emplace(messagebus::Message::FROM, from);
    }
    if (!to.empty()) {
        msg.metaData().emplace(messagebus::Message::TO, to);
    }
    if (!correlationID.empty()) {
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, correlationID);
    }
    if (!status.empty()) {
        msg.metaData().emplace(messagebus::Message::STATUS, status);
    }

    msg.userData().push_back(data);

    return msg;
}

// create response (data is a vector of strings)
static messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
    const std::string& from, const std::string& to, const std::string& status,
    const std::vector<std::string>& data)
{
    messagebus::Message msg;

    if (!subject.empty()) {
        msg.metaData().emplace(messagebus::Message::SUBJECT, subject);
    }
    if (!from.empty()) {
        msg.metaData().emplace(messagebus::Message::FROM, from);
    }
    if (!to.empty()) {
        msg.metaData().emplace(messagebus::Message::TO, to);
    }
    if (!correlationID.empty()) {
        msg.metaData().emplace(messagebus::Message::CORRELATION_ID, correlationID);
    }
    if (!status.empty()) {
        msg.metaData().emplace(messagebus::Message::STATUS, status);
    }

    for (const auto& e : data) {
        msg.userData().push_back(e);
    }

    return msg;
}

// new generation asset manipulation handler
void AssetServer::handleAssetManipulationReq(const messagebus::Message& msg)
{
    // asset manipulation is disabled
    if (m_limitations.global_configurability == 0) {
        throw std::runtime_error("Licensing limitation hit - asset manipulation is prohibited.");
    }

    // clang-format off
    static std::map<std::string, std::function<void(const messagebus::Message&)>> procMap = {
        { FTY_ASSET_SUBJECT_CREATE, [&](const messagebus::Message& msg){ createAsset(msg); } },
        { FTY_ASSET_SUBJECT_UPDATE, [&](const messagebus::Message& msg){ updateAsset(msg); } },
        { FTY_ASSET_SUBJECT_DELETE, [&](const messagebus::Message& msg){ deleteAsset(msg); } },
        { FTY_ASSET_SUBJECT_GET,    [&](const messagebus::Message& msg){ getAsset(msg); } },
        { FTY_ASSET_SUBJECT_LIST,   [&](const messagebus::Message& msg){ listAsset(msg); } },
    };
    // clang-format on

    const std::string& messageSubject = value(msg.metaData(), messagebus::Message::SUBJECT);

    if (procMap.find(messageSubject) != procMap.end()) {
        procMap[messageSubject](msg);
    }

    log_warning("Handle asset manipulation - Unknown subject");
}

// sends create/update/delete notification on both new and old interface
void AssetServer::sendNotification(const messagebus::Message& msg)
{
    const std::string& subject = msg.metaData().at(messagebus::Message::SUBJECT);

    if (subject == FTY_ASSET_SUBJECT_CREATED) {
        m_publisherCreate->publish(FTY_ASSET_TOPIC_CREATED, msg);

        // REMOVE as soon as old interface is not needed anymore
        // old interface
        fty::Asset asset;
        asset = fty::conversion::fromJson(msg.userData().back());
        send_create_or_update_asset(
            *this, asset.getInternalName(), "create", false /* read_only is not used */);
    } else if (subject == FTY_ASSET_SUBJECT_UPDATED) {
        m_publisherUpdate->publish(FTY_ASSET_TOPIC_UPDATED, msg);

        // REMOVE as soon as old interface is not needed anymore
        // old interface
        fty::Asset asset;
        asset = fty::conversion::fromJson(msg.userData().back()); // old interface replies only with updated asset
        send_create_or_update_asset(
            *this, asset.getInternalName(), "update", false /* read_only is not used */);
    } else if (subject == FTY_ASSET_SUBJECT_DELETED) {
        m_publisherDelete->publish(FTY_ASSET_TOPIC_DELETED, msg);
    }
}

void AssetServer::createAsset(const messagebus::Message& msg)
{
    log_debug("[handle asset manipulation] : subject CREATE");

    bool tryActivate = value(msg.metaData(), METADATA_TRY_ACTIVATE) == "true";

    try {
        std::string userData     = msg.userData().front();
        fty::Asset  asset        = fty::conversion::fromJson(userData);
        fty::Asset  createdAsset = ::createAsset(asset, tryActivate, m_testMode);

        auto response = createMessage(FTY_ASSET_SUBJECT_CREATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(createdAsset));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        // send notification
        messagebus::Message notification = createMessage(
            FTY_ASSET_SUBJECT_CREATED, "", m_agentNameNg, "", messagebus::STATUS_OK, fty::conversion::toJson(createdAsset));
        sendNotification(notification);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_CREATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

void AssetServer::updateAsset(const messagebus::Message& msg)
{
    log_debug("[handle asset manipulation] : subject UPDATE");

    bool tryActivate = value(msg.metaData(), METADATA_TRY_ACTIVATE) == "true";

    try {
        std::string userData       = msg.userData().front();
        fty::Asset  requestedAsset = fty::conversion::fromJson(userData);

        // data vector (contains asset before and after update)
        std::vector<std::string> assetJsonVector;
        // before update
        assetJsonVector.push_back(fty::conversion::toJson(getAssetFromDB(requestedAsset.getInternalName())));

        fty::Asset updatedAsset = ::updateAsset(requestedAsset, tryActivate, m_testMode);
        // after update
        assetJsonVector.push_back(fty::conversion::toJson(updatedAsset));

        // create response (ok)
        auto response = createMessage(FTY_ASSET_SUBJECT_UPDATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK,
            fty::conversion::toJson(updatedAsset));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);

        // send notification
        messagebus::Message notification = createMessage(
            FTY_ASSET_SUBJECT_UPDATED, "", m_agentNameNg, "", messagebus::STATUS_OK, assetJsonVector);
        sendNotification(notification);
    } catch (const std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_UPDATE,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

void AssetServer::deleteAsset(const messagebus::Message& msg)
{
    messagebus::Message response;
    try {
        std::istringstream input(msg.userData().front());

        cxxtools::SerializationInfo si;
        cxxtools::JsonDeserializer  deserializer(input);

        deserializer.deserialize(si);

        std::string assetIname;
        si.getMember("id").getValue(assetIname);
        fty::Asset asset = ::getAsset(assetIname, m_testMode);

        // fty::deleteAsset(asset, value(msg.metaData(), "RECURSIVE") == "YES");

        response = createMessage(value(msg.metaData(), messagebus::Message::SUBJECT),
            value(msg.metaData(), messagebus::Message::CORRELATION_ID), m_agentNameNg,
            value(msg.metaData(), messagebus::Message::FROM), messagebus::STATUS_OK, "");

    } catch (const std::exception& e) {
        response = createMessage(value(msg.metaData(), messagebus::Message::SUBJECT),
            value(msg.metaData(), messagebus::Message::CORRELATION_ID), m_agentNameNg,
            value(msg.metaData(), messagebus::Message::FROM), messagebus::STATUS_KO, e.what());
    }

    m_assetMsgQueue->sendReply(value(msg.metaData(), messagebus::Message::REPLY_TO), response);
}

void AssetServer::getAsset(const messagebus::Message& msg)
{
    log_debug("[handle asset manipulation] : subject GET");

    try {
        std::string assetIname = msg.userData().front();
        fty::Asset  asset      = ::getAsset(assetIname, m_testMode);

        // create response (ok)
        auto response = createMessage(FTY_ASSET_SUBJECT_GET,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK, fty::conversion::toJson(asset));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_GET,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}

void AssetServer::listAsset(const messagebus::Message& msg)
{
    log_debug("[handle asset manipulation] : subject LIST");

    try {
        std::vector<fty::Asset> assetVector = listAssets(m_testMode);

        std::vector<std::string> jsonVector;

        for (const fty::Asset& asset : assetVector) {
            jsonVector.push_back(fty::conversion::toJson(asset));
        }

        // create response (ok)
        auto response = createMessage(FTY_ASSET_SUBJECT_LIST,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_OK, jsonVector);

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } catch (std::exception& e) {
        log_error(e.what());
        // create response (error)
        auto response = createMessage(FTY_ASSET_SUBJECT_LIST,
            msg.metaData().find(messagebus::Message::CORRELATION_ID)->second, m_agentNameNg,
            msg.metaData().find(messagebus::Message::FROM)->second, messagebus::STATUS_KO,
            TRANSLATE_ME(e.what()));

        // send response
        log_debug("[handle asset manipulation] : sending response to %s",
            msg.metaData().find(messagebus::Message::FROM)->second.c_str());
        m_assetMsgQueue->sendReply(msg.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    }
}


} // namespace fty
