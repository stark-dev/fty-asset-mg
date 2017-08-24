/*  =========================================================================
    dbhelpers - Helpers functions for database

    Copyright (C) 2014 - 2015 Eaton

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

/*
@header
    dbhelpers - Helpers functions for database
@discuss
@end
*/

#include "fty_asset_classes.h"
#define INPUT_POWER_CHAIN     1

/**
 *  \brief Convert asset name to the id
 *
 *  \param[in] conn - a database connection
 *  \param[in] name - name of the asset to convert
 *  \param[out] id - converted id
 *
 *  \return  0 - in case of success
 *          -2 - in case if asset was not found
 *          -1 - in case of some unecpected error
 */
int
    select_asset_id (
        const std::string &name,
        a_elmnt_id_t &id
    )
{
    try {
        tntdb::Connection conn = tntdb::connectCached (url);
        tntdb::Statement st = conn.prepareCached(
            " SELECT "
            "   v.id "
            " FROM "
            "   v_bios_asset_element v "
            " WHERE "
            "   v.name = :name "
        );

        tntdb::Row row = st.set("name", name).
                            selectRow();
        row["id"].get(id);
        return 0;
    }
    catch (const tntdb::NotFound &e) {
        zsys_debug ("Requested element not found");
        id = 0;
        return -2;
    }
    catch (const std::exception& e) {
        zsys_error ("Error: ",e.what());
        id = 0;
        return -1;
    }
}

std::string
select_assets_by_container_filter (
    const std::set<std::string> &types_and_subtypes
)
{
    std::string types, subtypes, filter;

    for (const auto &i: types_and_subtypes) {
        uint32_t t = subtype_to_subtypeid (i);
        if (t != asset_subtype::SUNKNOWN) {
            subtypes +=  "," + std::to_string (t);
        } else {
            t = type_to_typeid (i);
            if (t == asset_type::TUNKNOWN) {
                throw std::invalid_argument ("'" + i + "' is not known type or subtype ");
            }
            types += "," + std::to_string (t);
        }
    }
    if (!types.empty ()) types = types.substr(1);
    if (!subtypes.empty ()) subtypes = subtypes.substr(1);
    if (!types.empty () || !subtypes.empty () ) {
        if (!types.empty ()) {
            filter += " id_type in (" + types + ") ";
            if (!subtypes.empty () ) filter += " OR ";
        }
        if (!subtypes.empty ()) {
            filter += " id_asset_device_type in (" + subtypes + ") ";
        }
    }
    zsys_debug ("filter: '%s'", filter.c_str ());
    return filter;
}

int
    select_links_by_container (
        a_elmnt_id_t element_id,
        std::set <std::pair<a_elmnt_id_t ,a_elmnt_id_t> > &links
    )
{
    links.clear();
    zsys_debug ("  links are selected for element_id = %" PRIi32, element_id);
    uint8_t linktype = INPUT_POWER_CHAIN;

    //      all powerlinks are included into "resultpowers"
    try{
        // v_bios_asset_link are only devices,
        // so there is no need to add more constrains
        tntdb::Connection conn = tntdb::connectCached (url);
        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            "   v.id_asset_element_src,"
            "   v.id_asset_element_dest"
            " FROM"
            "   v_bios_asset_link v,"
            "   v_bios_asset_element_super_parent v1,"
            "   v_bios_asset_element_super_parent v2"
            " WHERE"
            "   v.id_asset_link_type = :linktypeid AND"
            "   v.id_asset_element_dest = v2.id_asset_element AND"
            "   v.id_asset_element_src = v1.id_asset_element AND"
            "   ("
            "       ( :containerid IN (v2.id_parent1, v2.id_parent2 ,v2.id_parent3,"
            "               v2.id_parent4, v2.id_parent5) ) OR"
            "       ( :containerid IN (v1.id_parent1, v1.id_parent2 ,v1.id_parent3,"
            "               v1.id_parent4, v1.id_parent5) )"
            "   )"
        );

        // can return more than one row
        tntdb::Result result = st.set("containerid", element_id).
                                  set("linktypeid", linktype).
                                  select();
        zsys_debug("[t_bios_asset_link]: were selected %" PRIu32 " rows",
                                                         result.size());

        // Go through the selected links
        for ( auto &row: result )
        {
            // id_asset_element_src, required
            a_elmnt_id_t id_asset_element_src = 0;
            row[0].get(id_asset_element_src);
            assert ( id_asset_element_src );

            // id_asset_element_dest, required
            a_elmnt_id_t id_asset_element_dest = 0;
            row[1].get(id_asset_element_dest);
            assert ( id_asset_element_dest );

            links.insert(std::pair<a_elmnt_id_t ,a_elmnt_id_t>(id_asset_element_src, id_asset_element_dest));
        } // end for
        return 0;
    }
    catch (const std::exception &e) {
        zsys_error (e.what());
        return -1;
    }
}

