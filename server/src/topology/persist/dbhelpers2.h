/*  =========================================================================
    topology_power_persist_dbhelpers2 - class description

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

#ifndef TOPOLOGY_POWER_PERSIST_DBHELPERS2_H_INCLUDED
#define TOPOLOGY_POWER_PERSIST_DBHELPERS2_H_INCLUDED

#include <functional>
#include <string>
#include <map>
#include <tntdb/row.h>
#include <czmq.h>
#include <vector>
#include <tuple>
#include <string>

#include "persist_error.h"
#include "dbtypes.h"

/**
 * \brief helper structure for results of v_bios_measurement
 */
struct db_msrmnt_t {
    m_msrmnt_id_t id;
    time_t timestamp;
    m_msrmnt_value_t value;
    m_msrmnt_scale_t scale;
    m_msrmnt_id_t device_id;
    std::string units;
    std::string topic;
};

/**
 * \brief A type for storing basic information about device.
 *
 * First  -- id
 *              asset element id of the device in database.
 * Second -- device_name
 *              asset element name of the device in database.
 * Third  -- device_type_name
 *              name of the device type in database.
 * Forth  -- device_type_id
 *              id of the device type in database.
 */
typedef std::tuple< uint32_t, std::string, std::string, uint32_t > device_info_t;

inline uint32_t device_info_id(const device_info_t& d) {
    return std::get<0>(d);
}
inline uint32_t device_info_type_id(const device_info_t& d) {
    return std::get<3>(d);
}
inline std::string device_info_name(const device_info_t& d) {
    return std::get<1>(d);
}
inline std::string device_info_type_name(const device_info_t& d) {
    return std::get<2>(d);
}


/**
 * \brief A type for storing basic information about powerlink.
 *
 * First  -- src_id
 *              asset element id of the source device.
 * Second -- src_out
 *              output port on the source device.
 * Third  -- dest_id
 *              asset element id of the destination device.
 * Forth  -- dest_in
 *              input port on the destination device.
 */
typedef std::tuple< a_elmnt_id_t, std::string, a_elmnt_id_t, std::string > powerlink_info_t;


/**
 * \brief This function looks for a device_discovered in a monitor part
 * which is connected with the specified asset_element in the asset part.
 *
 * Throws exceptions: bios::MonitorCounterpartNotFound
 *                          if apropriate monitor element was not found.
 *                    bios::InternalDBError
 *                          if database error occured.
 *                    bios::ElementIsNotDevice
 *                          if specified element was not a device.
 *                    bios::NotFound
 *                          if specified asset element was not found.
 *
 * \param url              - the connection to database.
 * \param asset_element_id - the id of the asset_element.
 *
 * \return device_discovered_id - of the device connected with the
 *                                asset_element.
 */
m_dvc_id_t convert_asset_to_monitor_old(const char* url,
                a_elmnt_id_t asset_element_id);

// the same as previos. but c-style error handling
int convert_asset_to_monitor_safe_old(const char* url,
                a_elmnt_id_t asset_element_id, m_dvc_id_t *device_id);


/**
 * \brief This function looks for an asset_element in an asset part which
 * is connected with the specified device_discovered in monitor part.
 *
 * Throws exceptions: bios::NotFound
 *                          if asset element was not found.
 *                    bios::InternalDBError
 *                          if database error occured.
 *
 * \param url                  - the connection to database.
 * \param device_discovered_id - the id of the device_discovered.
 *
 * \return asset_element_id - id of the asset_element connected with the
 *                                device_discovered.
 */
a_elmnt_id_t convert_monitor_to_asset(const char* url,
                    m_dvc_id_t discovered_device_id);

int convert_monitor_to_asset_safe(const char* url,
                    m_dvc_id_t discovered_device_id, a_elmnt_id_t *asset_element_id);

#endif
