/*  =========================================================================
    asset_defs - Some fancy definitions related to assets

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

#ifndef ASSET_DEFS_H_INCLUDED
#define ASSET_DEFS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

// Enum to define legacy asset type and subtype
enum asset_type {
    TUNKNOWN     = 0,
    GROUP       = 1,
    DATACENTER  = 2,
    ROOM        = 3,
    ROW         = 4,
    RACK        = 5,
    DEVICE      = 6
};

enum asset_subtype {
    SUNKNOWN = 0,
    UPS = 1,
    GENSET,
    EPDU,
    PDU,
    SERVER,
    FEED,
    STS,
    SWITCH,
    STORAGE,
    VIRTUAL,
    N_A = 11,
    /* ATTENTION: don't change N_A id. It is used as default value in init.sql for types, that don't have N_A */
    ROUTER,
    RACKCONTROLLER,
    SENSOR
};

enum asset_operation
{
    INSERT = 1,
    DELETE,
    UPDATE,
    GET,
    RETIRE,
    INVENTORY
};

//  translate numeric ID to const char* type
const char *asset_type2str (int asset_type);

//  translate numeric ID to const char* subtype
const char *asset_subtype2str (int asset_subtype);

//  translate numeric ID to const char*
const char *asset_op2str (int asset_op);

uint32_t
    type_to_typeid
        (const std::string &type);
uint32_t
    subtype_to_subtypeid
        (const std::string &subtype);

uint32_t
    str2operation
        (const std::string &operation);

FTY_ASSET_EXPORT void
    asset_defs_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
