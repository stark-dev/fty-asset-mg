#include "asset.h"
#include "asset-db.h"

namespace fty {

//============================================================================================================

static constexpr const char* RC0_INAME = "rackcontroller-0";

//============================================================================================================

template <typename T, typename T1>
bool isAnyOf(const T& val, const T1& other)
{
    return val == other;
}

template <typename T, typename T1, typename... Vals>
bool isAnyOf(const T& val, const T1& other, const Vals&... args)
{
    if (val == other) {
        return true;
    } else {
        return isAnyOf(val, args...);
    }
}

//============================================================================================================

AssetImpl::AssetImpl(const std::string& nameId):
    m_db(new DB)
{
    m_db->loadAsset(nameId, *this);
    m_db->loadExtMap(*this);
    m_db->loadChildren(*this);
    m_db->loadLinkedAssets(*this);
}

bool AssetImpl::hasLogicalAsset() const
{
    return getExt().find("logical_asset") != getExt().end();
}

void AssetImpl::remove(bool recursive)
{
    if (RC0_INAME == getInternalName()) {
        throw std::runtime_error("Prevented deleting RC-0");
    }

    if (hasLogicalAsset()) {
        throw std::runtime_error(TRANSLATE_ME("a logical_asset (sensor) refers to it"));
    }

    if (!getLinkedAssets().empty()) {
        throw std::runtime_error(TRANSLATE_ME("can't delete the asset because it is linked to others"));
    }

    if (!recursive && !getChildren().empty()) {
        throw std::runtime_error(
            TRANSLATE_ME("can't delete the asset because it has at least one child"));
    }

    if (recursive) {
        for(const std::string& id: getChildren()) {
            AssetImpl asset(id);
            asset.remove(recursive);
        }
    }

    m_db->beginTransaction();
    try {
        if (isAnyOf(getAssetType(), TYPE_DATACENTER, TYPE_ROW, TYPE_ROOM, TYPE_RACK)) {
            if (m_db->isLastDataCenter(*this)) {
                throw std::runtime_error(TRANSLATE_ME("will not allow last datacenter to be deleted"));
            }
            m_db->removeFromGroups(*this);
            m_db->removeFromRelations(*this);
            m_db->removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_GROUP)) {
            m_db->clearGroup(*this);
            m_db->removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_DEVICE)) {
            m_db->removeFromGroups(*this);
            m_db->unlinkFrom(*this);
            m_db->removeFromRelations(*this);
            m_db->removeAsset(*this);
        } else {
            m_db->removeAsset(*this);
        }
    } catch (const std::exception& e) {
        m_db->rollbackTransaction();
        throw std::runtime_error(e.what());
    }
    m_db->commitTransaction();
}

void AssetImpl::save()
{
    m_db->save(*this);
}

std::vector<std::string> AssetImpl::list()
{
    return {};
}

}

