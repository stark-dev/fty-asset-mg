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

/*
@header
    topology_power_persist_dbhelpers2 -
@discuss
@end
*/

#include "dbhelpers2.h"

#include <assert.h>

#include <czmq.h>
#include <tntdb/connect.h>
#include <tntdb/row.h>
#include <tntdb/error.h>

#include <fty_common_db_dbpath.h>
#include <fty_common_db.h>
#include <fty_common.h>
#include <fty_log.h>

//TODO: used only in tests for legacy autodiscovery - should proably be removed
int convert_asset_to_monitor_safe(const char* url,
                a_elmnt_id_t asset_element_id, m_dvc_id_t *device_id)
{
    if ( device_id == NULL )
        return -5;
    try
    {
        *device_id = convert_asset_to_monitor_old(url, asset_element_id);
        return 0;
    }
    catch (const bios::NotFound &e){
        return -1;
    }
    catch (const bios::InternalDBError &e) {
        return -2;
    }
    catch (const bios::ElementIsNotDevice &e) {
        return -3;
    }
    catch (const bios::MonitorCounterpartNotFound &e) {
        return -4;
    }
}


//TODO: used only in tests for legacy autodiscovery - should proably be removed
m_dvc_id_t convert_asset_to_monitor_old(const char* url,
                a_elmnt_id_t asset_element_id)
{
    assert ( asset_element_id );
    m_dvc_id_t       device_discovered_id = 0;
    a_elmnt_tp_id_t  element_type_id      = 0;
    try{
        tntdb::Connection conn = tntdb::connect(url);

        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id_discovered_device, v1.id_type"
            " FROM"
            "   v_bios_monitor_asset_relation v"
            " LEFT JOIN t_bios_asset_element v1"
            "   ON (v1.id_asset_element = v.id_asset_element)"
            " WHERE v.id_asset_element = :id"
        );

        tntdb::Row row = st.set("id", asset_element_id).
                            selectRow();

        row[0].get(device_discovered_id);

        row[1].get(element_type_id);
        assert (element_type_id);
    }
    catch (const tntdb::NotFound &e){
        // apropriate asset element was not found
        log_info("end: asset element %" PRIu32 " notfound", asset_element_id);
        throw bios::NotFound();
    }
    catch (const std::exception &e) {
        log_warning("end: abnormal with '%s'", e.what());
        throw bios::InternalDBError(e.what());
    }
    if ( element_type_id != persist::asset_type::DEVICE )
    {
        log_info("end: specified element is not a device");
        throw bios::ElementIsNotDevice();
    }
    else if ( device_discovered_id == 0 )
    {
        log_warning("end: monitor counterpart for the %" PRIu32 " was not found",
                                                asset_element_id);
        throw bios::MonitorCounterpartNotFound ();
    }
    log_info("end: asset element %" PRIu32 " converted to %" PRIu32, asset_element_id, device_discovered_id);
    return device_discovered_id;
}

//TODO: used only in tests for legacy autodiscovery - should proably be removed
int convert_monitor_to_asset_safe(const char* url,
                    m_dvc_id_t discovered_device_id, a_elmnt_id_t *asset_element_id)
{
    if ( asset_element_id == NULL )
        return -5;
    try
    {
        *asset_element_id = convert_monitor_to_asset (url, discovered_device_id);
        return 0;
    }
    catch (const bios::NotFound &e){
        return -1;
    }
    catch (const bios::InternalDBError &e) {
        return -2;
    }
}


//TODO: used only in tests for legacy autodiscovery - should proably be removed
a_elmnt_id_t convert_monitor_to_asset(const char* url,
                    m_dvc_id_t discovered_device_id)
{
    log_info("start");
    assert ( discovered_device_id );
    a_elmnt_id_t asset_element_id = 0;
    try{
        tntdb::Connection conn = tntdb::connect(url);
        tntdb::Statement st = conn.prepare(
            " SELECT"
            " id_asset_element"
            " FROM"
            " v_bios_monitor_asset_relation"
            " WHERE id_discovered_device = :id"
        );

        tntdb::Value val = st.set("id", discovered_device_id).
                              selectValue();

        val.get(asset_element_id);
    }
    catch (const tntdb::NotFound &e){
        // apropriate asset element was not found
        log_warning("end: asset counterpart for the %" PRIu32 " was not found",
                                                discovered_device_id);
        throw bios::NotFound();
    }
    catch (const std::exception &e) {
        log_warning("end: abnormal with '%s'", e.what());
        throw bios::InternalDBError(e.what());
    }
    log_info("end: monitor device %" PRIu32 " converted to %" PRIu32, discovered_device_id, asset_element_id);
    return asset_element_id;
}


