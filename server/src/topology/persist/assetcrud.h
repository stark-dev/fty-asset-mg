/*  =========================================================================
    topology_power_persist_assetcrud - class description

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

#ifndef TOPOLOGY_POWER_PERSIST_ASSETCRUD_H_INCLUDED
#define TOPOLOGY_POWER_PERSIST_ASSETCRUD_H_INCLUDED

#include <tntdb/connect.h>
#include <fty_common_db_asset.h>

#include "dbtypes.h"

// ===============================================================
// Helper functions for direct interacting with database
// ===============================================================


std::set <a_elmnt_id_t> select_asset_group_elements (tntdb::Connection &conn, a_elmnt_id_t group_id);


zlist_t* select_asset_device_links_all(tntdb::Connection &conn,
                a_elmnt_id_t device_id, a_lnk_tp_id_t link_type_id);

db_reply <db_a_elmnt_t>
    select_asset_element_by_name
        (tntdb::Connection &conn,
         const char *element_name);


// dictionaries

/**
 * \brief Reads from database all available element types.
 *
 * Reads from the table t_bios_asset_element_type;
 *
 * \param[in] conn - the connection to database.
 *
 * \return a database reply where item is a map of names at the ids.
 *         In case of any erorrs item would be empty.
 */
db_reply < std::map <std::string, int> >
    get_dictionary_element_type
        (tntdb::Connection &conn);


/**
 * \brief Reads from database all available device types.
 *
 * Reads from the table t_bios_asset_device_type;
 *
 * \param[in] conn - the connection to database.
 *
 * \return a database reply where item is a map of names at the ids.
 *         In case of any erorrs item would be empty.
 */
db_reply < std::map <std::string, int> >
    get_dictionary_device_type
        (tntdb::Connection &conn);

db_reply <std::vector<db_a_elmnt_t>>
    select_asset_elements_by_type
        (tntdb::Connection &conn,
         a_elmnt_tp_id_t type_id);

/**
 * \brief Selects all links, where at least one end is inside the container
 */
db_reply <std::set <std::pair<a_elmnt_id_t ,a_elmnt_id_t>>>
    select_links_by_container
        (tntdb::Connection &conn,
         a_elmnt_id_t element_id);


#endif
