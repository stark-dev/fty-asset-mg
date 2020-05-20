/*  =========================================================================
    fty_asset_server - Asset server, that takes care about distribution of asset information across the system

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

#ifndef FTY_ASSET_SERVER_H_INCLUDED
#define FTY_ASSET_SERVER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_INAME "DC-1"
#define TEST_ENAME "MyDC"

//  @interface
//  Asset server, that takes care about distribution of
//                                      asset information across the system
FTY_ASSET_EXPORT void fty_asset_server(zsock_t* pipe, void* args);

//  Self test of this class
FTY_ASSET_EXPORT void fty_asset_server_test(bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

static constexpr const char* FTY_ASSET_MAILBOX = "FTY.Q.ASSET.QUERY";
// new interface mailbox subjects
static constexpr const char* FTY_ASSET_SUBJECT_CREATE      = "CREATE";
static constexpr const char* FTY_ASSET_SUBJECT_UPDATE      = "UPDATE";
static constexpr const char* FTY_ASSET_SUBJECT_DELETE      = "DELETE";
static constexpr const char* FTY_ASSET_SUBJECT_DELETE_LIST = "DELETE_LIST";
static constexpr const char* FTY_ASSET_SUBJECT_GET         = "GET";
static constexpr const char* FTY_ASSET_SUBJECT_LIST        = "LIST";

// new interface topics
static constexpr const char* FTY_ASSET_TOPIC_CREATED = "FTY.T.ASSET.CREATED";
static constexpr const char* FTY_ASSET_TOPIC_UPDATED = "FTY.T.ASSET.UPDATED";
static constexpr const char* FTY_ASSET_TOPIC_DELETED = "FTY.T.ASSET.DELETED";

// new interface topic subjects
static constexpr const char* FTY_ASSET_SUBJECT_CREATED = "CREATED";
static constexpr const char* FTY_ASSET_SUBJECT_UPDATED = "UPDATED";
static constexpr const char* FTY_ASSET_SUBJECT_DELETED = "DELETED";


static constexpr const char* METADATA_TRY_ACTIVATE      = "TRY_ACTIVATE";
static constexpr const char* METADATA_NO_ERROR_IF_EXIST = "NO_ERROR_IF_EXIST";

#endif
