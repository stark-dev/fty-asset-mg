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
        case VIRTUAL:
            return "virtual";
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
