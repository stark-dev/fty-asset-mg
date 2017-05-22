/*  =========================================================================
    total_power - Calculation of total power for one asset

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
    total_power - Calculation of total power for one asset
@discuss
@end
*/

#include "fty_asset_classes.h"
#include "dbpath.h"
#include <tntdb/connect.h>
#include <tntdb/result.h>
#include <tntdb/error.h>
#include <exception>
#define INPUT_POWER_CHAIN     1

// ----- table:  t_bios_asset_element_type ------------
// ----- column: id_asset_element_type  ---------------
// TODO tntdb can't manage uint8_t, so for now there is
// uint16_t
typedef uint16_t  a_elmnt_tp_id_t;
// ----- table:  t_bios_asset_element -----------------
// ----- column: id_asset_element ---------------------
typedef uint32_t a_elmnt_id_t;
// ----- table:  t_bios_asset_element_type ------------
// ----- column: id_asset_element_type  ---------------
// TODO tntdb can't manage uint8_t, so for now there is
// uint16_t
typedef uint16_t  a_elmnt_stp_id_t;

class ShortAssetInfo {
public:
    a_elmnt_id_t asset_id;
    std::string asset_name;
    a_elmnt_stp_id_t subtype_id;

    ShortAssetInfo (a_elmnt_id_t aasset_id, const std::string &aasset_name, a_elmnt_stp_id_t asubtype_id)
    {
        asset_id = aasset_id;
        asset_name = aasset_name;
        subtype_id = asubtype_id;
    };
};

inline bool operator<(const ShortAssetInfo& lhs, const ShortAssetInfo& rhs)
{
  return lhs.asset_id < rhs.asset_id;
}


/**
 *  \brief A connection string to the database
 
static std::string url =
    std::string("mysql:db=box_utf8;user=") +
    ((getenv("DB_USER")   == NULL) ? "root" : getenv("DB_USER")) +
    ((getenv("DB_PASSWD") == NULL) ? ""     :
    std::string(";password=") + getenv("DB_PASSWD"));
*/


// ============================================================================
// ======   Forward declaration of general asset functions
// TODO move it in future to some general section

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
static int
    select_asset_id (
        tntdb::Connection &conn,
        const std::string &name,
        a_elmnt_id_t &id
    );

/**
 *  \brief Select all assets inside the asset-container (all 5 level down)
 *
 *  \param[in] conn - db connection
 *  \param[in] element_id - id of the asset-container
 *  \param[in] cb - callback to be called with every selected row.
 *
 *   Every selected row has the following columns:
 *       name, asset_id, subtype_id, subtype_name, type_id
 *
 *  \return 0 on success (even if nothing was found)
 */
int
    select_assets_by_container (
        tntdb::Connection &conn,
        a_elmnt_id_t element_id,
        std::function<void(const tntdb::Row&)> cb
    );

/**
 *  \brief Selects all links, where at least one end is inside the specified
 *          container
 *
 *  \param[in] conn - db connection
 *  \param[in] element_id - id of the container
 *  \param[out] links - links, where at least one end belongs to the
 *                  devices in the container
 *
 *  \return 0 on success (even if nothing was found)
 */
int
    select_links_by_container (
        tntdb::Connection &conn,
        a_elmnt_id_t element_id,
        std::set <std::pair<a_elmnt_id_t ,a_elmnt_id_t> > &links
    );

// ======   End of Forward declaration of general asset functions
// ============================================================================




// ============================================================================
// ===== Forward declaration of internal functions used for
// =====               power device selection
// Just to allow us to order functions as we like.

/**
 *  \brief For the specified asset finds out the devices
 *        that are used for total power computation
 *
 *  \param[in] assetId - id of the asset, for which we need to
 *                       determine devices for total power calculation
 *  \param[out] powerDevices - list of devices for the specified asset
 *                       used for total power computation.
 *                       It's content would be cleared every time
 *                       at the beginning.
 *
 *  \return  0 - in case of success
 *          -1 - in case of internal error
 *          -2 - in case the requested asset was not found
 */
static int
    select_total_power_by_id (
        tntdb::Connection &conn,
        a_elmnt_id_t assetId,
        std::vector<std::string> &powerDevices
    );

