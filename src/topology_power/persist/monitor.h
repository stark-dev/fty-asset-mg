/*  =========================================================================
    topology_power_persist_monitor - class description

    Copyright (C) 2014 - 2018 Eaton

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

#ifndef TOPOLOGY_POWER_PERSIST_MONITOR_H_INCLUDED
#define TOPOLOGY_POWER_PERSIST_MONITOR_H_INCLUDED

#include <tntdb/connect.h>
//#include "topology_power/persist/dbhelpers2.h"
#include <fty_common_db.h>

// ===============================================================
// DEVICE
// ===============================================================

db_reply_t
    select_device (tntdb::Connection &conn,
                   const char* device_name);

#endif
