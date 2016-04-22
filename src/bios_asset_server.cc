/*  =========================================================================
    bios_legacy_asset_server - Server translating legacy configure messages to new protocl

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
    bios_asset_server - Asset server, that takes care about distribution of
                                      asset information across the system
@discuss
     ASSET PROTOCOL
     ## Topology request

     power topology request:
         subject: "TOPOLOGY"
         message: is a multipart message A/B
                 A = "TOPOLOGY_POWER" - mandatory
                 B = "asset_name" - mandatory

     power topology reply in "OK" case:
         subject: "TOPOLOGY"
         message: is a multipart message A/B/D/C1/.../CN
                 A = "TOPOLOGY_POWER" - mandatory
                 B = "asset_name" - mandatory
                 D = "OK" - mandatory
                 Ci = "asset_name" of power source - not mandatory
                     if there are no power devices
                      -> message is A/B/D

     power topology reply in "ERROR" case:
         subject: "TOPOLOGY"
         message: is a multipart message A/B/D/E
                 A = "TOPOLOGY_POWER" - mandatory
                 B = "asset_name" - mandatory
                 D = "ERROR" - mandatory
                 E = "ASSET_NOT_FOUND"/"INTERNAL_ERROR" - mandatory
@end
*/

#include "agent_asset_classes.h"
#include <string>
#include <tntdb/connect.h>
// TODO add dependencies tntdb and cxxtools

// ============================================================
//         Functionality for TOPOLOGY processing
// ============================================================
static void
    s_processTopology(
        mlm_client_t *client,
        const std::string &assetName)
{
    // result of power topology - list of power device names
    std::vector<std::string> powerDevices{};
    
    // select power devices
    int rv = select_devices_total_power(assetName, powerDevices);
    
    // message to be sent as reply
    zmsg_t *msg = zmsg_new ();

    // these 2 parts are the same for OK and ERROR messages
    zmsg_addstr (msg, "TOPOLOGY_POWER");
    zmsg_addstr (msg, assetName.c_str());

    // form a message according the contract for the case "OK" and for the case "ERROR"
    if ( rv == -1 ) {
        zsys_error ("Error occured during the request processing");
        zmsg_addstr (msg, "ERROR");
        zmsg_addstr (msg, "INTERNAL_ERROR");
    } else if ( rv == -2 ) {
        zsys_debug ("Asset was not found");
        zmsg_addstr (msg, "ERROR");
        zmsg_addstr (msg, "ASSET_NOT_FOUND");
    } else {
        zmsg_addstr (msg, "OK");

        zsys_debug ("Power topology for '%s':", assetName.c_str());
        for (const auto &powerDeviceName : powerDevices ) {
            zsys_debug ("   - %s", powerDeviceName.c_str());
            zmsg_addstr (msg, powerDeviceName.c_str());
        }
    }
    mlm_client_sendto (client, mlm_client_sender (client), "TOPOLOGY", NULL, 5000, &msg);
}