int
    select_assets_by_container_cb (
        a_elmnt_id_t element_id,
        const std::set<std::string> &types_and_subtypes,
        std::function<void(const tntdb::Row&)> cb
    )
{
    zsys_debug ("container element_id = %" PRIu32, element_id);

    try {
        tntdb::Connection conn = tntdb::connectCached (url);
        std::string request =
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v "
            " WHERE "
            "   (:containerid in (v.id_parent1, v.id_parent2, v.id_parent3, v.id_parent4, v.id_parent5) OR :containerid = 0 ) ";

        if(!types_and_subtypes.empty())
            request += " AND ( " + select_assets_by_container_filter (types_and_subtypes) +")";
        zsys_debug("[v_bios_asset_element_super_parent]: %s", request.c_str());

        // Can return more than one row.
        tntdb::Statement st = conn.prepareCached(request);

        tntdb::Result result = st.set("containerid", element_id).
                                  select();
        zsys_debug("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            cb(row);
        }
        return 0;
    }
    catch (const std::exception& e) {
        zsys_error ("Error: ",e.what());
        return -1;
    }
}

int
    select_assets_by_container_cb (
        a_elmnt_id_t element_id,
        std::function<void(const tntdb::Row&)> cb
    )
{

    std::set <std::string> empty;
    return select_assets_by_container_cb (element_id, empty, cb);
}

int
select_assets_by_container (
        const std::string& container_name,
        const std::set <std::string>& filter,
        std::vector <std::string>& assets,
        bool test)
{
    if (test)
        return 0;

    a_elmnt_id_t id = 0;

    // get container asset id
    if (! container_name.empty ()) {
        if (select_asset_id (container_name, id) != 0) {
            return -1;
        }
    }

    std::function<void(const tntdb::Row&)> func =
        [&assets](const tntdb::Row& row)
        {
            std::string name;
            row["name"].get (name);
            assets.push_back (name);
        };

    return select_assets_by_container_cb (id, filter, func);
}

int
    select_asset_element_basic
        (const std::string &asset_name,
         std::function<void(const tntdb::Row&)> cb,
         bool test)
{
    if (test)
        return 0;
    zsys_debug ("asset_name = %s", asset_name.c_str());
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }

    try{
        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            "   v.id, v.name, v.id_type, v.type_name,"
            "   v.subtype_id, v.id_parent,"
            "   v.id_parent_type, v.status,"
            "   v.priority, v.asset_tag, v.parent_name "
            " FROM"
            "   v_web_element v"
            " WHERE :name = v.name"
        );

        tntdb::Row row = st.set("name", asset_name).
                            selectRow();
        zsys_debug("[v_web_element]: were selected %" PRIu32 " rows", 1);

        cb(row);
        return 0;
    }
    catch (const tntdb::NotFound &e) {
        zsys_info ("end: %s", "asset '%s' not found", asset_name.c_str());
        return 0;
    }
    catch (const std::exception &e) {
        zsys_error ("Cannot select basic asset info: %s", e.what());
        return -1;
    }
}

int
    select_ext_attributes
        (uint32_t asset_id,
         std::function<void(const tntdb::Row&)> cb,
         bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }
    try {
        // Can return more than one row
        tntdb::Statement st_extattr = conn.prepareCached(
            " SELECT"
            "   v.keytag, v.value, v.read_only"
            " FROM"
            "   v_bios_asset_ext_attributes v"
            " WHERE v.id_asset_element = :asset_id"
        );

        tntdb::Result result = st_extattr.set("asset_id", asset_id).
                                          select();
        zsys_debug("[v_bios_asset_ext_attributes]: were selected %" PRIu32 " rows", result.size());

        // Go through the selected extra attributes
        for ( const auto &row: result ) {
            cb(row);
        }
        return 0;
    }
    catch (const std::exception &e) {
        zsys_error ("select_ext: %s", e.what());
        return -1;
    }
}

int
    select_asset_element_super_parent (
            uint32_t id,
            std::function<void(
                const tntdb::Row&
                )>& cb,
            bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }
    try{
        tntdb::Statement st = conn.prepareCached(
            " SELECT "
            "   v.id_asset_element as id, "
            "   v.id_parent1 as id_parent1, "
            "   v.id_parent2 as id_parent2, "
            "   v.id_parent3 as id_parent3, "
            "   v.id_parent4 as id_parent4, "
            "   v.id_parent5 as id_parent5, "
            "   v.name_parent1 as parent_name1, "
            "   v.name_parent2 as parent_name2, "
            "   v.name_parent3 as parent_name3, "
            "   v.name_parent4 as parent_name4, "
            "   v.name_parent5 as parent_name5, "
            "   v.name as name, "
            "   v.type_name as type_name, "
            "   v.id_asset_device_type as device_type, "
            "   v.status as status, "
            "   v.asset_tag as asset_tag, "
            "   v.priority as priority, "
            "   v.id_type as id_type "
            " FROM v_bios_asset_element_super_parent v "
            " WHERE "
            "   v.id_asset_element = :id "
            );

        tntdb::Result res = st.set ("id", id).select ();
        zsys_debug("[v_bios_asset_element_super_parent]: were selected %i rows", 1);

        for (const auto& r: res) {
            cb(r);
        }
        return 0;
    }
    catch (const std::exception &e) {
        zsys_error ("[v_bios_asset_element_super_parent]: error '%s'", e.what());
        return -1;
    }
}

