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
    bool verbose;
} asset_autoupdate_t;

static void
asset_autoupdate_destroy (asset_autoupdate_t **self_p)
{
    assert (self_p);
    asset_autoupdate_t *self = *self_p;
    if ( self ) {
        zstr_free (&self->name);
        mlm_client_destroy (&self->client);
        free (self);
        *self_p = NULL;
    }
}

static asset_autoupdate_t*
asset_autoupdate_new (void)
{
    asset_autoupdate_t *self = (asset_autoupdate_t *) zmalloc (sizeof(asset_autoupdate_t));
    if ( self ) {
        self->client = mlm_client_new ();
        if (self->client) {
            self->verbose = false;
        }
        else {
            asset_autoupdate_destroy (&self);
        }
    }
    return self;
}


#define is_ipv6(X) (X.find(':') != std::string::npos)
#define icase_streq(X,Y) (strcasecmp ((X),(Y)) == 0)

void
autoupdate_update_rc_self(asset_autoupdate_t *self, const std::string &assetName)
{
    bool haveLAN1 = false;
    zhash_t *inventory = zhash_new ();
    zhash_t *aux = zhash_new ();
    std::string key, topic;

    zhash_autofree (inventory);
    zhash_autofree (aux);
    zhash_update (aux, "time", (void *)std::to_string (zclock_time () / 1000).c_str ());

    auto ifaces = local_addresses();
    haveLAN1 = (ifaces.find ("LAN1") != ifaces.cend ());
    if (haveLAN1) {
        // hopefully running on RC
        int ipv4index = 0;
        int ipv6index = 0;
        for (int i = 1; i <= 3; ++i) {
            const auto iface = ifaces.find( "LAN" + std::to_string (i));
            if (iface != ifaces.cend()) {
                for (auto addr: iface->second) {
                    if (is_ipv6 (addr)) {
                        key = "ipv6." + std::to_string (++ipv6index);
                    } else {
                        key = "ip." + std::to_string (++ipv4index);
                    }
                    zhash_update (inventory, key.c_str(), (void *)addr.c_str ());
                }
            }
        }
        zmsg_t *msg = bios_proto_encode_asset (
            aux,
            assetName.c_str (),
            "inventory",
            inventory);
        if (msg) {
            topic = "inventory@" + assetName;
            zsys_debug ("new inventory message %s", topic.c_str());
            int r = mlm_client_send (self->client, topic.c_str (), &msg);
            if( r != 0 ) zsys_error("failed to send inventory %s result %" PRIi32, topic.c_str(), r);
            zmsg_destroy (&msg);
        }
    } else {
        // interfaces have unexpected names, publish them in unspecified order
        int ipv4index = 0;
        int ipv6index = 0;
        for (auto interface: ifaces) {
            if (interface.first == "lo") continue;
            for (auto addr: interface.second) {
                if (is_ipv6 (addr)) {
                    key = "ipv6." + std::to_string (++ipv6index);
                } else {
                    key = "ip." + std::to_string (++ipv4index);
                }
                zhash_update (inventory, key.c_str(), (void *)addr.c_str ());
            }
        }
        zmsg_t *msg = bios_proto_encode_asset (
            aux,
            assetName.c_str (),
            "inventory",
            inventory);
        if (msg) {
            topic = "inventory@" + assetName;
            zsys_debug ("new inventory message %s", topic.c_str());
            int r = mlm_client_send (self->client, topic.c_str (), &msg);
            if( r != 0 ) zsys_error("failed to send inventory %s result %" PRIi32, topic.c_str(), r);
            zmsg_destroy (&msg);
        }
    }
    zhash_destroy (&inventory);
    zhash_destroy (&aux);
}

void
autoupdate_update_rc_information(asset_autoupdate_t *self)
{
    if (!self || !self->client) return;

    if (self->rcs.size() == 1) {
        // there is only one rc, must be me
        autoupdate_update_rc_self (self, self->rcs[0]);
        return;
    }
    if (self->rcs.size() > 1) {
        // there are more rc, test if one of my dns names == rc name
        // resolve dns names of all IP addresses on all interfaces
        // compare resolved name with list of RCs
        for (auto interface: local_addresses ()) {
            for (auto ip: interface.second) {
                std::string name = ip_to_name (ip.c_str());
                if (! name.empty ()) {
                    std::string hostname = name;
                    unsigned int i = hostname.find ('.');
                    if (i != std::string::npos) {
                        hostname.resize (i);
                    }
                    zsys_debug ("ip %s, dns name '%s'", ip.c_str (), name.c_str ());
                    for (auto rc: self->rcs) {
                        if (icase_streq (rc.c_str (), hostname.c_str ()) || icase_streq (rc.c_str (), name.c_str ())) {
                            // asset name == hostname this is me
                            autoupdate_update_rc_self (self, rc);
                            return;
                        }
                    }
                }
            }
        }
    }
}