void
    bios_asset_server (zsock_t *pipe, void *args)
{
    zsys_debug ("asset server started");
    bool verbose = false;

    char *name = strdup ((char*) args);

    mlm_client_t *client = mlm_client_new ();

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe(client), NULL);

    // Signal need to be send as it is required by "actor_new"
    zsock_signal (pipe, 0);

    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, -1);
        if ( !which ) {
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
                int rv = mlm_client_connect (client, endpoint, 1000, name);
                if (rv == -1) {
                    zsys_error ("%s: can't connect to malamute endpoint '%s'", name, endpoint);
                }
                zstr_free (&endpoint);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (client, stream);
                if (rv == -1) {
                    zsys_error ("%s: can't set producer on stream '%s'", name, stream);
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
                    zsys_error ("%s: can't set consumer on stream '%s', '%s'", name, stream, pattern);
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
        zmsg_t *zmessage = mlm_client_recv (client);
        if ( zmessage == NULL ) {
            continue;
        }
        std::string topic = mlm_client_subject(client);
        if ( verbose ) {
            zsys_debug("Got message '%s'", topic.c_str());
        }

        // Some stream message comes:
        //  * INSERT/UPDATE/GET/DELETE/RETIRE -> TODO
        //
        // TODO rework message processing
        if (is_bios_proto (zmessage)) {
            // DO NOTHING for now
        }
        else {
            char* command = zmsg_popstr (zmessage);
            if ( streq (command, "TOPOLOGY_POWER") ) {
                char* asset_name = zmsg_popstr (zmessage);
                s_processTopology (client, asset_name);
                zstr_free (&asset_name);
            }
            else {
                zsys_error ("unexpected command '%s'", command);
            }
            zstr_free (&command);
        }
        zmsg_destroy (&zmessage);
    }
exit:
    //TODO:  save info to persistence before I die
    zpoller_destroy (&poller);
    mlm_client_destroy (&client);
    zstr_free (&name);
    zsys_debug ("asset server ended");
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
bios_asset_server_test (bool verbose)
{
    printf (" * bios_asset_server: ");
// Everything is commented out because: need to have a proper database
// --> move it out of "make check"
/*    //  @selftest
    static const char* endpoint = "inproc://bios-asset-server-test";

    // malamute broker
    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    assert ( server != NULL );
    if (verbose) {
        zstr_send (server, "VERBOSE");
    }
    
    zstr_sendx (server, "BIND", endpoint, NULL);

    // NOT legacy assets
    zactor_t *la_server = zactor_new (bios_asset_server, (void*)"AGENT_ASSET");
    if (verbose) {
        zstr_send (la_server, "VERBOSE");
    }
    zstr_sendx (la_server, "CONNECT", endpoint, NULL);
    zsock_wait (la_server);

    mlm_client_t *client = mlm_client_new ();
    mlm_client_connect (client, endpoint, 5000, "topology-peer");
    // scenario name
    std::string scenario;
    // all topology messages has the same topic
    static const char* topic = "TOPOLOGY";
    
    // ====================================================    
    scenario = "scenario 1";
    zsys_info ("# %s:", scenario.c_str());
    //      prepare and send request message
    zmsg_t *msg = zmsg_new();
    zmsg_addstr (msg, "TOPOLOGY_POWER");
    zmsg_addstr (msg, "RACK1-LAB");
    mlm_client_sendto (client, "AGENT_ASSET", topic, NULL, 5000, &msg);

    //      wait for a responce from asset agent
    zpoller_t *poller = zpoller_new (mlm_client_msgpipe(client), NULL);
    void *which = zpoller_wait (poller, 1000);
    assert ( which != NULL );
    zpoller_destroy (&poller);

    //      receive a responce
    msg = mlm_client_recv (client);

    //      check the response
    assert ( zmsg_size (msg) == 5 );
    std::vector <std::string> expectedMessageGeneral = 
        {"TOPOLOGY_POWER", "RACK1-LAB", "OK"};
    for ( int i = 0 ; i < 3 ; i++ ) {
        char *somestring = zmsg_popstr (msg);
        assert ( expectedMessageGeneral.at(i) == somestring );
        free (somestring);
    }
    std::set <std::string> expectedMessageDevices = 
        {"UPS1-LAB", "UPS2-LAB"};
    for ( int i = 3 ; i < 5 ; i++ ) {
        char *somestring = zmsg_popstr (msg);
        assert ( expectedMessageDevices.count(somestring) == 1 );
        free (somestring);
    }
    //       crear 
    zmsg_destroy (&msg);
    expectedMessageGeneral.clear(); 
    zsys_info ("### %s: OK", scenario.c_str());


    // ====================================================    
    scenario = "scenario 2";
    zsys_info ("# %s:", scenario.c_str());
    //      prepare and send request message
    msg = zmsg_new();
    zmsg_addstr (msg, "TOPOLOGY_POWER");
    zmsg_addstr (msg, "NOTFOUNDASSET");
    mlm_client_sendto (client, "AGENT_ASSET", topic, NULL, 5000, &msg);

    //      wait for a responce from asset agent
    poller = zpoller_new (mlm_client_msgpipe(client), NULL);
    which = zpoller_wait (poller, 1000);
    assert ( which != NULL );
    zpoller_destroy (&poller);

    //      receive a responce
    msg = mlm_client_recv (client);
    //      check the response
    assert ( zmsg_size (msg) == 4 );
    expectedMessageGeneral = 
        {"TOPOLOGY_POWER", "NOTFOUNDASSET", "ERROR", "ASSET_NOT_FOUND"};
    for ( int i = 0 ; i < 4 ; i++ ) {
        char *somestring = zmsg_popstr (msg);
        assert ( expectedMessageGeneral.at(i) == somestring );
        free (somestring);
    }

    //       crear 
    zmsg_destroy (&msg);
    expectedMessageGeneral.clear(); 
    zsys_info ("### %s: OK", scenario.c_str());


    // selftest should clear after itself
    mlm_client_destroy (&client);
    zactor_destroy (&la_server);
    
    zactor_destroy (&server);
*/
    //  @end
    printf ("OK\n");
}
