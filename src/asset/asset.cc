#include "asset.h"
#include "asset-db.h"
#include "asset/conversion/full-asset.h"
#include <fty_asset_activator.h>
#include <fty_common_db_dbpath.h>
#include <time.h>
#include <uuid/uuid.h>

#define AGENT_ASSET_ACTIVATOR "etn-licensing-credits"

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
#define TIME_STR_SIZE 100

/// generate current timestamp string in format yyyy-mm-ddThh:MM:ss+0000
static std::string generateCurrentTimestamp()
{
    std::time_t timestamp = std::time(NULL);
    char        timeString[TIME_STR_SIZE];
    std::strftime(timeString, TIME_STR_SIZE - 1, "%FT%T%z", std::localtime(&timestamp));

    return std::string(timeString);
}

/// generate asset UUID (if manufacture, model and serial are known -> EATON namespace UUID, otherwise random)
static std::string generateUUID(
    const std::string& manufacturer, const std::string& model, const std::string& serial)
{
    static std::string ns = "\x93\x3d\x6c\x80\xde\xa9\x8c\x6b\xd1\x11\x8b\x3b\x46\xa1\x81\xf1";
    std::string        uuid;

    if (!manufacturer.empty() && !model.empty() && !serial.empty()) {
        log_debug("generate full UUID");

        std::string src = ns + manufacturer + model + serial;
        // hash must be zeroed first
        unsigned char* hash = (unsigned char*)calloc(SHA_DIGEST_LENGTH, sizeof(unsigned char));

        SHA1((unsigned char*)src.c_str(), src.length(), hash);

        hash[6] &= 0x0F;
        hash[6] |= 0x50;
        hash[8] &= 0x3F;
        hash[8] |= 0x80;

        char uuid_char[37];
        uuid_unparse_lower(hash, uuid_char);

        uuid = uuid_char;
    } else {
        std::cerr << "generate random UUID\n";

        uuid_t u;
        uuid_generate_random(u);

        char uuid_char[37];
        uuid_unparse_lower(u, uuid_char);

        uuid = uuid_char;
    }

    return uuid;
}

//============================================================================================================

AssetImpl::AssetImpl()
    : m_db(new DB)
{
}

AssetImpl::AssetImpl(const std::string& nameId)
    : m_db(new DB)
{
    m_db->loadAsset(nameId, *this);
    m_db->loadExtMap(*this);
    m_db->loadChildren(*this);
    m_db->loadLinkedAssets(*this);
}

AssetImpl::~AssetImpl()
{
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
        throw std::runtime_error(TRANSLATE_ME("can't delete the asset because it has at least one child"));
    }

    if (recursive) {
        for (const std::string& id : getChildren()) {
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
            m_db->removeExtMap(*this);
            m_db->removeFromGroups(*this);
            m_db->removeFromRelations(*this);
            m_db->removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_GROUP)) {
            m_db->clearGroup(*this);
            m_db->removeExtMap(*this);
            m_db->removeAsset(*this);
        } else if (isAnyOf(getAssetType(), TYPE_DEVICE)) {
            m_db->removeFromGroups(*this);
            m_db->unlinkFrom(*this);
            m_db->removeFromRelations(*this);
            m_db->removeExtMap(*this);
            m_db->removeAsset(*this);
        } else {
            m_db->removeExtMap(*this);
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
    m_db->beginTransaction();
    try {
        if (getId()) {
            m_db->update(*this);
        } else { // create
            setInternalName(getAssetType() == TYPE_DEVICE ? getAssetSubtype() : getAssetType());
            // set creation timestamp
            setExtEntry(fty::EXT_CREATE_TS, generateCurrentTimestamp(), true);
            // set uuid
            setExtEntry(fty::EXT_UUID, generateUUID(getManufacturer(), getModel(), getSerialNo()), true);

            m_db->insert(*this);
            setInternalName(m_db->unameById(getId()));
        }
        m_db->saveLinkedAssets(*this);
        m_db->saveExtMap(*this);
    } catch (const std::exception& e) {
        m_db->rollbackTransaction();
        throw e.what();
    }
    m_db->commitTransaction();
}

bool AssetImpl::activate()
{
    mlm::MlmSyncClient  client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
    fty::AssetActivator activationAccessor(client);

    // TODO remove as soon as fty::Asset activation is supported
    fty::FullAsset fa = fty::conversion::toFullAsset(*this);

    std::cerr << "checking asset activation\n";
    bool isActivable = activationAccessor.isActivable(fa);

    if (isActivable) {
        activationAccessor.activate(fa);
        std::cerr << "asset activated\n";

        setAssetStatus(fty::AssetStatus::Active);

        return true;
    } else {
        return false;
    }
}

std::vector<std::string> AssetImpl::list()
{
    AssetImpl::DB db;
    return db.listAllAssets();
}

void AssetImpl::reload()
{
    m_db->loadAsset(getInternalName(), *this);
    m_db->loadExtMap(*this);
    m_db->loadChildren(*this);
    m_db->loadLinkedAssets(*this);
}

} // namespace fty
