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

#ifndef TOTAL_POWER_H_INCLUDED
#define TOTAL_POWER_H_INCLUDED

#include <string>
#include <vector>
#include <functional>

/*
 * \brief For the specified asset finds out the devices
 *        that are used for total power computation
 * *
 * \param[in] assetName - name of the asset, for which we need to
 *                      determine devices for total power calculation
 * \param[out] powerDevices - list of devices for the specified asset
 *                      used for total power computation.
 *                      It's content would be cleared every time
 *                      at the beginning.
 *
 * \return  0 - in case of success
 *         -1 - in case of internal error
 *         -2 - in case the requested asset was not found
 */
FTY_ASSET_PRIVATE int
    select_devices_total_power(
        const std::string &assetName,
        std::vector<std::string> &powerDevices
    );

FTY_ASSET_PRIVATE int
    select_assets_by_container (
        const std::string& container_name,
        const std::set <std::string>& filter,
        std::vector <std::string>& assets
    );

FTY_ASSET_PRIVATE int
    select_assets_by_filter (
        const std::set <std::string>& filter,
        std::vector <std::string>& assets
    );

FTY_ASSET_PRIVATE int
    select_ext_attributes
        (uint32_t asset_id,
         std::function<void(const tntdb::Row&)> cb);

FTY_ASSET_PRIVATE int
    select_asset_element_basic
        (const std::string &asset_name,
         std::function<void(const tntdb::Row&)> cb);

FTY_ASSET_PRIVATE int
    select_asset_element_super_parent (
        uint32_t id,
        std::function<void(const tntdb::Row&)>& cb);

FTY_ASSET_PRIVATE int
    select_assets (
            std::function<void(
                const tntdb::Row&
                )>& cb);

// update inventory data of asset in database
// returns -1 for database failure, otherwise 0
FTY_ASSET_PRIVATE int
process_insert_inventory (
    const std::string& device_name,
    zhash_t *ext_attributes);

//  Self test of this class
FTY_ASSET_PRIVATE void
    total_power_test (bool verbose);


#endif
