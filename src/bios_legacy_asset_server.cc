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
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe(client),  NULL);

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
                if (!agent) {
                    zsys_error ("%s: can't connect to malamute endpoint (bios-agent) '%s'", name, endpoint);
                }
                zstr_free (&endpoint);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = bios_agent_set_producer (agent, stream);
                if (rv == -1) {
                    zsys_error ("%s: can't set producer on stream '%s'", name, stream);
                }
                zstr_free (&stream);
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
            }
            else
                zsys_info ("unhandled command %s", cmd);
            zstr_free (&cmd);
            zmsg_destroy (&msg);
            continue;
        }

        zmsg_t *msg = mlm_client_recv (client);
        if ( !msg ) {
            zsys_error ("mlm_client_recv failed");
            continue;
        }
        bios_proto_t *bmsg = bios_proto_decode (&msg);
        if ( !bmsg ) {
            zsys_error ("bios_proto_decode failed");
            zmsg_destroy (&msg);
            continue;
        }
        if ( bios_proto_id (bmsg) != BIOS_PROTO_ASSET ) {
            zsys_error ("NOT ASSET MESSAGE detected on ASSET strema!");
            bios_proto_destroy (&bmsg);
            continue;
        }
        zhash_t *aux = bios_proto_aux (bmsg);
        char *priority = NULL;
        char *type = NULL;
        char *subtype = NULL;
        char *parent = NULL;
        char *status = NULL;
        if ( aux != NULL ) {
            // if we have additional information
            priority = (char *) zhash_lookup (aux, "priority");
            type = (char *) zhash_lookup (aux, "type");
            subtype = (char *) zhash_lookup (aux, "subtype");
            parent = (char *) zhash_lookup (aux, "parent");
            status = (char *) zhash_lookup (aux, "status");
        }

        zhash_t *ext = bios_proto_get_ext(bmsg);
        ymsg_t *newmsg = bios_asset_extra_encode(
                bios_proto_name(bmsg),
                &ext,
                (type == NULL ? 0 : type_to_typeid(type)),
                (subtype == NULL ? 0 : subtype_to_subtypeid(subtype)),
                (parent == NULL ? 0 : atoi(parent)),
                (status == NULL ? "": status),
                (priority == NULL ? 0 : atoi(priority)),
                str2operation(bios_proto_operation(bmsg))
            );

        zsys_debug ("name=%s, status=%s", bios_proto_name(bmsg), status);
        zsys_debug ("op=%s, type=%s, subtype=%s, parent_id=%s", bios_proto_operation(bmsg), type, subtype, parent);

        char *subject;
        int r = asprintf (&subject, "configure@%s", bios_proto_name(bmsg));
        assert ( r != -1);
        r = bios_agent_send (agent, subject, &newmsg);
        zstr_free (&subject);
        bios_proto_destroy (&bmsg);
    }

exit:
    zpoller_destroy (&poller);
    bios_agent_destroy (&agent);
    mlm_client_destroy (&client);
    zstr_free (&name);
}

//  --------------------------------------------------------------------------
//  Self test of this class
/*
 * \brief Helper function for asset message sending
 *
 *  \param[in] verbose - if function should produce debug information or not
 *  \param[in] producer - a client, that will publish ASSET message
 *                  according parameters
 *  \param[in] priority - priprity of the asset (or null if not specified)
 *  \param[in] type - type of the asset (or null if not specified)
 *  \param[in] subtype - subtype of the asset (or null if not specified)
 *  \param[in] parent - id of the parent (or null if not specified)
 *  \param[in] status - status of the asset
 *  \param[in] ext - hash of ext attributes
 *  \param[in] opearion - operation on the asset (mandatory)
 *  \param[in] asset_name - name of the assset (mandatory)
 */
static void s_send_asset_message (
    bool verbose,
    mlm_client_t *producer,
    const char *priority,
    const char *type,
    const char *subtype,
    const char *parent,
    const char *status,
    zhash_t *ext,
    const char *operation,
    const char *asset_name)
{
    assert (operation);
    assert (asset_name);
    assert (status);
    zhash_t *aux = zhash_new ();
    if ( priority )
        zhash_insert (aux, "priority", (void *)priority);
    if ( type )
        zhash_insert (aux, "type", (void *)type);
    if ( subtype )
        zhash_insert (aux, "subtype", (void *)subtype);
    if ( parent )
        zhash_insert (aux, "parent", (void *)parent);
    zhash_insert (aux, "status", (void *)status);
    zmsg_t *msg = bios_proto_encode_asset (aux, asset_name, operation, ext);
    assert (msg);
    int rv = mlm_client_send (producer, asset_name, &msg);
    assert ( rv == 0 );
    if ( verbose )
        zsys_info ("asset message was send");
    zhash_destroy (&aux);
}

void
bios_legacy_asset_server_test (bool verbose)
{
    printf (" * bios_legacy_asset_server: ");

    //  @selftest
    static const char* endpoint = "inproc://bios-legacy-asset-server-test";

    // malamute broker
    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    assert ( server != NULL );
    zstr_sendx (server, "BIND", endpoint, NULL);

    // legacy assets
    zactor_t *la_server = zactor_new (bios_legacy_asset_server, (void*)"agent-legacy-metrics");
    if (verbose)
        zstr_send (la_server, "VERBOSE");
    zstr_sendx (la_server, "CONNECT", endpoint, NULL);
    zstr_sendx (la_server, "PRODUCER", bios_get_stream_main (), NULL);
    zstr_sendx (la_server, "CONSUMER", "ASSETS", ".*", NULL);

    bios_agent_t *agent = bios_agent_new (endpoint, "autoconfig");
    bios_agent_set_consumer (agent, bios_get_stream_main(), "^configure@");

    mlm_client_t *client = mlm_client_new ();
    mlm_client_connect (client, endpoint, 5000, "asset-producer");
    mlm_client_set_producer (client, "ASSETS");

    zhash_t *ext = zhash_new ();
    zhash_autofree(ext);
    zhash_insert (ext, "qqq", (void*)"wq");
    s_send_asset_message (verbose, client, "5", "rack", "N_A", "223", "active", ext, "create", "ASSET_NN");
    zhash_destroy (&ext);
    ymsg_t *msg = bios_agent_recv (agent);
    assert ( msg );
    char *device_name = NULL;
    uint32_t type_id = 0;
    uint32_t subtype_id = 0;
    uint32_t parent_id = 0;
    char *status = NULL;
    uint8_t priority = 0;
    int8_t operation = 0;

    int r = bios_asset_extra_extract (msg, &device_name, &ext, &type_id, &subtype_id, &parent_id, &status, &priority, &operation);
    assert (r == 0 );
    assert ( zhash_lookup (ext, "qqq") != NULL );
    assert ( streq (device_name ,"ASSET_NN") );
    assert ( streq (status ,"active") );
    assert ( type_id == asset_type::RACK );
    assert ( subtype_id == asset_subtype::N_A);
    assert ( parent_id == 223);
    assert ( priority == 5);
    assert ( operation == asset_operation::INSERT);
    ymsg_destroy (&msg);
    zhash_destroy (&ext);
    zstr_free (&device_name);
    zstr_free (&status);

    mlm_client_destroy (&client);
    bios_agent_destroy (&agent);
    zactor_destroy (&la_server);
    zactor_destroy (&server);

    //  @end
    printf ("OK\n");
}
