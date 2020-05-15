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
    bool isLastDataCenter(Asset& asset);

    void beginTransaction();
    void rollbackTransaction();
    void commitTransaction();

    void save(Asset& asset);

private:
    mutable tntdb::Connection m_conn;
};

} // namespace fty
