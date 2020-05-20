#pragma once
#include "include/fty-asset.h"
#include <memory>

typedef struct _mlm_client_t mlm_client_t;
namespace messagebus {
class MessageBus;
class Message;
} // namespace messagebus

namespace fty {

class AssetServer
{
public:
    AssetServer();
    ~AssetServer() = default;

    bool getTestMode() const
    {
        return m_testMode;
    }

    const std::string& getAgentName() const
    {
        return m_agentName;
    }

    const std::string& getMailboxEndpoint() const
    {
        return m_mailboxEndpoint;
    }

    const std::string& getStreamEndpoint() const
    {
        return m_streamEndpoint;
    }

    const mlm_client_t* getMailboxClient() const
    {
        return m_mailboxClient.get();
    }

    const mlm_client_t* getStreamClient() const
    {
        return m_streamClient.get();
    }

    int getMaxActivePowerDevices() const
    {
        return m_limitations.max_active_power_devices;
    }

    int getGlobalConfigurability() const
    {
        return m_limitations.global_configurability;
    }

    void setTestMode(bool test)
    {
        m_testMode = test;
    }

    void setAgentName(const std::string& name)
    {
        m_agentName = name;
    }

    void setMailboxEndpoint(const std::string& endpoint)
    {
        m_mailboxEndpoint = endpoint;
    }

    void setStreamEndpoint(const std::string& endpoint)
    {
        m_streamEndpoint = endpoint;
    }

    void setMaxActivePowerDevices(int m)
    {
        m_limitations.max_active_power_devices = m;
    }

    void setGlobalConfigurability(int c)
    {
        m_limitations.global_configurability = c;
    }

    // new generation interface
    const std::string& getAgentNameNg() const
    {
        return m_agentNameNg;
    }

    void setAgentNameNg(const std::string& name)
    {
        m_agentNameNg = name;
    }

    void createMailboxClientNg();
    void resetMailboxClientNg();
    void connectMailboxClientNg();
    void receiveMailboxClientNg(const std::string& query);

    void createPublisherClientNg();
    void resetPublisherClientNg();
    void connectPublisherClientNg();

private:
    void createAsset(const messagebus::Message& msg);
    void updateAsset(const messagebus::Message& msg);
    void deleteAsset(const messagebus::Message& msg);
    void getAsset(const messagebus::Message& msg);
    void listAsset(const messagebus::Message& msg);
    void deleteAssetList(const messagebus::Message& msg);

private:
    static void destroyMlmClient(mlm_client_t* client);

    using MlmClientPtr = std::unique_ptr<mlm_client_t, decltype(&AssetServer::destroyMlmClient)>;
    using MsgBusPtr    = std::unique_ptr<messagebus::MessageBus>;


private:
    bool               m_testMode        = false;
    std::string        m_agentName       = "asset-agent";
    std::string        m_mailboxEndpoint = "ipc://@/malamute";
    std::string        m_streamEndpoint  = "ipc://@/malamute";
    LIMITATIONS_STRUCT m_limitations;

    MlmClientPtr m_mailboxClient;
    MlmClientPtr m_streamClient;

    // new generation interface
    std::string m_agentNameNg = "asset-agent-ng";
    MsgBusPtr   m_assetMsgQueue;
    MsgBusPtr   m_publisherCreate;
    MsgBusPtr   m_publisherUpdate;
    MsgBusPtr   m_publisherDelete;

    // topic handlers
    void handleAssetManipulationReq(const messagebus::Message& msg);

    // notifications
    void sendNotification(const messagebus::Message&);
};

} // namespace fty
