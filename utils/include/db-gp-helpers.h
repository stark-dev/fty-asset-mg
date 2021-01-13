/*  =========================================================================
    dbhelpers - Helpers functions for database

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

#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <fty_common_db.h>
#include <tntdb.h>
#include <tntdb/connect.h>
#include <tntdb/row.h>

/// select one field of an asset from the database
/// SELECT <column> from asset_table where <keyColumn> = <keyValue>
template<typename TypeRet, typename TypeValue>
TypeRet selectAssetProperty(
    const std::string& column,     // column to select
    const std::string& keyColumn,  // key
    const TypeValue& keyValue      // value of key param
)
{
    std::stringstream query;
    query << " SELECT " << column <<
             " FROM t_bios_asset_element " <<
             " WHERE " << keyColumn << " = :value";

    tntdb::Connection conn = tntdb::connectCached (DBConn::url);

    TypeRet obj;

    auto q = conn.prepareCached (query.str());
    q.set ("value", keyValue);

    auto row = q.selectRow();

    row[0].get(obj);

    return obj;
}

/// select one field of an asset from the database
/// SELECT <column> from asset_table where <keyColumn> like '%<keyValue>%'
template<typename TypeRet>
const TypeRet selectAssetPropertyLike(
    const std::string& column,     // column to select
    const std::string& keyColumn,  // key
    const std::string& keyValue    // value of key param
)
{
    std::stringstream query;
    query << " SELECT " << column <<
             " FROM t_bios_asset_element " <<
             " WHERE " << keyColumn << " like '%" << keyValue << "%'";

    tntdb::Connection conn = tntdb::connectCached (DBConn::url);
    tntdb::Statement statement;

    TypeRet obj;

    auto q = conn.prepareCached(query.str());
    auto row = q.selectRow();

    row[0].get(obj);

    return obj;
}

/// update one field of an asset in the database
/// UPDATE in asset_table <updateColumn> to <updateValue> where <keyColumn> = <keyValue>
template<typename TypeValue, typename TypeKey>
void updateAssetProperty(
    const std::string& updateColumn,
    const TypeValue& updateValue,
    const std::string& keyColumn,
    const TypeKey& keyValue
)
{
    std::stringstream query;
    query << " UPDATE t_bios_asset_element " <<
             " SET " << updateColumn << " = :updateValue" <<
             " WHERE " << keyColumn << " = :keyValue";
    
    tntdb::Connection conn = tntdb::connectCached (DBConn::url);
    tntdb::Statement statement;

    statement = conn.prepareCached (query.str());

    statement
        .set("updateValue", updateValue)
        .set("keyValue", keyValue)
        .execute();
}