int
    select_assets_by_filter_cb (
        const std::set<std::string> &types_and_subtypes,
        std::function<void(const tntdb::Row&)> cb
    )
{
    try {
        tntdb::Connection conn = tntdb::connectCached (url);
        std::string request =
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v ";

        if(!types_and_subtypes.empty())
            request += " WHERE " + select_assets_by_container_filter (types_and_subtypes);
        zsys_debug("[v_bios_asset_element_super_parent]: %s", request.c_str());
        // Can return more than one row.
        tntdb::Statement st = conn.prepareCached(request);
        tntdb::Result result = st.select();
        zsys_debug("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            cb(row);
        }
        return 0;
    }
    catch (const std::exception& e) {
        zsys_error ("Error: ",e.what());
        return -1;
    }
}

int
select_assets_by_filter (
        const std::set <std::string>& filter,
        std::vector <std::string>& assets)
{

    std::function<void(const tntdb::Row&)> func =
        [&assets](const tntdb::Row& row)
        {
            std::string name;
            row["name"].get (name);
            assets.push_back (name);
        };

    return select_assets_by_filter_cb (filter, func);
}

int
    select_assets (
            std::function<void(
                const tntdb::Row&
                )>& cb, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }
    try{
        tntdb::Statement st = conn.prepareCached(
            " SELECT "
            "   v.name, "
            "   v.id,  "
            "   v.id_type,  "
            "   v.id_subtype,  "
            "   v.id_parent,  "
            "   v.parent_name,  "
            "   v.status,  "
            "   v.priority,  "
            "   v.asset_tag  "
            " FROM v_bios_asset_element v "
            );

        tntdb::Result res = st.select ();
        zsys_debug("[v_bios_asset_element]: were selected %zu rows", res.size());

        for (const auto& r: res) {
            cb(r);
        }
        return 0;
    }
    catch (const std::exception &e) {
        zsys_error ("[v_bios_asset_element]: error '%s'", e.what());
        return -1;
    }
}

int
process_insert_inventory
    (const std::string& device_name,
    zhash_t *ext_attributes,
    bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }

    tntdb::Transaction trans (conn);
    tntdb::Statement st = conn.prepareCached (
        " INSERT INTO"
        "   t_bios_asset_ext_attributes"
        "   (keytag, value, id_asset_element, read_only)"
        " VALUES"
        "  ( :keytag, :value, (SELECT id_asset_element FROM t_bios_asset_element WHERE name=:device_name), 1)"
        " ON DUPLICATE KEY"
        "   UPDATE"
        "       value = VALUES (value),"
        "       read_only = 1,"
        "       id_asset_ext_attribute = LAST_INSERT_ID(id_asset_ext_attribute)");

    for (void* it = zhash_first (ext_attributes);
               it != NULL;
               it = zhash_next (ext_attributes)) {

        const char *value = (const char*) it;
        const char *keytag = (const char*)  zhash_cursor (ext_attributes);

        try {
            st.set ("keytag", keytag).
               set ("value", value).
               set ("device_name", device_name).
               execute ();
        }
        catch (const std::exception &e)
        {
            zsys_warning ("%s:\texception on updating %s {%s, %s}\n\t%s", "", device_name.c_str (), keytag, value, e.what ());
            continue;
        }
    }

    trans.commit ();
    return 0;
}

void
dbhelpers_test (bool verbose)
{
    printf (" * dbhelpers: ");

    printf ("OK\n");
}

// ----------------------------------------------------------------------------
// Get datacenter ID if there is just one. Other ways return 0

static uint64_t s_get_datacenter (bool test)
{
    if (test)
        return 0;
    try {
        uint64_t id = 0;

        tntdb::Connection conn = tntdb::connectCached (url);
        tntdb::Statement statement;
        statement = conn.prepareCached (
            " SELECT id_asset_element from t_bios_asset_element where id_type = 2"
        );
        auto result = statement.select();
        if (result.size () != 1) {
            // there is not one DC => do not assigne
            zsys_info ("there is not just one DC. Can't add it automatically.");
            return 0;
        }
        auto row = result[0]; //result.getRow();
        row[0].get (id);
        return id;
    }
    catch (const std::exception &e) {
        zsys_error ("Exception in DC lookup: %s", e.what ());
        return 0;
    }
}