void
autoupdate_request_all_rcs (asset_autoupdate_t *self)
{
    if (!self ) return;

    zsys_debug ("requesting RC list");
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "GET");
    zmsg_addstr (msg, "");
    zmsg_addstr (msg, "rack controller");
    int rv = mlm_client_sendto (self->client, "asset-agent", "ASSETS_IN_CONTAINER", NULL, 5000, &msg);
    if (rv != 0) {
        zsys_error ("sending request for list of RCs failed");
        zmsg_destroy (&msg);
    }
}

void
autoupdate_update (asset_autoupdate_t *self)
{
    if (!self ) return;

    autoupdate_update_rc_information (self);
}

void
autoupdate_handle_message (asset_autoupdate_t *self, zmsg_t *message)
{
    if (!self || !message ) return;

    const char *sender = mlm_client_sender (self->client);
    const char *subj = mlm_client_subject (self->client);
    if (streq (sender, "asset-agent")) {
        if (streq (subj, "ASSETS_IN_CONTAINER")) {
            zmsg_print (message);
            self->rcs.clear ();
            char *okfail = zmsg_popstr (message);
            if (streq (okfail, "OK")) {
                char *rc = zmsg_popstr(message);
                while (rc) {
                    self->rcs.push_back(rc);
                    // TODO: ask agent-asset for device details
                    zstr_free (&rc);
                    rc = zmsg_popstr(message);
                }
            }
            zstr_free (&okfail);
            autoupdate_update (self);
        }
    }
}


void
bios_asset_autoupdate_server (zsock_t *pipe, void *args)
{
    assert (pipe);
    assert (args);

    asset_autoupdate_t *self = asset_autoupdate_new ();
    assert (self);
    self->name = strdup ((char*) args);
    assert (self->name);

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe(self->client), NULL);
    assert (poller);

    // Signal need to be send as it is required by "actor_new"
    zsock_signal (pipe, 0);
    zsys_info ("%s:\tStarted", self->name);

    // ask for list of RCs
    //autoupdate_request_all_rcs (&self);
    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, -1);
        if ( !which ) {
            // cannot expire as waiting until infinity
            // so it is interrupted
            continue;
        }
        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            char *cmd = zmsg_popstr (msg);
            if ( self->verbose ) {
                zsys_debug ("%s:\tActor command=%s", self->name, cmd);
            }

            if (streq (cmd, "$TERM")) {
                if ( !self->verbose ) // ! is here intentionally, to get rid of duplication information
                    zsys_info ("%s:\tGot $TERM", self->name);
                zstr_free (&cmd);
                zmsg_destroy (&msg);
                goto exit;
            }
            else
            if (streq (cmd, "VERBOSE")) {
                self->verbose = true;
            }
            else
            if (streq (cmd, "CONNECT")) {
                char* endpoint = zmsg_popstr (msg);
                int rv = mlm_client_connect (self->client, endpoint, 1000, self->name);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't connect to malamute endpoint '%s'", self->name, endpoint);
                }
                zstr_free (&endpoint);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (self->client, stream);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't set producer on stream '%s'", self->name, stream);
                }
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "CONSUMER")) {
                char* stream = zmsg_popstr (msg);
                char* pattern = zmsg_popstr (msg);
                int rv = mlm_client_set_consumer (self->client, stream, pattern);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't set consumer on stream '%s', '%s'", self->name, stream, pattern);
                }
                zstr_free (&pattern);
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "WAKEUP")) {
                autoupdate_request_all_rcs (self);
            }
            else
            {
                zsys_info ("%s:\tUnhandled command %s", self->name, cmd);
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
            continue;
        }
        if (which == mlm_client_msgpipe(self->client)) {
            zmsg_t *zmessage = mlm_client_recv (self->client);
            autoupdate_handle_message (self, zmessage);
            zmsg_destroy (&zmessage);
        }
    }
 exit:
    zsys_info ("%s:\tended", self->name);
    zpoller_destroy (&poller);
    asset_autoupdate_destroy (&self);
}

void
bios_asset_autoupdate_test (bool verbose)
{
    printf (" * bios_asset_autoupdate: ");
    //  @selftest
    //  @end
    printf ("OK\n");
}
