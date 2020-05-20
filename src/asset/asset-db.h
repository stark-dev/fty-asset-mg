#pragma once
#include "asset.h"
#include <memory>
#include <tntdb.h>

namespace fty {

class AssetImpl::DB
{
public:
    DB();
    void loadAsset(const std::string& nameId, Asset& asset);

    void loadExtMap(Asset& asset);
    void loadChildren(Asset& asset);
    void loadLinkedAssets(Asset& asset);

    void unlinkFrom(Asset& asset);
    void clearGroup(Asset& asset);
    void removeAsset(Asset& asset);
    void removeFromRelations(Asset& asset);
    void removeFromGroups(Asset& asset);
    void removeExtMap(Asset& asset);
    bool isLastDataCenter(Asset& asset);

    void beginTransaction();
    void rollbackTransaction();
    void commitTransaction();

    void update(Asset& asset);
    void insert(Asset& asset);

    void saveLinkedAssets(Asset& asset);
    void saveExtMap(Asset& asset);
    std::string unameById(uint32_t id);
private:
    mutable tntdb::Connection m_conn;
};

} // namespace fty
