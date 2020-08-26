/*  =========================================================================
    asset_db - asset_db

    Copyright (C) 2014 - 2020 Eaton

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

#include "asset-db.h"
#include "asset.h"
#include <cstdlib>
#include <fty_common_db_dbpath.h>
#include <sstream>
#include <tntdb.h>

namespace fty {

// static helpers

// DB
DB::DB(bool test)
{
    if (!test) {
        m_conn = tntdb::connectCached(DBConn::url);
    }
}

void DB::loadAsset(const std::string& nameId, Asset& asset)
{
    tntdb::Row row;

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            a.id_asset_element AS id,
            a.name             AS name,
            e.name             AS type,
            d.name             AS subType,
            p.name             AS parentName,
            a.status           AS status,
            a.priority         AS priority,
            a.asset_tag        AS tag,
            a.id_secondary     AS idSecondary
        FROM t_bios_asset_element AS a
            INNER JOIN t_bios_asset_device_type AS d
            INNER JOIN t_bios_asset_element_type AS e
            ON a.id_type = e.id_asset_element_type AND a.id_subtype = d.id_asset_device_type
            LEFT JOIN t_bios_asset_element AS p
            ON a.id_parent = p.id_asset_element
        WHERE a.name = :asset_name
    )");
    q.set("asset_name", nameId);
    // clang-format on

    try {
        m_conn_lock.lock();
        row = q.selectRow();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

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
    if (!row.isNull("idSecondary")) {
        asset.setSecondaryID(row.getString("idSecondary"));
    }
}

void DB::loadExtMap(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            keytag,
            value,
            read_only
        FROM
            t_bios_asset_ext_attributes
        WHERE
            id_asset_element = :asset_id
    )");
    // clang-format on
    q.set("asset_id", assetID);

    tntdb::Result res;

    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    for (const auto& row : res) {
        asset.setExtEntry(row.getString("keytag"), row.getString("value"), row.getBool("read_only"));
    }
}

std::vector<std::string> DB::getChildren(const Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            name
        FROM
            t_bios_asset_element
        WHERE
            id_parent = :asset_id
    )");
    // clang-format on
    q.set("asset_id", assetID);

    tntdb::Result res;

    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    std::vector<std::string> children;
    for (const auto& row : res) {
        children.push_back(row.getString("name"));
    }

    return children;
}

// returns 0 if internal name is not found, the integer ID otherwise
uint32_t DB::getID(const std::string& internalName)
{
    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            id_asset_element
        FROM
            t_bios_asset_element
        WHERE
            name = :internal_name
    )");
    // clang-format on
    q.set("internal_name", internalName);

    uint32_t assetID = 0;

    try {
        m_conn_lock.lock();
        auto v = q.selectValue();
        m_conn_lock.unlock();

        assetID = v.getInt32();
    } catch (tntdb::NotFound&) {
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return assetID;
}

uint32_t DB::getTypeID(const std::string& type)
{
    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            id_asset_element_type
        FROM
            t_bios_asset_element_type
        WHERE
            name = :type_name
    )");
    // clang-format on
    q.set("type_name", type);

    uint32_t typeID = 0;

    try {
        m_conn_lock.lock();
        auto v = q.selectValue();
        m_conn_lock.unlock();

        typeID = v.getInt32();
    } catch (tntdb::NotFound&) {
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return typeID;
}
uint32_t DB::getSubtypeID(const std::string& subtype)
{
    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            id_asset_device_type
        FROM
            t_bios_asset_device_type
        WHERE
            name = :subtype_name
    )");
    // clang-format on
    q.set("subtype_name", subtype);

    uint32_t subtypeID = 0;

    try {
        m_conn_lock.lock();
        auto v = q.selectValue();
        m_conn_lock.unlock();

        subtypeID = v.getInt32();
    } catch (tntdb::NotFound&) {
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return subtypeID;
}

void DB::loadLinkedAssets(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            l.id_asset_element_src  AS id,
            e.name                  AS name,
            l.src_out               AS srcOut,
            l.dest_in               AS destIn,
            l.id_asset_link_type    AS linkType
        FROM
            v_bios_asset_link AS l
        INNER JOIN
            t_bios_asset_element AS e ON l.id_asset_element_src = e.id_asset_element
        WHERE
            l.id_asset_element_dest = :asset_id
    )");
    // clang-format on
    q.set("asset_id", assetID);

    tntdb::Result res;

    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    std::vector<AssetLink> links;
    for (const auto& row : res) {
        std::string srcOut, destIn;
        // may be NULL
        if (!row.isNull("srcOut")) {
            row.getString("srcOut", srcOut);
        }
        if (!row.isNull("destIn")) {
            row.getString("destIn", destIn);
        }

        links.push_back(AssetLink(row.getString("name"), srcOut, destIn, row.getInt("linkType")));
    }
    asset.setLinkedAssets(links);
}


bool DB::isLastDataCenter(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepare(R"(
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
    )");
    // clang-format on
    q.set("asset_id", assetID);

    int numDatacentersAfterDelete = -1;

    try {
        m_conn_lock.lock();
        numDatacentersAfterDelete = q.selectValue().getInt();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return numDatacentersAfterDelete == 0;
}

void DB::removeFromGroups(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_group_relation
        WHERE
            id_asset_element = :asset_id
    )");
    // clang-format on
    q.set("asset_id", assetID);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::removeFromRelations(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_monitor_asset_relation
        WHERE
            id_asset_element = :asset_id
    )");
    // clang-format on
    q.set("asset_id", assetID);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error(std::string(e.what()));
    }
}

void DB::removeAsset(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_element
        WHERE
            id_asset_element = :asset_id
    )");
    // clang-format on
    q.set("asset_id", assetID);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::removeExtMap(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_ext_attributes
        WHERE
            id_asset_element = :assetId
    )");
    // clang-format on
    q.set("assetId", assetID);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::clearGroup(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_group_relation
        WHERE
            d_asset_group = :grp
    )");
    // clang-format on
    q.set("grp", assetID);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

bool DB::hasLinkedAssets(const Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    assert(assetID);

    // clang-format off
    auto q = m_conn.prepare(R"(
        SELECT
            COUNT(id_link)
        FROM
            t_bios_asset_link
        WHERE
            id_asset_device_src = :src
    )");
    // clang-format on
    q.set("src", assetID);

    int linkedAssets;
    try {
        m_conn_lock.lock();
        linkedAssets = q.selectValue().getInt();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return linkedAssets != 0;
}

void DB::link(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType)
{
    uint32_t srcID = getID(src.getInternalName());
    assert(srcID);

    uint32_t destID = getID(dest.getInternalName());
    assert(destID);

    // clang-format off
    tntdb::Result res;
    auto q1 = m_conn.prepareCached(R"(
        SELECT
            e.id_asset_element   AS srcId,
            e.name               AS srcName,
            l.src_out            AS srcOut,
            l.dest_in            AS destIn,
            l.id_asset_link_type AS linkType
        FROM
            t_bios_asset_element AS e
        INNER JOIN
            t_bios_asset_link AS l
        ON
            e.id_asset_element = l.id_asset_device_src
        WHERE
             id_asset_device_dest = :assetId
    )");
    // clang-format on
    q1.set("assetId", destID);

    try {
        m_conn_lock.lock();
        res = q1.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    std::vector<AssetLink> existing;
    for (const auto& row : res) {
        std::string tmpOut, tmpIn;

        // may be NULL
        if (!row.isNull("srcOut")) {
            row.getString("srcOut", tmpOut);
        }
        if (!row.isNull("destIn")) {
            row.getString("destIn", tmpIn);
        }

        existing.push_back(AssetLink(row.getString("srcName"), tmpOut, tmpIn, row.getInt("linkType")));
    }

    // new link to insert
    const AssetLink l(src.getInternalName(), srcOut, destIn, linkType);

    auto found = std::find(existing.begin(), existing.end(), l);

    if (found != existing.end()) {
        throw std::logic_error("Link to asset " + src.getInternalName() + " already exists");
    }

    // clang-format off
    auto q2 = m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_link
            (id_asset_device_src, src_out, id_asset_device_dest, dest_in, id_asset_link_type)
        VALUES (
            :src,
            :srcOut,
            :dest,
            :destIn,
            :linkType
        )
    )");
    // clang-format on

    q2.set("src", srcID);
    q2.set("dest", destID);

    srcOut.empty() ? q2.setNull("srcOut") : q2.set("srcOut", srcOut);
    destIn.empty() ? q2.setNull("destIn") : q2.set("destIn", destIn);

    q2.set("linkType", linkType);

    try {
        m_conn_lock.lock();
        q2.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::unlink(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType)
{
    uint32_t srcID = getID(src.getInternalName());
    assert(srcID);

    uint32_t destID = getID(dest.getInternalName());
    assert(destID);

    tntdb::Statement q;

    std::stringstream qs;

    // clang-format off
    qs << 
        " DELETE FROM "                     \
        "    t_bios_asset_link"             \
        " WHERE"                            \
        "    id_asset_device_src = :src"    \
        "    AND"                           \
        "    id_asset_device_dest = :dest";
    if(!srcOut.empty()) {
        qs <<
        "    AND"                   \
        "    src_out = :srcOut";
    }
    if(!destIn.empty()) {
        qs <<
        "    AND"                   \
        "    dest_in = :destIn";
    }
    qs <<
        "    AND" \
        "    id_asset_link_type = :linkType";
    // clang-format on

    q = m_conn.prepareCached(qs.str().c_str());

    q.set("src", srcID);
    q.set("dest", destID);

    if (!srcOut.empty())
        q.set("srcOut", srcOut);
    if (!destIn.empty())
        q.set("destIn", destIn);

    q.set("linkType", linkType);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::unlinkAll(Asset& dest)
{
    uint32_t destID = getID(dest.getInternalName());
    assert(destID);

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_link
        WHERE
            id_asset_device_dest = :dest
    )");
    // clang-format on
    q.set("dest", destID);

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::beginTransaction()
{
    m_conn_lock.lock();
    m_conn.beginTransaction();
    m_conn_lock.unlock();
}

void DB::rollbackTransaction()
{
    m_conn_lock.lock();
    m_conn.rollbackTransaction();
    m_conn_lock.unlock();
}

void DB::commitTransaction()
{
    m_conn_lock.lock();
    m_conn.commitTransaction();
    m_conn_lock.unlock();
}

void DB::update(Asset& asset)
{
    uint32_t parentId = 0;

    // if parent name is not empty, check if it exists
    const std::string& parentIname = asset.getParentIname();
    if (!parentIname.empty()) {
        parentId = getID(parentIname);
        if (parentId == 0) {
            throw std::runtime_error("Could not find parent internal name");
        }
    }

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        UPDATE
            t_bios_asset_element
        SET
            id_type = (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
            id_subtype = (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
            id_parent = :parent_id,
            status = :status,
            priority = :priority,
            asset_tag = :assetTag,
            id_secondary = :idSecondary
        WHERE
            id_asset_element = :assetId
    )");
    // clang-format on
    q.set("assetId", getID(asset.getInternalName()));
    q.set("type", asset.getAssetType());
    q.set("subtype", asset.getAssetSubtype());
    // name field can't be null, parent id is set to NULL if parentIname is empty
    parentId == 0 ? q.setNull("parent_id") : q.set("parent_id", parentId);
    q.set("status", assetStatusToString(asset.getAssetStatus()));
    q.set("priority", asset.getPriority());
    asset.getAssetTag().empty() ? q.setNull("assetTag") : q.set("assetTag", asset.getAssetTag());
    asset.getSecondaryID().empty() ? q.setNull("idSecondary") : q.set("idSecondary", asset.getSecondaryID());

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

void DB::insert(Asset& asset)
{
    uint32_t parentId = 0;

    // if parent name is not empty, check if it exists
    const std::string& parentIname = asset.getParentIname();
    if (!parentIname.empty()) {
        parentId = getID(parentIname);
        if (parentId == 0) {
            throw std::runtime_error("Could not find parent internal name");
        }
    }
    // clang-format off
    auto q = m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_element
            (name, id_type, id_subtype, id_parent, status, priority, asset_tag, id_secondary)
        VALUES (
            :name,
            (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
            (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
            :parent_id,
            :status,
            :priority,
            :asset_tag,
            :idSecondary
        )
    )");
    // clang-format on
    q.set("name", asset.getInternalName());
    q.set("type", asset.getAssetType());
    q.set("subtype", asset.getAssetSubtype());
    // name field can't be null, parent id is set to NULL if parentIname is empty
    parentId == 0 ? q.setNull("parent_id") : q.set("parent_id", parentId);
    // always insert as non active, update after activation
    q.set("status", assetStatusToString(fty::AssetStatus::Nonactive));
    q.set("priority", asset.getPriority());
    asset.getAssetTag().empty() ? q.setNull("assetTag") : q.set("assetTag", asset.getAssetTag());
    asset.getSecondaryID().empty() ? q.setNull("idSecondary") : q.set("idSecondary", asset.getSecondaryID());

    try {
        m_conn_lock.lock();
        q.execute();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }
}

std::string DB::inameById(uint32_t id)
{
    std::string res;

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT name FROM t_bios_asset_element WHERE id_asset_element = :assetId
    )");
    // clang-format on
    q.set("assetId", id);

    try {
        m_conn_lock.lock();
        res = q.selectRow().getString("name");
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return res;
}

std::string DB::inameByUuid(const std::string& uuid)
{
    std::string res;
    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            name
        FROM
            t_bios_asset_element
        WHERE
            id_asset_element = (
                SELECT
                    id_asset_element
                FROM
                    t_bios_asset_ext_attributes
                WHERE
                    keytag = "uuid" AND value = :uuid
            )
        )");
    q.set("uuid", uuid);
    // clang-format on

    try {
        m_conn_lock.lock();
        res = q.selectRow().getString("name");
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    return res;
}

void DB::saveLinkedAssets(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    if (assetID == 0) {
        throw std::runtime_error("Asset " + asset.getInternalName() + " not found");
    }

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            e.id_asset_element   AS srcId,
            e.name               AS srcName,
            l.src_out            AS srcOut,
            l.dest_in            AS destIn,
            l.id_asset_link_type AS linkType
        FROM t_bios_asset_link   AS l
        INNER JOIN
            t_bios_asset_element AS e
            ON e.id_asset_element = l.id_asset_device_src
        WHERE
             id_asset_device_dest = :assetId
    )");
    // clang-format on
    q.set("assetId", assetID);

    tntdb::Result res;
    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    using Existing = std::pair<uint32_t, AssetLink>;

    // get existings links from database
    std::vector<Existing> toRemove;
    for (const auto& row : res) {
        std::string tmpOut, tmpIn;

        // may be NULL
        if (!row.isNull("srcOut")) {
            row.getString("srcOut", tmpOut);
        }
        if (!row.isNull("destIn")) {
            row.getString("destIn", tmpIn);
        }

        toRemove.emplace_back(row.getUnsigned32("srcId"),
            AssetLink(row.getString("srcName"), tmpOut, tmpIn, row.getInt("linkType")));
    }

    // add new links
    for (const AssetLink l : asset.getLinkedAssets()) {
        // delete link from the list of links to remove
        auto found = std::find_if(toRemove.begin(), toRemove.end(), [&](const Existing& e) {
            return e.second == l;
        });

        if (found != toRemove.end()) {
            // link is required, do not remove
            toRemove.erase(found);
        } else {
            // create new link
            AssetImpl src(l.sourceId);
            link(src, l.srcOut, asset, l.destIn, l.linkType);
        }
    }

    // remove links not present in DTO
    for (const auto& entry : toRemove) {
        const AssetLink& l = entry.second;

        // remove all remaining links
        AssetImpl src(l.sourceId);
        unlink(src, l.srcOut, asset, l.destIn, l.linkType);
    }
}

void DB::saveExtMap(Asset& asset)
{
    uint32_t assetID = getID(asset.getInternalName());
    if (assetID == 0) {
        throw std::runtime_error("Asset " + asset.getInternalName() + " not found");
    }

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            id_asset_ext_attribute AS id,
            keytag                 AS akey,
            value                  AS avalue,
            read_only              AS readOnly
        FROM t_bios_asset_ext_attributes
        WHERE
             id_asset_element = : assetId
    )");
    // clang-format on
    q.set("assetId", assetID);

    tntdb::Result res;

    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    using Existing = std::tuple<uint32_t, std::string, std::string, bool>;

    std::vector<Existing> existing;
    for (const auto& row : res) {
        existing.emplace_back(
            row.getUnsigned32("id"), row.getString("akey"), row.getString("avalue"), row.getBool("readOnly"));
    }


    for (const auto& it : asset.getExt()) {
        auto found = std::find_if(existing.begin(), existing.end(), [&](const Existing& e) {
            return std::get<1>(e) == it.first;
        });

        if (found == existing.end()) {
            // clang-format off
            auto q = m_conn.prepareCached(R"(
                INSERT INTO t_bios_asset_ext_attributes (keytag, value, id_asset_element, read_only)
                VALUES (:key, :value, :assetId, :readOnly)
            )");
            // clang-format on
            q.set("key", it.first);
            q.set("value", it.second.first);
            q.set("readOnly", it.second.second);
            q.set("assetId", assetID);
            try {
                m_conn_lock.lock();
                q.execute();
                m_conn_lock.unlock();
            } catch (std::exception& e) {
                m_conn_lock.unlock();
                throw std::runtime_error("database error - " + std::string(e.what()));
            }
        } else {
            if (std::get<2>(*found) != it.second.first || std::get<3>(*found) != it.second.second) {
                // clang-format off
                auto q = m_conn.prepareCached(R"(
                    UPDATE t_bios_asset_ext_attributes
                    SET
                        value = :value,
                        read_only = :readOnly
                    WHERE id_asset_ext_attribute = :extId
                )");
                // clang-format on
                q.set("value", it.second.first);
                q.set("readOnly", it.second.second);
                q.set("extId", std::get<0>(*found));

                try {
                    m_conn_lock.lock();
                    q.execute();
                    m_conn_lock.unlock();
                } catch (std::exception& e) {
                    m_conn_lock.unlock();
                    throw std::runtime_error("database error - " + std::string(e.what()));
                }
            }
            existing.erase(found);
        }
    }

    for (const auto& toRem : existing) {
        // clang-format off
        auto q = m_conn.prepareCached(R"(
            DELETE FROM t_bios_asset_ext_attributes
            WHERE id_asset_ext_attribute = :extId
        )");
        // clang-format on
        q.set("extId", std::get<0>(toRem));

        try {
            m_conn_lock.lock();
            q.execute();
            m_conn_lock.unlock();
        } catch (std::exception& e) {
            m_conn_lock.unlock();
            throw std::runtime_error("database error - " + std::string(e.what()));
        }
    }
}

static void addFilter(
    std::stringstream& qs, const std::string& filter, const std::vector<std::string>& values)
{
    auto value = values.begin();
    qs << " ( ";
    qs << filter << " = " << *value << " ";

    for (auto it = std::next(values.begin()); it != values.end(); it++) {
        qs << " OR ";
        qs << filter << " = " << *it << " ";
    }
    qs << " ) ";
}

std::vector<std::string> DB::listAssets(std::map<std::string, std::vector<std::string>> filters)
{
    std::vector<std::string> assetList;

    tntdb::Statement q;

    std::stringstream qs;

    qs << " SELECT "
          " name AS name "
          " FROM t_bios_asset_element ";

    if (!filters.empty()) {
        qs << " WHERE ";

        // first filter
        auto it     = filters.begin();
        auto filter = it->first;
        auto values = it->second;

        addFilter(qs, filter, values);

        // next filters
        for (auto it = std::next(filters.begin()); it != filters.end(); it++) {
            qs << " AND ";
            auto filter = it->first;
            auto values = it->second;

            addFilter(qs, filter, values);
        }
    }

    q = m_conn.prepareCached(qs.str().c_str());

    tntdb::Result res;

    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    for (const auto& row : res) {
        const std::string& assetName = row.getString("name");
        // discard rackcontroller 0
        if (assetName != RC0) {
            assetList.emplace_back(assetName);
        }
    }

    return assetList;
}

std::vector<std::string> DB::listAllAssets()
{
    std::vector<std::string> assetList;

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        SELECT
            name          AS name
        FROM t_bios_asset_element
    )");
    // clang-format on

    tntdb::Result res;

    try {
        m_conn_lock.lock();
        res = q.select();
        m_conn_lock.unlock();
    } catch (std::exception& e) {
        m_conn_lock.unlock();
        throw std::runtime_error("database error - " + std::string(e.what()));
    }

    for (const auto& row : res) {
        const std::string& assetName = row.getString("name");
        // discard rackcontroller 0
        if (assetName != RC0) {
            assetList.emplace_back(assetName);
        }
    }

    return assetList;
}

} // namespace fty
