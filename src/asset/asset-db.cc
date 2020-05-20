#include "asset-db.h"
#include <fty_common_db_dbpath.h>
#include <tntdb.h>

namespace fty {

AssetImpl::DB::DB()
    : m_conn(tntdb::connectCached(DBConn::url))
{
}

void AssetImpl::DB::loadAsset(const std::string& nameId, Asset& asset)
{
    // clang-format off
    tntdb::Row row = m_conn.prepareCached(R"(
        SELECT
            a.id_asset_element AS id,
            a.name             AS name,
            e.name             AS type,
            d.name             AS subType,
            p.name             AS parentName,
            a.status           AS status,
            a.priority         AS priority,
            a.asset_tag        AS tag
        FROM t_bios_asset_element AS a
            INNER JOIN t_bios_asset_device_type AS d
            INNER JOIN t_bios_asset_element_type AS e
            ON a.id_type = e.id_asset_element_type AND a.id_subtype = d.id_asset_device_type
            LEFT JOIN t_bios_asset_element AS p
            ON a.id_parent = p.id_asset_element
        WHERE a.name = :asset_name
    )")
    .set("asset_name", nameId)
    .selectRow();
    // clang-format on

    asset.setId(row.getInt("id"));
    asset.setInternalName(row.getString("name"));
    asset.setAssetType(row.getString("type"));
    asset.setAssetSubtype(row.getString("subType"));
    if (!row.isNull("parentName")) {
        asset.setParentIname(row.getString("parentName"));
    }
    asset.setAssetStatus(stringToAssetStatus(row.getString("status")));
    asset.setPriority(row.getInt("priority"));
    if (!row.isNull("tag")) {
        asset.setAssetTag(row.getString("tag"));
    }
}

void AssetImpl::DB::loadExtMap(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    auto ext = m_conn.prepareCached(R"(
        SELECT
            keytag,
            value,
            read_only
        FROM
            t_bios_asset_ext_attributes
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .select();
    // clang-format on

    for (const auto& row : ext) {
        asset.setExtEntry(row.getString("keytag"), row.getString("value"), row.getBool("read_only"));
    }
}

void AssetImpl::DB::loadChildren(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    auto result = m_conn.prepareCached(R"(
        SELECT
            name
        FROM
            t_bios_asset_element
        WHERE
            id_parent = :asset_id
    )")
    .set("asset_id", asset.getId())
    .select();
    // clang-format on

    std::vector<std::string> children;
    for (const auto& row : result) {
        children.push_back(row.getValue("name").getString());
    }
    asset.setChildren(children);
}

void AssetImpl::DB::loadLinkedAssets(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    auto result = m_conn.prepareCached(R"(
        SELECT
            l.id_asset_element_dest AS id,
            e.name                  AS name
        FROM
            v_bios_asset_link AS l
        INNER JOIN
            t_bios_asset_element AS e ON l.id_asset_element_dest = e.id_asset_element
        WHERE
            l.id_asset_element_src = :asset_id
    )")
    .set("asset_id", asset.getId())
    .select();
    // clang-format on

    std::vector<std::string> links;
    for (const auto& row : result) {
        links.push_back(row.getString("name"));
    }
    asset.setLinkedAssets(links);
}


bool AssetImpl::DB::isLastDataCenter(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    int numDatacentersAfterDelete = m_conn.prepare(R"(
        SELECT
            COUNT(id_asset_element)
        FROM
            t_bios_asset_element
        WHERE
            id_type = (
                SELECT id_asset_element_type
                FROM   t_bios_asset_element_type
                WHERE  name = 'datacenter'
            )
            AND id_asset_element != :asset_id
    )")
    .set("asset_id", asset.getId())
    .selectValue()
    .getInt();
    // clang-format on

    return numDatacentersAfterDelete == 0;
}

void AssetImpl::DB::removeFromGroups(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_group_relation
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .execute();
    // clang-format on
}

void AssetImpl::DB::removeFromRelations(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_monitor_asset_relation
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .execute();
    // clang-format on
}

void AssetImpl::DB::removeAsset(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_element
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .execute();
    // clang-format on
}

void AssetImpl::DB::clearGroup(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_group_relation
        WHERE
            d_asset_group = :grp
    )")
    .set("grp", asset.getId())
    .execute();
    // clang-format on
}

void AssetImpl::DB::unlinkFrom(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_link
        WHERE
            id_asset_device_dest = :dest
    )")
    .set("dest", asset.getId())
    .execute();
    // clang-format on
}

void AssetImpl::DB::beginTransaction()
{
    m_conn.beginTransaction();
}

void AssetImpl::DB::rollbackTransaction()
{
    m_conn.rollbackTransaction();
}

void AssetImpl::DB::commitTransaction()
{
    m_conn.commitTransaction();
}

void AssetImpl::DB::update(Asset& asset)
{
    // clang-format off
    int affected = m_conn.prepareCached(R"(
        UPDATE
            t_bios_asset_element
        SET
           id_type = (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
           id_subtype = (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
           id_parent = (SELECT id_asset_element from (SELECT * FROM t_bios_asset_element) AS e where e.name = :parent),
            status = :status,
            priority = :priority,
            asset_tag = :assetTag
        WHERE
            id_asset_element = :assetId
    )")
   .set("type", asset.getAssetType())
   .set("subtype", asset.getAssetSubtype())
   .set("parent", asset.getParentIname())
   .set("status", assetStatusToString(asset.getAssetStatus()))
   .set("priority", asset.getPriority())
   .set("assetTag", asset.getAssetTag())
   .set("assetId", asset.getId())
   .execute();
    // clang-format on

    if (affected == 0) {
        throw std::runtime_error(TRANSLATE_ME("updating not existing asset %d", asset.getId()));
    }
}

void AssetImpl::DB::insert(Asset& asset)
{
    // clang-format off
    m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_element
            (name, id_type, id_subtype, id_parent, status, priority, asset_tag)
        VALUES (
            CONCAT(:name, "-", COALESCE((SELECT MAX(e.id_asset_element)+1 FROM t_bios_asset_element as e), '1')),
            (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
            (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
            (SELECT p.id_asset_element FROM t_bios_asset_element AS p WHERE p.name = :parent),
            :status,
            :priority,
            :asset_tag
        )
    )")
    .set("name", asset.getInternalName())
    .set("type", asset.getAssetType())
    .set("subtype", asset.getAssetSubtype())
    .set("parent", asset.getParentIname())
    .set("status", assetStatusToString(asset.getAssetStatus()))
    .set("priority", asset.getPriority())
    .set("assetTag", asset.getAssetTag())
    .set("assetId", asset.getId())
    .execute();
    // clang-format on

    asset.setId(m_conn.lastInsertId());
}

std::string AssetImpl::DB::unameById(uint32_t id)
{
    // clang-format off
    return m_conn.prepareCached(R"(
        SELECT name FROM t_bios_asset_element WHERE id_asset_element = :assetId
    )")
    .set("assetId", id)
    .selectRow()
    .getString("name");
    // clang-format on
}

} // namespace fty
