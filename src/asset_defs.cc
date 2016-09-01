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

/*
@header
    asset_defs - Some fancy definitions related to assets
@discuss
@end
*/

#include "agent_asset_classes.h"
#include <algorithm>

//  Structure of our class

//  translate numeric ID to const char*
const char *asset_type2str (int asset_type)
{
    switch (asset_type) {
        case GROUP:
            return "group";
        case DATACENTER:
            return "datacenter";
        case ROOM:
            return "room";
        case ROW:
            return "row";
        case RACK:
            return "rack";
        case DEVICE:
            return "device";
        default:
            return "unknown";
    };
    return "make-gcc-happy"; // should not reach here
}

//  translate numeric ID to const char*
const char *asset_subtype2str (int asset_subtype)
{
    switch (asset_subtype) {
        case UPS:
            return "ups";
        case GENSET:
            return "genset";
        case EPDU:
            return "epdu";
        case PDU:
            return "pdu";
        case SERVER:
            return "server";
        case FEED:
            return "feed";
        case STS:
            return "sts";
        case SWITCH:
            return "switch";
        case STORAGE:
            return "storage";
        case ROUTER:
            return "router";
        case RACKCONTROLLER:
            return "rack controller";
        case VIRTUAL:
            return "virtual";
        case SENSOR:
            return "sensor";
        case N_A:
            return "N_A";
        default:
            return "unknown";
    };
    return "make-gcc-happy"; // should not reach here
}

//  translate numeric ID to const char*
const char *asset_op2str (int asset_op)
{

    switch (asset_op)
    {
        case INSERT:
            return "create";
        case DELETE:
            return "delete";
        case UPDATE:
            return "update";
        case GET:
            return "get";
        case RETIRE:
            return "retire";
        default:
            return "unknown";
    }
    return "make-gcc-happy"; // should not reach here
}

uint32_t
    type_to_typeid
        (const std::string &type)
{
    std::string t (type);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    if(t == "datacenter") {
        return asset_type::DATACENTER;
    } else if(t == "room") {
        return asset_type::ROOM;
    } else if(t == "row") {
        return asset_type::ROW;
    } else if(t == "rack") {
        return asset_type::RACK;
    } else if(t == "group") {
        return asset_type::GROUP;
    } else if(t == "device") {
        return asset_type::DEVICE;
    } else
        return asset_type::TUNKNOWN;
}

uint32_t
    subtype_to_subtypeid
        (const std::string &subtype)
{
    std::string st(subtype);
    std::transform(st.begin(), st.end(), st.begin(), ::tolower);
    if(st == "ups") {
        return asset_subtype::UPS;
    }
    else if(st == "genset") {
        return asset_subtype::GENSET;
    }
    else if(st == "epdu") {
        return asset_subtype::EPDU;
    }
    else if(st == "server") {
        return asset_subtype::SERVER;
    }
    else if(st == "pdu") {
        return asset_subtype::PDU;
    }
    else if(st == "feed") {
        return asset_subtype::FEED;
    }
    else if(st == "sts") {
        return asset_subtype::STS;
    }
    else if(st == "switch") {
        return asset_subtype::SWITCH;
    }
    else if(st == "storage") {
        return asset_subtype::STORAGE;
    }
    else if (st == "vm") {
        return asset_subtype::VIRTUAL;
    }
    else if (st == "router") {
        return asset_subtype::ROUTER;
    }
    else if (st == "rack controller") {
        return asset_subtype::RACKCONTROLLER;
    }
    else if (st == "sensor") {
        return asset_subtype::SENSOR;
    }
    else if(st == "n_a") {
        return asset_subtype::N_A;
    }
    else if(st == "") {
        return asset_subtype::N_A;
    }
    else
        return asset_subtype::SUNKNOWN;
}


uint32_t
    str2operation
        (const std::string &operation)
{
    std::string t (operation);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    if(t == "create") {
        return asset_operation::INSERT;
    } else if(t == "delete") {
        return asset_operation::DELETE;
    } else if(t == "retire") {
        return asset_operation::RETIRE;
    } else if(t == "inventory") {
        return asset_operation::INVENTORY;
    } else if(t == "update") {
        return asset_operation::UPDATE;
    } else if(t == "get") {
        return asset_operation::GET;
    }
    return 0;
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
asset_defs_test (bool verbose)
{
    printf (" * asset_defs: ");

    //  @selftest
    assert (streq (asset_type2str (2), "datacenter"));
    assert (streq (asset_subtype2str (2), "genset"));
    assert (streq (asset_op2str (2), "delete"));
    //  @end
    printf ("OK\n");
}
