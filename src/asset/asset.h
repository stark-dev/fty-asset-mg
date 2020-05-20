#pragma once
#include "include/fty_asset_dto.h"

namespace fty {

class AssetImpl : public Asset
{
public:
    AssetImpl();
    AssetImpl(const std::string& nameId);
    ~AssetImpl() override;

    void remove(bool recursive = false);
    bool hasLogicalAsset() const;
    void save(bool test = false);
    void reload();
    bool activate();

    static std::vector<std::string> list();

    using Asset::operator==;

private:
    class DB;
    std::unique_ptr<DB> m_db;
};

} // namespace fty
