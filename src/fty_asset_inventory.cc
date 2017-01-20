/*  =========================================================================
    fty_asset_inventory - Inventory server: process inventory ASSET messages and update extended attributes

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
    fty_asset_inventory - Inventory server: process inventory ASSET messages and update extended attributes
@discuss
@end
*/

#include "fty_asset_classes.h"

//  Structure of our class

void
fty_asset_inventory_server (zsock_t *pipe, void *args)
{
    char *name = strdup ((const char*)args);
    mlm_client_t *client = mlm_client_new ();
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);
    bool verbose = false;

    zsock_signal (pipe, 0);
    zsys_info ("%s:\tStarted", name);

    while (!zsys_interrupted)
    {
        void *which = zpoller_wait (poller, -1);
        if (!which)
            continue;
        else
        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            char *cmd = zmsg_popstr (msg);
            if ( verbose ) {
                zsys_debug ("%s:\tActor command=%s", name, cmd);
            }

            if (streq (cmd, "$TERM")) {
                if ( !verbose ) // ! is here intentionally, to get rid of duplication information
                    zsys_info ("%s:\tGot $TERM", name);
                zstr_free (&cmd);
                zmsg_destroy (&msg);
                break;
            }
            else
            if (streq (cmd, "VERBOSE")) {
                verbose = true;
            }
            else
            if (streq (cmd, "CONNECT")) {
                char* endpoint = zmsg_popstr (msg);
                int rv = mlm_client_connect (client, endpoint, 1000, name);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't connect to malamute endpoint '%s'", name, endpoint);
                }
                zstr_free (&endpoint);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (client, stream);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't set producer on stream '%s'", name, stream);
                }
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "CONSUMER")) {
                char* stream = zmsg_popstr (msg);
                char* pattern = zmsg_popstr (msg);
                int rv = mlm_client_set_consumer (client, stream, pattern);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't set consumer on stream '%s', '%s'", name, stream, pattern);
                }
                zstr_free (&pattern);
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            {
                zsys_info ("%s:\tUnhandled command %s", name, cmd);
            }
        }
        else
        if (which == mlm_client_msgpipe (client)) {
            zmsg_t *msg = mlm_client_recv (client);
            fty_proto_t *proto = fty_proto_decode (&msg);
            if (!proto) {
                zsys_warning ("%s:'tfty_proto_decode () failed", name);
                continue;
            }
        
            const char *device_name = fty_proto_name (proto);
            zhash_t *ext = fty_proto_ext (proto);
            const char *operation = fty_proto_operation(proto);

            if (streq (operation, "inventory"))
                process_insert_inventory (device_name, ext);
            fty_proto_destroy (&proto);
        }
    }

    mlm_client_destroy (&client);
    zpoller_destroy (&poller);
    zstr_free (&name);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_asset_inventory_test (bool verbose)
{
    printf (" * fty_asset_inventory: ");

    //  @selftest
    //  Simple create/destroy test
    zactor_t *self = zactor_new (fty_asset_inventory_server, (void*) "asset-inventory");
    zclock_sleep (200);
    zactor_destroy (&self);
    //  @end
    printf ("OK\n");
}