/**
 *  \brief An implementation of the algorithm.
 *
 *  GOAL: Take the first "smart" devices in every powerchain that are
 *        the closest to "feed".
 *
 *  If the found device is not smart, try to look at upper level. Repeat until
 *  chain ends or until all chains are processed.
 *
 *  Asset container - is an asset for which we want to compute the totl power
 *  Asset container devices - all devices placed in the asset container.
 *
 *  \param[in] devices_in_container - information about all devices in the
 *                          asset container
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *
 *  \return a list of power devices names. It there are no power devices the
 *          list is empty.
 */
static std::vector<std::string>
    total_power_v2 (
        const std::map <a_elmnt_id_t, ShortAssetInfo> &devices_in_container,
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links
    );

/**
 *  \brief Checks if some power device is directly powering devices
 *          in some other racks.
 *
 *  \param[in] device - device to check
 *  \param[in] devices_in_container - information about all devices in the
 *                          asset container
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *
 *  \return true or false
 */
static bool
    is_powering_other_rack (
        const ShortAssetInfo &device,
        const std::map <a_elmnt_id_t, ShortAssetInfo> &devices_in_container,
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links
    );

/**
 *  \brief From old set of border devices generates a new set of border devices
 *
 *  Each device "A" is replaced with the list of devices that were powered by
 *  this device "A".
 *  A border device is a device, that should be taken as a power
 *  source device at first turn.
 *
 *  \param[in] devices_in_container - information about all devices in the
 *                          asset container
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *  \param[in][out] border_devices - list of border devices to be updated
 */
static void
    update_border_devices (
        const std::map <a_elmnt_id_t, ShortAssetInfo> &container_devices,
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links,
        std::set <ShortAssetInfo> &border_devices
    );

/**
 *  \brief For the specified asset it derives a set of powered devices
 *
 *  From set of links derives set of elements that at least once were
 *              in a role of a destination device in a link.
 *  Link is: from to
 *
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *  \param[in] element_id - powering device id
 *
 *  \return a set of powered devices. It can be empty.
 */
static std::set<a_elmnt_id_t>
    find_dests (
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links,
        a_elmnt_id_t element_id
    );

/**
 * \brief Simple wrapper to make code more readable
 */
static bool
    is_ups (
        const ShortAssetInfo &device
    );

/**
 * \brief Simple wrapper to make code more readable
 */
static bool
    is_epdu (
        const ShortAssetInfo &device
    );
// ======   End of Forward declaration of internal functions used for
// =====               power device selection
// ============================================================================


int
    select_devices_total_power(
        const std::string &assetName,
        std::vector<std::string> &powerDevices
    )
{
    tntdb::Connection conn = tntdb::connectCached (url);
    a_elmnt_id_t assetId = 0;
    int rv = select_asset_id (conn, assetName, assetId);
    if ( rv != 0 ) {
        return rv;
    }
    return select_total_power_by_id (conn, assetId, powerDevices);
}


static bool
    is_epdu (
        const ShortAssetInfo &device
    )
{
    if ( device.subtype_id == asset_subtype::EPDU ) {
        return true;
    }
    else {
        return false;
    }
}


static bool
    is_ups (
        const ShortAssetInfo &device
    )
{
    if ( device.subtype_id == asset_subtype::UPS ) {
        return true;
    }
    else {
        return false;
    }
}


static std::set<a_elmnt_id_t>
    find_dests (
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links,
        a_elmnt_id_t element_id
    )
{
    std::set<a_elmnt_id_t> dests;

    for ( auto &one_link: links )
    {
        if ( one_link.first == element_id )
            dests.insert(one_link.second);
    }
    return dests;
}


static void
    update_border_devices (
        const std::map <a_elmnt_id_t, ShortAssetInfo> &container_devices,
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links,
        std::set <ShortAssetInfo> &border_devices
    )
{
    std::set<ShortAssetInfo> new_border_devices;
    for ( auto &border_device: border_devices )
    {
        auto adevice_dests = find_dests (links, border_device.asset_id);
        for ( auto &adevice: adevice_dests )
        {
            auto it = container_devices.find(adevice);
            if ( it != container_devices.cend() )
                new_border_devices.insert(it->second);
            else
            {
                zsys_error ("DB can be in inconsistant state or some device "
                        "has power source in the other container");
                zsys_error ("device(as element) %" PRIu32 " is not in container",
                                                adevice);
                // do nothing in this case
            }
        }
    }
    border_devices.clear();
    border_devices.insert(new_border_devices.begin(),
                          new_border_devices.end());
}


