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

//  Self test of this class
void
    dbhelpers_test (bool verbose);

//  @end



#endif
