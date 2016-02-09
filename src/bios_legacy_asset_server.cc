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
    bios_legacy_asset_server - Server translating legacy configure messages to new protocl
@discuss
@end
*/

#include "agent_asset_classes.h"

void
    bios_legacy_asset_server (zsock_t *pipe, void *args)
{
    bool verbose = false;
    char *name = strdup ((char*)args);

    bios_agent_t *agent;
    mlm_client_t *client = mlm_client_new ();
    zpoller_t *poller = zpoller_new (pipe, NULL);

    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, -1);
        if (!which)
            break;

        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            char *cmd = zmsg_popstr (msg);

            if (verbose)
                zsys_debug ("%s: api command %s", cmd);

            if (streq (cmd, "$TERM")) {
                zstr_free (&cmd);
                zmsg_destroy (&msg);
                goto exit;
            }
            else
            if (streq (cmd, "VERBOSE"))
                verbose = true;
            else
            if (streq (cmd, "CONNECT")) {
                char* endpoint = zmsg_popstr (msg);
                int rv = mlm_client_connect (client, endpoint, 1000, name);
                if (rv == -1) {
                    zsys_error ("%s: can't connect to malamute endpoint '%s'", name, endpoint);
                }
                agent = bios_agent_new (endpoint, "legacy-assets-agent");
                if (!agent)
                {
                    zsys_error ("%s: can't connect to malamute endpoint (bios-agent) '%s'", name, endpoint);
                }
                zpoller_add (poller, bios_agent_msgpipe (agent));
                zstr_free (&endpoint);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (client, stream);
                if (rv == -1) {
                    zsys_error ("%s: can't set producer on stream '%s'", name, stream);
                }
                zstr_free (&stream);
            }
            else
            if (streq (cmd, "CONSUMER")) {
                char* stream = zmsg_popstr (msg);
                char* pattern = zmsg_popstr (msg);
                int rv = bios_agent_set_consumer (agent, stream, pattern);
                if (rv == -1) {
                    zsys_error ("%s: can't set consumer on stream '%s', '%s'", name, stream, pattern);
                }
                zstr_free (&pattern);
                zstr_free (&stream);
            }
            else
                zsys_info ("unhandled command %s", cmd);
            zstr_free (&cmd);
            zmsg_destroy (&msg);
            continue;
        }

        ymsg_t *ymsg = bios_agent_recv (agent);
        char *name, *status;
        zhash_t *ext;
        uint32_t type_id, subtype_id, parent_id;
        uint8_t priority;
        int8_t event_type;

        int r = bios_asset_extra_extract (ymsg,
                &name,
                &ext,
                &type_id,
                &subtype_id,
                &parent_id,
                &status,
                &priority,
                &event_type);


        if (r != 0) {
            zsys_warning ("can't decode message from sender %s, subject %s : %d", bios_agent_sender (agent), bios_agent_subject (agent), r);
            continue;
        }
        zsys_debug ("name=%s, status=%s", name, status);
        zsys_debug ("type_id=%d, subtype_id=%d, parent_id=%d", type_id, subtype_id, parent_id);

        char *s_priority, *s_parent;
        asprintf (&s_priority, "%u", (unsigned) priority);
        asprintf (&s_parent, "%lu", (long) parent_id);  // TODO: map to name through DB

        char *subject;
        asprintf (&subject, "%s.%s@%s", asset_type2str (type_id), asset_subtype2str (subtype_id), name);

        zhash_t *aux = zhash_new ();
        zhash_insert (aux, "priority", (void*) s_priority);
        zhash_insert (aux, "type", (void*) asset_type2str (type_id));
        zhash_insert (aux, "subtype", (void*) asset_subtype2str (subtype_id));
        zhash_insert (aux, "parent", (void*) s_parent);
        zhash_insert (aux, "status", status);

        zmsg_t *msg = bios_proto_encode_asset (
                aux,
                name,
                asset_op2str (event_type),
                ext);
        mlm_client_send (client, subject, &msg);

        zhash_destroy (&ext);
        zhash_destroy (&aux);
        zstr_free (&subject);
        zstr_free (&s_parent);
        zstr_free (&s_priority);

        zstr_free (&name);
        zstr_free (&status);
        ymsg_destroy (&ymsg);
    }

exit:
    zpoller_destroy (&poller);
    bios_agent_destroy (&agent);
    mlm_client_destroy (&client);
    zstr_free (&name);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
bios_legacy_asset_server_test (bool verbose)
{
    printf (" * bios_legacy_asset_server: ");

    //  @selftest
    static const char* endpoint = "inproc://bios-legacy-asset-server-test";

    // malamute broker
    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    assert ( server != NULL );
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", endpoint, NULL);

    // legacy assets
    zactor_t *la_server = zactor_new (bios_legacy_asset_server, (void*)"agent-legacy-metrics");
    if (verbose)
        zstr_send (la_server, "VERBOSE");
    zstr_sendx (la_server, "CONNECT", endpoint, NULL);
    zstr_sendx (la_server, "CONSUMER", bios_get_stream_main (),"^configure.*", NULL);
    zstr_sendx (la_server, "PRODUCER", "ASSETS", NULL);

    bios_agent_t *agent = bios_agent_new (endpoint, "rest-api");
    bios_agent_set_producer (agent, bios_get_stream_main ());

    mlm_client_t *client = mlm_client_new ();
    mlm_client_connect (client, endpoint, 5000, "asset-reader");
    mlm_client_set_consumer (client, "ASSETS", ".*");

    zhash_t *ext = zhash_new ();
    zhash_insert (ext, "key", (void*) "value");
    const char *name = "NAME";
    uint32_t type_id = 1;
    uint32_t subtype_id = 2;
    uint32_t parent_id = 1;
    const char *status = "active";
    uint8_t priority = 2;
    int8_t operation = 1;
    ymsg_t * ymsg = bios_asset_extra_encode
        (name, &ext, type_id, subtype_id, parent_id, status, priority, operation);
    assert (ymsg);
    bios_agent_send (agent, "configure@NAME", &ymsg);

    zmsg_t *msg = mlm_client_recv (client);
    bios_proto_t *bmsg = bios_proto_decode (&msg);
    assert (bmsg);
    bios_proto_print (bmsg);

    assert (streq (mlm_client_subject (client), "group.genset@NAME"));
    assert (streq (bios_proto_name (bmsg), "NAME"));
    assert (streq (bios_proto_operation (bmsg), "create"));
    assert (zhash_lookup (bios_proto_ext (bmsg), "key"));
    assert (streq (
                (char*) zhash_lookup (bios_proto_ext (bmsg), "key"),
                "value"));
    assert (streq (
                (char*) zhash_lookup (bios_proto_aux (bmsg), "parent"),
                "1"));
    assert (streq (
                (char*) zhash_lookup (bios_proto_aux (bmsg), "status"),
                "active"));
    assert (streq (
                (char*) zhash_lookup (bios_proto_aux (bmsg), "type"),
                "group"));
    assert (streq (
                (char*) zhash_lookup (bios_proto_aux (bmsg), "subtype"),
                "genset"));
    assert (streq (
                (char*) zhash_lookup (bios_proto_aux (bmsg), "priority"),
                "2"));
    bios_proto_destroy (&bmsg);

    mlm_client_destroy (&client);
    bios_agent_destroy (&agent);
    zactor_destroy (&la_server);
    zactor_destroy (&server);

    //  @end
    printf ("OK\n");
}
