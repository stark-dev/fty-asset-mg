/*  =========================================================================
    defs - Defines constants

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

#ifndef DEFS_H_INCLUDED
#define DEFS_H_INCLUDED

//TODO: fix that better - this will work until we'll don't touch the initdb.sql
#define UI_PROPERTIES_CLIENT_ID 5
#define DUMMY_DEVICE_ID 1

typedef struct _LIMITATIONS_STRUCT
{
    int max_active_power_devices;
    int global_configurability;
} LIMITATIONS_STRUCT;

//! Constants for licensing-related errors
enum licensing_err {
    LICENSING_POWER_DEVICES_COUNT_REACHED,
    LICENSING_GLOBAL_CONFIGURABILITY_DISABLED
};


// TODO: Make the following 5 values configurable
#define NUT_MEASUREMENT_REPEAT_AFTER    300     //!< (once in 5 minutes now (300s))
#define NUT_INVENTORY_REPEAT_AFTER      3600    //!< (every hour now (3600s))
#define NUT_POLLING_INTERVAL            5000    //!< (check with upsd ever 5s)

// Note !!! If you change this value you have to change the following tests as well: TODO
#define AGENT_NUT_REPEAT_INTERVAL_SEC       NUT_MEASUREMENT_REPEAT_AFTER     //<! TODO
#define AGENT_NUT_SAMPLING_INTERVAL_SEC     5   //!< TODO: We might not need this anymore

#define KEY_REPEAT "repeat"
#define KEY_STATUS "status"
#define KEY_CONTENT_TYPE "content-type"
#define PREFIX_X "x-"
#define OK      "ok"
#define ERROR   "error"
#define YES     "yes"
#define NO      "no"
#define KEY_ADD_INFO      "add_info"
#define KEY_AFFECTED_ROWS "affected_rows"
#define KEY_ERROR_TYPE     "error_type"
#define KEY_ERROR_SUBTYPE  "error_subtype"
#define KEY_ERROR_MSG      "error_msg"
#define KEY_ROWID          "rowid"

// asset_extra message
#define KEY_ASSET_TYPE_ID   "__type_id"
#define KEY_ASSET_SUBTYPE_ID   "__subtype_id"
#define KEY_ASSET_PARENT_ID "__parent_id"
#define KEY_ASSET_PRIORITY  "__priority"
#define KEY_ASSET_STATUS    "__status"
#define KEY_ASSET_NAME      "__name"
#define KEY_OPERATION       "__operation"

// agent names
#define FTY_EMAIL_ADDRESS "fty-email"

//static const std::string url = "sqlite:/work/core/tools/my1db.db";

// self test
void
    defs_test (bool verbose);




#endif
