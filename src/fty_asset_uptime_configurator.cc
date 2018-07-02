/*  =========================================================================
    fty_asset_uptime_configurator - Configuration for kpi-uptime

    Copyright (C) 2014 - 2017 Eaton

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
    fty_asset_uptime_configurator - Configuration for kpi-uptime
@discuss
@end
*/

#include "fty_asset_classes.h"

#include <tntdb/connect.h>
#include <tntdb/result.h>
#include <tntdb/row.h>
#include <tntdb/error.h>
#include <exception>
#include <functional>

bool
    get_dc_upses
(std::map <std::string, std::vector <std::string>>& dc_upses, std::string &asset_name)
{
    try {
        tntdb::Connection conn = tntdb::connectCached (url);

        std::vector <std::string> container_upses{};
        std::function<void(const tntdb::Row&)> func = \
            [&container_upses](const tntdb::Row& row)
            {
                uint32_t type_id = 0;
                row["type_id"].get(type_id);

                std::string device_type_name = "";
                row["subtype_name"].get(device_type_name);

                if ( ( type_id == 6 ) && ( device_type_name == "ups" ) )
                {
                    std::string device_name = "";
                    row["name"].get(device_name);
                    container_upses.push_back(device_name);
                }
            };

        // select dcs and their IDs
        db_reply <std::map <uint32_t, std::string> > reply =
            select_short_elements (conn, asset_type::DATACENTER, asset_subtype::N_A);
        if (reply.status == 0) {
            zsys_error ("Cannot select datacenters");
            return false;
        }
        // for each DC save its devices (/upses...done by fun)
        for (const auto& dc : reply.item )
        {
            int rv = select_assets_by_container_cb (dc.first, func);
            if (rv != 0) {
                zsys_error ("Cannot read upses for dc with id = '%" PRIu32"'", dc.first);
                continue;
            }
            dc_upses.emplace (dc.second, container_upses);
            container_upses.clear();
        }
        conn.close ();
    }
    catch (const tntdb::Error& e) {
        zsys_error ("Database error: %s", e.what());
        return false;
    }
    catch (const std::exception& e) {
        zsys_error ("Error: %s", e.what());
        return false;
    }
    return true;
}

bool
    insert_upses_to_aux (zhash_t *aux, std::string dc_name)
{
    assert (aux);

    std::map<std::string, std::vector <std::string>> dc_upses;
    get_dc_upses(dc_upses, dc_name);

    for (auto& item : dc_upses)
    {
        int i = 0;
        char key[10];
        for (auto& ups_it : item.second)
        {
            sprintf (key,"ups%d", i);
            zhash_insert (aux, key, (void*) ups_it.c_str());
            i++;
        }
    }
    return true;
}

// helper for testing
bool
    list_zhash_content (zhash_t *zhash)
{
    for (void *it = zhash_first (zhash);
         it != NULL;
	     it = zhash_next (zhash))
    {
        zsys_debug ("--->list: %s\n", (char*) it);
    }
    return true;
}
//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_asset_uptime_configurator_test (bool verbose)
{
    printf (" * fty_asset_uptime_configurator: ");
    //  @end
    std::map <std::string, std::vector <std::string>> dc_upses;
    std::vector <std::string> ups_vector;
    zhash_t *aux = zhash_new ();
    assert(aux);
    ups_vector.push_back("ups_1");
    ups_vector.push_back("ups_2");
    ups_vector.push_back("ups_3");
    dc_upses.emplace ("dc1", ups_vector);
    dc_upses.emplace ("dc2", ups_vector);

    //insert_upses_to_aux (aux, "dc1");
    //assert (zhash_size (aux) == 3);
    printf ("OK\n");
    zhash_destroy (&aux);
 }