static bool
    is_powering_other_rack (
        const ShortAssetInfo &device,
        const std::map <a_elmnt_id_t, ShortAssetInfo> &devices_in_container,
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links
    )
{
    auto adevice_dests = find_dests (links, device.asset_id);
    for ( auto &adevice: adevice_dests )
    {
        auto it = devices_in_container.find(adevice);
        if ( it == devices_in_container.cend() ) {
            // it means, that destination device is out of the container
            return true;
        }
    }
    return false;
}


static std::vector<std::string>
    total_power_v2 (
        const std::map <a_elmnt_id_t, ShortAssetInfo> &devices_in_container,
        const std::set <std::pair<a_elmnt_id_t, a_elmnt_id_t> > &links
    )
{

    // the set of all border devices ("starting points")
    std::set <ShortAssetInfo> border_devices;
    // the set of all destination devices in selected links
    std::set <a_elmnt_id_t> dest_dvcs{};
    //  from (first)   to (second)
    //           +--------------+
    //  B________|______A__C    |
    //           |              |
    //           +--------------+
    //   B is out of the Container
    //   A is in the Container
    //   then A is border device
    for ( auto &oneLink : links ) {
        zsys_debug ("  cur_link: %d->%d", oneLink.first, oneLink.second);
        auto it = devices_in_container.find (oneLink.first);
        if ( it == devices_in_container.end() )
            // if in the link first point is out of the Container,
            // the second definitely should be in Container,
            // otherwise it is not a "container"-link
        {
            border_devices.insert(
                    devices_in_container.find(oneLink.second)->second);
        }
        dest_dvcs.insert(oneLink.second);
    }
    //  from (first)   to (second)
    //           +-----------+
    //           |A_____C    |
    //           |           |
    //           +-----------+
    //   A is in the Container (from)
    //   C is in the Container (to)
    //   then A is border device
    //
    //   Algorithm: from all devices in the Container we will
    //   select only those that don't have an incoming links
    //   (they are not a destination device for any link)
    for ( auto &oneDevice : devices_in_container ) {
        if ( dest_dvcs.find (oneDevice.first) == dest_dvcs.end() ) {
            border_devices.insert ( oneDevice.second );
        }
    }

    std::vector <std::string> dvc{};
    if ( border_devices.empty() ) {
        return dvc;
    }
    // it is not a good idea to delete from collection while iterating it
    std::set <ShortAssetInfo> todelete{};
    while ( !border_devices.empty() ) {
        for ( auto &border_device: border_devices ) {
            if ( ( is_epdu(border_device) ) ||
                 ( ( is_ups(border_device) ) &&
                   ( !is_powering_other_rack (border_device, devices_in_container, links) ) ) )
            {
                dvc.push_back(border_device.asset_name);
                // remove from border
                todelete.insert(border_device);
                continue;
            }
            // NOT IMPLEMENTED
            //if ( is_it_device(border_device) )
            //{
            //    // remove from border
            //    // add to ipmi
            //}
        }
        for (auto &todel: todelete) {
            border_devices.erase(todel);
        }
        update_border_devices(devices_in_container, links, border_devices);
    }
    return dvc;
}


static int
    select_total_power_by_id (
        tntdb::Connection &conn,
        a_elmnt_id_t assetId,
        std::vector<std::string> &powerDevices
    )
{
    // at the beginning clear
    powerDevices.clear();
    // select all devices in the container
    std::map <a_elmnt_id_t, ShortAssetInfo> container_devices{};
    std::function<void(const tntdb::Row&)> func = \
                [&container_devices](const tntdb::Row& row)
                {
                    a_elmnt_tp_id_t type_id = 0;
                    row["type_id"].get(type_id);

                    if ( type_id == asset_type::DEVICE ) {
                        std::string device_name = "";
                        row["name"].get(device_name);

                        a_elmnt_id_t asset_id = 0;
                        row["asset_id"].get(asset_id);

                        a_elmnt_stp_id_t subtype_id = 0;
                        row["subtype_id"].get(subtype_id);

                        container_devices.emplace (asset_id,
                                ShortAssetInfo(asset_id, device_name,
                                    subtype_id));
                    }
                };

    auto rv = select_assets_by_container (conn, assetId, func);

    // here would be placed names of devices to summ up
    if ( rv != 0 ) {
        zsys_warning ("asset_id='%" PRIu32 "': problems appeared in selecting devices",
                assetId);
        // so return an empty set of power devices
        powerDevices = {};
        return -1;
    }
    if ( container_devices.empty() ) {
        zsys_debug ("asset_id='%" PRIu32 "': has no devices", assetId);
        // so return an empty set of power devices
        powerDevices = {};
        return 0;
    }
    std::set <std::pair<a_elmnt_id_t ,a_elmnt_id_t> > links{};
    rv = select_links_by_container (conn, assetId, links);
    if ( rv != 0 ) {
        zsys_warning ("asset_id='%" PRIu32 "': internal problems in links detecting",
                assetId);
        // so return an empty set of power devices
        powerDevices = {};
        return -1;
    }

    if ( links.empty() ) {
        zsys_debug ("asset_id='%" PRIu32 "': has no power links", assetId);
        // so return an empty set of power devices
        powerDevices = {};
        return 0;
    }

    powerDevices = total_power_v2 (container_devices, links);
    return 0;
}