db_reply_t
    create_or_update_asset (fty_proto_t *fmsg, bool test)
{
    const char   *element_name;
    uint64_t      type_id;
    unsigned int  subtype_id;
    uint64_t      parent_id;
    const char   *status;
    unsigned int  priority;
    const char   *operation;
    bool          update;

    type_id = type_to_typeid (fty_proto_aux_string (fmsg, "type", ""));
    subtype_id = subtype_to_subtypeid (fty_proto_aux_string (fmsg, "subtype", ""));
    if (subtype_id == 0) subtype_id = asset_subtype::N_A;
    parent_id = fty_proto_aux_number (fmsg, "parent", 0);
    status = fty_proto_aux_string (fmsg, "status", "nonactive");
    priority = fty_proto_aux_number (fmsg, "priority", 5);
    element_name = fty_proto_name (fmsg);
    // TODO: element name from ext.name?
    operation = fty_proto_operation (fmsg);
    update = streq (operation, "update");

    if (!update) {
        if (streq (element_name,"")) {
            if (type_id == asset_type::DEVICE) {
                element_name = asset_subtype2str (subtype_id);
            } else {
                element_name = asset_type2str (type_id);
            }
        }
        // TODO: sanitize name ("rack controller")
    }
    zsys_debug ("  element_name = '%s'", element_name);

    db_reply_t ret = db_reply_new ();

    // ASSUMPTION: all datacenters are unlocated elements
        if (type_id == asset_type::DATACENTER && parent_id != 0)
        {
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_BADINPUT;
            // bios_error_idx (ret.rowid, ret.msg, "request-param-bad", "location", parent_id, "<nothing for type datacenter>");
            return ret;
        } else {
            if (parent_id == 0) {
                parent_id = s_get_datacenter (test);
            }
        }
        // TODO: check whether asset exists and drop?

    if (test) {
        ret.status = 1;
        return ret;
    }
    try {
        // this concat with last_insert_id may have race condition issue but hopefully is not important
        tntdb::Connection conn = tntdb::connectCached (url);
        tntdb::Statement statement;
        if (update) {
            statement = conn.prepareCached (
                " INSERT INTO t_bios_asset_element "
                " (name, id_type, id_subtype, id_parent, status, priority, asset_tag) "
                " VALUES "
                " (:name, :id_type, :id_subtype, :id_parent, :status, :priority, :asset_tag) "
                " ON DUPLICATE KEY UPDATE name = :name "
            );
        } else {
            // @ is prohibited in name => name-@@-342 is unique
            statement = conn.prepareCached (
                " INSERT INTO t_bios_asset_element "
                " (name, id_type, id_subtype, id_parent, status, priority, asset_tag) "
                " VALUES "
                " (concat (:name, '-@@-', " + std::to_string (rand ())  + "), :id_type, :id_subtype, :id_parent, :status, :priority, :asset_tag) "
            );
        }
        if (parent_id == 0)
        {
            ret.affected_rows = statement.
                set ("name", element_name).
                set ("id_type", type_id).
                set ("id_subtype", subtype_id).
                setNull ("id_parent").
                set ("status", status).
                set ("priority", priority).
                set ("asset_tag", "").
                execute();
        }
        else
        {
            ret.affected_rows = statement.
                set ("name", element_name).
                set ("id_type", type_id).
                set ("id_subtype", subtype_id).
                set ("id_parent", parent_id).
                set ("status", status).
                set ("priority", priority).
                set ("asset_tag", "").
                execute();
        }

        ret.rowid = conn.lastInsertId ();
        zsys_debug ("[t_bios_asset_element]: was inserted %" PRIu64 " rows", ret.affected_rows);
        if (! update) {
            // it is insert, fix the name
            statement = conn.prepareCached (
                " UPDATE t_bios_asset_element "
                "  set name = concat(:name, '-', :id) "
                " WHERE id_asset_element = :id "
            );
            statement.set ("name", element_name).
                set ("id", ret.rowid).
                execute();
            // also set name to fty_proto
            fty_proto_set_name (fmsg, "%s-%" PRIu64, element_name, ret.rowid);
        }
        if (ret.affected_rows == 0) {
            ret.status = 0;
            //TODO: rework to bad param
            // bios_error_idx(ret.rowid, ret.msg, "data-conflict", element_name, "Most likely duplicate entry.");
        }
        else {
            // went well some lines changed
            zsys_debug ("Insert went well, processing inventory.");
            process_insert_inventory (fty_proto_name (fmsg), fty_proto_ext (fmsg), false);
            ret.status = 1;
        }
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        // bios_error_idx(ret.rowid, ret.msg, "internal-error", "Unspecified issue with database.");
        return ret;
    }
}
