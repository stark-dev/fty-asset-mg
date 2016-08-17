/*  =========================================================================
    bios_asset_autoupdate - Asset server, that udates some of asset information on change like IP address in case of DHCP

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
    bios_asset_autoupdate - Asset server, that udates some of asset information on change like IP address in case of DHCP
@discuss
@end
*/

#include "agent_asset_classes.h"
typedef struct {
    mlm_client_t *client = NULL;
    char *name = NULL;
    std::vector<std::string> rcs;
} asset_autoupdate_t;

void
autoupdate_request_all_rcs (asset_autoupdate_t *self)
{
    if (!self || !self->client) return;
    
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "GET");
    zmsg_addstr (msg, "");
    zmsg_addstr (msg, "rack controller");
    mlm_client_sendto (self->client, "asset-agent", "ASSETS_IN_CONTAINER", NULL, 5000, &msg);
}

void
autoupdate_check (asset_autoupdate_t *self)
{
    if (!self || !self->client) return;

    autoupdate_request_all_rcs (self);
}

void
bios_asset_autoupdate_server (zsock_t *pipe, void *args)
{
    zsys_debug ("asset autoupdate started");
    bool verbose = false;
    asset_autoupdate_t self;

    self.name = strdup ((char*) args);
    self.client = mlm_client_new ();

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe(self.client), NULL);

    // Signal need to be send as it is required by "actor_new"
    zsock_signal (pipe, 0);

    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, 30000);
        if ( !which ) {
            autoupdate_request_all_rcs (&self);
            // cannot expire as waiting until infinity
            // so it is interrupted
            break;
        }
        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            char *cmd = zmsg_popstr (msg);
            if ( verbose ) {
                zsys_debug ("actor command=%s", cmd);
            }

            if (streq (cmd, "$TERM")) {
                zsys_info ("Got $TERM");
                zstr_free (&cmd);
                zmsg_destroy (&msg);
                goto exit;
            }
            else
            if (streq (cmd, "VERBOSE")) {
                verbose = true;
            }
            else
            if (streq (cmd, "CONNECT")) {
                char* endpoint = zmsg_popstr (msg);
                int rv = mlm_client_connect (self.client, endpoint, 1000, self.name);
                if (rv == -1) {
                    zsys_error ("%s: can't connect to malamute endpoint '%s'", self.name, endpoint);
                }
                zstr_free (&endpoint);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (self.client, stream);
                if (rv == -1) {
                    zsys_error ("%s: can't set producer on stream '%s'", self.name, stream);
                }
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "CONSUMER")) {
                char* stream = zmsg_popstr (msg);
                char* pattern = zmsg_popstr (msg);
                int rv = mlm_client_set_consumer (self.client, stream, pattern);
                if (rv == -1) {
                    zsys_error ("%s: can't set consumer on stream '%s', '%s'", self.name, stream, pattern);
                }
                zstr_free (&pattern);
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            {
                zsys_info ("unhandled command %s", cmd);
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
            continue;
        }

        // This agent is a reactive agent, it reacts only on messages
        // and doesn't do anything if there are no messages
        zmsg_t *zmessage = mlm_client_recv (self.client);
        if ( zmessage == NULL ) continue;
        zmsg_destroy (&zmessage);
    }
 exit:
    zpoller_destroy (&poller);
    mlm_client_destroy (&self.client);
    zstr_free (&self.name);
    zsys_debug ("asset autoupdate ended");
}

void
bios_asset_autoupdate_test (bool verbose)
{
    printf (" * bios_asset_autoupdate: ");
    //  @selftest
    //  @end
    printf ("OK\n");
}