// GENERAL FUNCTIONS START TODO move in future somewhere
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
        filter = " AND ( ";
        if (!types.empty ()) {
            filter += " id_type in (" + types + ") ";
            if (!subtypes.empty () ) filter += " OR ";
        }
        if (!subtypes.empty ()) {
            filter += " id_asset_device_type in (" + subtypes + ") ";
        }
        filter += " ) ";
    }
    zsys_debug ("filter: '%s'", filter.c_str ());
    return filter;
}


int
    select_assets_by_container (
        tntdb::Connection &conn,
        a_elmnt_id_t element_id,
        const std::set<std::string> &types_and_subtypes,
        std::function<void(const tntdb::Row&)> cb
    )
{
    zsys_debug ("container element_id = %" PRIu32, element_id);

    try {
        // Can return more than one row.
        tntdb::Statement st = conn.prepareCached(
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v "
            " WHERE "
            "   (:containerid in (v.id_parent1, v.id_parent2, v.id_parent3, v.id_parent4, v.id_parent5) OR :containerid = 0 ) "
            + select_assets_by_container_filter (types_and_subtypes)
        );

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
    select_assets_by_container (
        tntdb::Connection &conn,
        a_elmnt_id_t element_id,
        std::function<void(const tntdb::Row&)> cb
    )
{

    std::set <std::string> empty;
    return select_assets_by_container (conn, element_id, empty, cb);
}

int
    select_links_by_container (
        tntdb::Connection &conn,
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


static int
    select_asset_id (
        tntdb::Connection &conn,
        const std::string &name,
        a_elmnt_id_t &id
    )
{
    try {
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

int
select_assets_by_container (
        tntdb::Connection &conn,
        const std::string& container_name,
        const std::set <std::string>& filter,
        std::vector <std::string>& assets)
{
    a_elmnt_id_t id = 0;

    // get container asset id
    if (! container_name.empty ()) {
        if (select_asset_id (conn, container_name, id) != 0) {
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

    return select_assets_by_container (conn, id, filter, func);
}

int
select_assets_by_container (
        const std::string& container_name,
        const std::set <std::string>& filter,
        std::vector <std::string>& assets)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }
    return select_assets_by_container (conn, container_name, filter, assets);
}

int
    select_asset_element_basic
        (const std::string &asset_name,
         std::function<void(const tntdb::Row&)> cb)
{
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
         std::function<void(const tntdb::Row&)> cb)
{
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
                )>& cb)
{
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
    select_assets_by_filter (
        tntdb::Connection &conn,
        const std::set<std::string> &types_and_subtypes,
        std::function<void(const tntdb::Row&)> cb
    )
{
    try {
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
        tntdb::Connection &conn,
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

    return select_assets_by_filter (conn, filter, func);
}

int
select_assets_by_filter (
        const std::set <std::string>& filter,
        std::vector <std::string>& assets)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached (url);
    }
    catch ( const std::exception &e) {
        zsys_error ("DB: cannot connect, %s", e.what());
        return -1;
    }
    return select_assets_by_filter (conn, filter, assets);
}





int
    select_assets (
            std::function<void(
                const tntdb::Row&
                )>& cb)
{
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
process_insert_inventory (
    const std::string& device_name,
    zhash_t *ext_attributes)
{
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
total_power_test (bool verbose)
{
    printf (" * total_power: ");
    printf ("OK\n");
}
