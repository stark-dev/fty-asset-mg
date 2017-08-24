/*  =========================================================================
    dbhelpers - Helpers functions for database

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

#ifndef DBHELPERS_H_INCLUDED
#define DBHELPERS_H_INCLUDED


#include <functional>
#include <string>
#include <map>
#include <tntdb/row.h>
#include <tntdb/connect.h>
#include <czmq.h>
#include <vector>
#include <tuple>
#include <string>
#include "fty_asset_classes.h"
//#include "preproc.h"

// all fields called name
#define MAX_NAME_LENGTH         50
// t_bios_asset_ext_attributes.keytag
#define MAX_KEYTAG_LENGTH       40
// t_bios_asset_ext_attributes.value
#define MAX_VALUE_LENGTH        255
// t_bios_asset_device.mac
#define MAX_MAC_LENGTH          17
// t_bios_asset_device.hostname
#define MAX_HOSTNAME_LENGTH     25
// t_bios_asset_device.fullhostname
#define MAX_FULLHOSTNAME_LENGTH 45

#define MAX_DESCRIPTION_LENGTH  255

/**
 * \brief helper structure for results of v_bios_asset_element
 */
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

struct db_a_elmnt_t {
    uint32_t         id;
    std::string      name;
    std::string      status;
    uint32_t         parent_id;
    uint32_t         priority;
    uint32_t         type_id;
    uint32_t         subtype_id;
    std::string      asset_tag;
    std::map <std::string, std::string> ext;

    db_a_elmnt_t () :
        id{},
        name{},
        status{},
        parent_id{},
        priority{},
        type_id{},
        subtype_id{},
        asset_tag{},
        ext{}
    {}

    db_a_elmnt_t (
        uint32_t         id,
        std::string      name,
        std::string      status,
        uint32_t         parent_id,
        uint32_t         priority,
        uint32_t         type_id,
        uint32_t         subtype_id,
        std::string      asset_tag) :

        id(id),
        name(name),
        status(status),
        parent_id(parent_id),
        priority(priority),
        type_id(type_id),
        subtype_id(subtype_id),
        asset_tag(asset_tag),
        ext{}
    {}
};


typedef std::function<void(const tntdb::Row&)> row_cb_f ;

template <typename T>
struct db_reply{
    int status; // ok/fail
    int errtype;
    int errsubtype;
    uint64_t rowid;  // insert/update or http error code if status == 0
    uint64_t affected_rows; // for update/insert/delete
    std::string msg;
    zhash_t *addinfo;
    T item;
};

typedef db_reply<uint64_t> db_reply_t;

inline db_reply_t db_reply_new() {
    return db_reply_t {
/* .status  */        1,
/* .errtype */        0,
/* .errsubtype */     0,
/* .rowid */          0,
/* .affected_rows */  0,
/* .msg = */         "",
/* .addinfo  */    NULL,
/* .item */          0};
}

template <typename T>
inline db_reply<T> db_reply_new(T& item) {
    return db_reply<T> {
/* .status  */       1,
/* .errtype */       0,
/* .errsubtype */    0,
/* .rowid */         0,
/* .affected_rows  */0,
/* .msg  */         "",
/* .addinfo  */   NULL,
/* .item  */     item};
}

FTY_ASSET_PRIVATE int
    select_asset_id (
        const std::string &name,
        a_elmnt_id_t &id
    );

FTY_ASSET_PRIVATE int
    select_assets_by_container (
        const std::string& container_name,
        const std::set <std::string>& filter,
        std::vector <std::string>& assets
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
FTY_ASSET_PRIVATE int
    select_assets_by_container_cb (
        a_elmnt_id_t element_id,
        std::function<void(const tntdb::Row&)> cb
    );

FTY_ASSET_PRIVATE std::string
select_assets_by_container_filter (
    const std::set<std::string> &types_and_subtypes
);

FTY_ASSET_PRIVATE int
    select_assets_by_filter (
        const std::set <std::string>& filter,
        std::vector <std::string>& assets
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
FTY_ASSET_PRIVATE int
    select_links_by_container (
        a_elmnt_id_t element_id,
        std::set <std::pair<a_elmnt_id_t ,a_elmnt_id_t> > &links
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
    process_insert_inventory
    (const std::string& device_name,
    zhash_t *ext_attributes,
    bool test);

//  Self test of this class
void
    dbhelpers_test (bool verbose);

//  @end



#endif
