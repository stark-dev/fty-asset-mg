/*  =========================================================================
    fty_asset_server - Asset server, that takes care about distribution of asset information across the system

    Copyright (C) 2014 - 2017 Eaton

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
    fty_asset_server - Asset server, that takes care about distribution of asset information across the system
@discuss
     ASSET PROTOCOL

     ------------------------------------------------------------------------
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


    ------------------------------------------------------------------------
    ## Asset manipulation protocol

    REQ:
        subject: "ASSET_MANIPULATION"
        Message is a fty protocol (fty_proto_t) message

        *) fty_proto ASSET message

        where 'operation' is one of [ create | update | delete | retire ].
        Asset messages with different operation value are discarded and not replied to.

    REP:
        subject: same as in REQ
        Message is a multipart string message:

        * OK/<asset_id>
        * ERROR/<reason>

        where:
 (OPTIONAL)     <asset_id>  = asset id (in case of create, update operation)
                <reason>    = Error message/code TODO

    Note: in REQ message certain asset information are encoded as follows

      'ext' field
          Power Links - key: "power_link.<device_name>", value: "<first_outlet_num>/<second_outlet_num>", i.e. 1 --> 2 == "1/2"
          Groups - key: "group", value: "<group_name_1>/.../<group_name_N>"


    ------------------------------------------------------------------------
    ## ASSETS in container

    REQ:
        subject: "ASSETS_IN_CONTAINER"
        Message is a multipart string message

        * GET/<container name>/<type 1>/.../<type n>

        where:
            <container name>        = Name of the container things belongs to that
                                      when empty, no container is used, so all assets are take in account
            <type X>                = Type or subtype to be returned. Possible values are
                                      ups
                                      TODO: add more
                                      when empty, no filtering is done
    REP:
        subject: "ASSETS_IN_CONTAINER"
        Message is a multipart message:

        * OK                         = empty container
        * OK/<asset 1>/.../<asset N> = non-empty
        * ERROR/<reason>

        where:
            <reason>          = ASSET_NOT_FOUND / INTERNAL_ERROR / BAD_COMMAND

    REQ:
        subject: "ASSETS"
        Message is a multipart string message

        * GET/<type 1>/.../<type n>

        where:
            <type X>                = Type or subtype to be returned. Possible values are
                                      ups
                                      TODO: add more
                                      when empty, no filtering is done
    REP:
        subject: "ASSETS"
        Message is a multipart message:

        * OK                         = empty container
        * OK/<asset 1>/.../<asset N> = non-empty
        * ERROR/<reason>

        where:
            <reason>          = ASSET_NOT_FOUND / INTERNAL_ERROR / BAD_COMMAND
    ------------------------------------------------------------------------
    ## REPUBLISH

    REQ:
        subject: "REPUBLISH"
        Message is a multipart string message

        /asset1/asset2/asset3       - republish asset information about asset1 asset2 and asset3
        /$all                       - republish information about all assets

@end
*/

#include "fty_asset_classes.h"
#include <string>
#include <tntdb/connect.h>
#include <functional>
// TODO add dependencies tntdb and cxxtools

//  Structure of our class

struct _fty_asset_server_t {
    bool verbose;
    char *name;
    mlm_client_t *client;
};


//  --------------------------------------------------------------------------
//  Destroy the fty_asset_server

void
fty_asset_server_destroy (fty_asset_server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        fty_asset_server_t *self = *self_p;
        zstr_free (&self->name);
        mlm_client_destroy (&self->client);
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Create a new fty_asset_server

fty_asset_server_t *
fty_asset_server_new (void)
{
    fty_asset_server_t *self = (fty_asset_server_t *) zmalloc (sizeof (fty_asset_server_t));
    assert (self);
    self->client = mlm_client_new ();
    if (self->client) {
        self->verbose = false;
    }
    else {
        fty_asset_server_destroy (&self);
    }
    return self;
}


// ============================================================
//         Functionality for TOPOLOGY processing
// ============================================================
static void
    s_processTopology(
        fty_asset_server_t *cfg,
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
        zsys_error ("%s:\tTOPOLOGY_POWER: Cannot select power sources", cfg->name);
        zmsg_addstr (msg, "ERROR");
        zmsg_addstr (msg, "INTERNAL_ERROR");
    } else if ( rv == -2 ) {
        zsys_debug ("%s:\tTOPOLOGY_POWER: Asset was not found", cfg->name);
        zmsg_addstr (msg, "ERROR");
        zmsg_addstr (msg, "ASSET_NOT_FOUND");
    } else {
        zmsg_addstr (msg, "OK");
        if ( cfg->verbose )
            zsys_debug ("%s:\tPower topology for '%s':", cfg->name, assetName.c_str());
        for (const auto &powerDeviceName : powerDevices ) {
            if ( cfg->verbose )
                zsys_debug ("%s:\t\t%s", cfg->name, powerDeviceName.c_str());
            zmsg_addstr (msg, powerDeviceName.c_str());
        }
    }
    rv = mlm_client_sendto (cfg->client, mlm_client_sender (cfg->client), "TOPOLOGY", NULL, 5000, &msg);
    if ( rv != 0 )
        zsys_error ("%s:\tTOPOLOGY_POWER: cannot send response message", cfg->name);
}

static void
    s_handle_subject_topology(
        fty_asset_server_t *cfg,
        zmsg_t *zmessage)
{
    assert (zmessage);
    assert (cfg);
    char* command = zmsg_popstr (zmessage);
    if ( streq (command, "TOPOLOGY_POWER") ) {
        char* asset_name = zmsg_popstr (zmessage);
        s_processTopology (cfg, asset_name);
        zstr_free (&asset_name);
    }
    else
        zsys_error ("%s:\tUnknown command for subject=TOPOLOGY '%s'", cfg->name, command);
    zstr_free (&command);
}

static void
    s_handle_subject_assets_in_container (
        fty_asset_server_t *cfg,
        zmsg_t *msg)
{
    assert (msg);
    assert (cfg);
    if (zmsg_size (msg) < 2) {
        zsys_error ("%s:\tASSETS_IN_CONTAINER: incoming message have less than 2 frames", cfg->name);
        return;
    }

    char* c_command = zmsg_popstr (msg);
    zmsg_t *reply = zmsg_new ();

    if (! streq (c_command, "GET")) {
        zsys_error ("%s:\tASSETS_IN_CONTAINER: bad command '%s', expected GET", cfg->name, c_command);
        zmsg_addstr (reply, "ERROR");
        zmsg_addstr (reply, "BAD_COMMAND");
    }
    zstr_free (&c_command);

    std::string container_name;
    char* c_container_name = zmsg_popstr (msg);
    container_name = c_container_name;
    zstr_free (&c_container_name);

    std::set <std::string> filters;
    while (zmsg_size (msg) > 0) {
        char *filter = zmsg_popstr (msg);
        filters.insert (filter);
        zstr_free (&filter);
    }
    std::vector <std::string> assets;
    int rv = 0;

    // if there is no error msg prepared, call SQL
    if (zmsg_size (msg) == 0)
            rv = select_assets_by_container (container_name, filters, assets);

    if (rv == -1)
    {
        zmsg_addstr (reply, "ERROR");
        zmsg_addstr (reply, "INTERNAL_ERROR");
    }
    else
    if (rv == -2)
    {
        zmsg_addstr (reply, "ERROR");
        zmsg_addstr (reply, "ASSET_NOT_FOUND");
    }
    else
    {
        zmsg_addstr (reply, "OK");
        for (const auto& dev : assets)
            zmsg_addstr (reply, dev.c_str ());
    }

    // send the reply
    rv = mlm_client_sendto (cfg->client, mlm_client_sender (cfg->client), "ASSETS_IN_CONTAINER", NULL, 5000, &reply);
    if (rv == -1)
        zsys_error ("%s:\tASSETS_IN_CONTAINER: mlm_client_sendto failed", cfg->name);

}



static void
    s_handle_subject_assets (
        fty_asset_server_t *cfg,
        zmsg_t *msg)
{
    assert (msg);
    assert (cfg);

    char* c_command = zmsg_popstr (msg);
    zmsg_t *reply = zmsg_new ();

    if (! streq (c_command, "GET")) {
        zsys_error ("%s:\tASSETS: bad command '%s', expected GET", cfg->name, c_command);
        zmsg_addstr (reply, "ERROR");
        zmsg_addstr (reply, "BAD_COMMAND");
    }
    zstr_free (&c_command);

    std::set <std::string> filters;
    while (zmsg_size (msg) > 0) {
        char *filter = zmsg_popstr (msg);
        filters.insert (filter);
        zstr_free (&filter);
    }
    std::vector <std::string> assets;
    int rv = 0;

    // if there is no error msg prepared, call SQL
    if (zmsg_size (msg) == 0){
            rv = select_assets_by_filter(filters, assets);
    }

    if (rv == -1)
    {
        zmsg_addstr (reply, "ERROR");
        zmsg_addstr (reply, "INTERNAL_ERROR");
    }
    else
    if (rv == -2)
    {
        zmsg_addstr (reply, "ERROR");
        zmsg_addstr (reply, "ASSET_NOT_FOUND");
    }
    else
    {
        zmsg_addstr (reply, "OK");
        for (const auto& dev : assets)
            zmsg_addstr (reply, dev.c_str ());
    }

    // send the reply
    rv = mlm_client_sendto (cfg->client, mlm_client_sender (cfg->client), "ASSETS", NULL, 5000, &reply);
    if (rv == -1)
        zsys_error ("%s:\tASSETS: mlm_client_sendto failed", cfg->name);

}

static void
    s_update_asset (
        fty_asset_server_t *cfg,
        const std::string &asset_name)
{
    assert (cfg);
    zhash_t *aux = zhash_new ();
    zhash_autofree (aux);
    uint32_t asset_id = 0;

    std::function<void(const tntdb::Row&)> cb1 = \
        [aux, &asset_id, asset_name](const tntdb::Row &row)
        {
            int foo_i = 0;
            row ["priority"].get (foo_i);
            zhash_insert (aux, "priority", (void*) std::to_string (foo_i).c_str());

            foo_i = 0;
            row ["id_type"].get (foo_i);
            zhash_insert (aux, "type", (void*) asset_type2str (foo_i));

            // additional aux items (requiered by uptime)
            if (streq (asset_type2str (foo_i), "datacenter"))
            {
                if (!insert_upses_to_aux (aux, asset_name))
                    zsys_error ("insert_upses_to_aux: failed to insert upses");
            }
            foo_i = 0;
            row ["subtype_id"].get (foo_i);
            zhash_insert (aux, "subtype", (void*) asset_subtype2str (foo_i));

            foo_i = 0;
            row ["id_parent"].get (foo_i);
            zhash_insert (aux, "parent", (void*) std::to_string (foo_i).c_str());

            std::string foo_s;
            row ["status"].get (foo_s);
            zhash_insert (aux, "status", (void*) foo_s.c_str());

            row ["id"].get (asset_id);
        };

    // select basic info
    int rv = select_asset_element_basic (asset_name, cb1);
    if ( rv != 0 ) {
        zsys_warning ("%s:\tCannot select info about '%s'", cfg->name, asset_name.c_str());
        zhash_destroy (&aux);
        return;
    }

    zhash_t *ext = zhash_new ();
    zhash_autofree (ext);

    std::function<void(const tntdb::Row&)> cb2 = \
        [ext](const tntdb::Row &row)
            {
                std::string keytag;
                row ["keytag"].get (keytag);
                std::string value;
                row ["value"].get (value);
                zhash_insert (ext, keytag.c_str(), (void*) value.c_str());
            };
    // select ext attributes
    rv = select_ext_attributes (asset_id, cb2);
    if ( rv != 0 ) {
        zsys_warning ("%s:\tCannot select ext attributes for '%s'", cfg->name, asset_name.c_str());
        zhash_destroy (&aux);
        zhash_destroy (&ext);
        return;
    }

    // create uuid ext attribute if missing
    if (! zhash_lookup (ext, "uuid") ) {
        const char *serial = (const char *) zhash_lookup (ext, "serial_no");
        const char *model = (const char *) zhash_lookup (ext, "model");
        const char *mfr = (const char *) zhash_lookup (ext, "manufacturer");
        const char *type = (const char *) zhash_lookup (aux, "type");
        if (! type) type = "";
        fty_uuid_t *uuid = fty_uuid_new ();
        zhash_t *ext_new = zhash_new ();

        if (serial && model && mfr) {
            // we have all information => create uuid
            const char *uuid_new = fty_uuid_calculate (uuid, mfr, model, serial);
            zhash_insert (ext, "uuid", (void *) uuid_new);
            zhash_insert (ext_new, "uuid", (void *) uuid_new);
            process_insert_inventory (asset_name.c_str (), ext_new);
        } else {
            if (streq (type, "device")) {
                // it is device, put FFF... and wait for information
                zhash_insert (ext, "uuid", (void *) fty_uuid_calculate (uuid, NULL, NULL, NULL));
            } else {
                // it is not device, we will probably not get more information
                // lets generate random uuid and save it
                const char *uuid_new = fty_uuid_generate (uuid);
                zhash_insert (ext, "uuid", (void *) uuid_new);
                zhash_insert (ext_new, "uuid", (void *) uuid_new);
                process_insert_inventory (asset_name.c_str (), ext_new);
            }
        }
        fty_uuid_destroy (&uuid);
        zhash_destroy (&ext_new);
    }

    std::function<void(const tntdb::Row&)> cb3 = \
        [aux](const tntdb::Row &row)
        {
            for (const auto& name: {"parent_name1", "parent_name2", "parent_name3", "parent_name4", "parent_name5"}) {
                std::string foo;
                row [name].get (foo);
                std::string hash_name = name;
                //                11 == strlen ("parent_name")
                hash_name.insert (11, 1, '.');
                if (!foo.empty ())
                    zhash_insert (aux, hash_name.c_str (), (void*) foo.c_str ());
            }
        };
    // select "physical topology"
    rv = select_asset_element_super_parent (asset_id, cb3);
    if (rv != 0) {
        zhash_destroy (&aux);
        zhash_destroy (&ext);
        zsys_error ("%s:\tselect_asset_element_super_parent ('%s') failed.", cfg->name, asset_name.c_str());
        return;
    }
    // other information like, groups, power chain for now are not included in the message
    std::string subject;
    subject = (const char*) zhash_lookup (aux, "type");
    subject.append (".");
    subject.append ((const char*)zhash_lookup (aux, "subtype"));
    subject.append ("@");
    subject.append (asset_name);

    zmsg_t *msg = fty_proto_encode_asset (
            aux,
            asset_name.c_str(),
            FTY_PROTO_ASSET_OP_UPDATE,
            ext);
    rv = mlm_client_send (cfg->client, subject.c_str(), &msg);
    zhash_destroy (&ext);
    zhash_destroy (&aux);
    if ( rv != 0 ) {
        zsys_error ("%s:\tmlm_client_send failed for asset '%s'", cfg->name, asset_name.c_str());
        return;
    }
}

static void
    s_update_topology(
        fty_asset_server_t *cfg,
        fty_proto_t *msg)
{
    assert (msg);
    assert (cfg);

    if ( !streq (fty_proto_operation (msg),FTY_PROTO_ASSET_OP_UPDATE)) {
        zsys_info ("%s:\tIgnore: '%s' on '%s'", cfg->name, fty_proto_operation(msg), fty_proto_name (msg));
        return;
    }
    // select assets, that were affected by the change
    std::set<std::string> empty;
    std::vector <std::string> asset_names;
    int rv = select_assets_by_container (fty_proto_name (msg), empty, asset_names);
    if ( rv != 0 ) {
        zsys_warning ("%s:\tCannot select assets in container '%s'", cfg->name, fty_proto_name (msg));
        return;
    }

    // For every asset we need to form new message!
    for ( const auto &asset_name : asset_names ) {
        s_update_asset (cfg, asset_name);
    }
}

static void
s_repeat_all (fty_asset_server_t *cfg, const std::set<std::string>& assets_to_publish)
{
    assert (cfg);

    std::vector <std::string> asset_names;
    std::function<void(const tntdb::Row&)> cb = \
        [&asset_names, &assets_to_publish](const tntdb::Row &row)
        {
            std::string foo;
            row ["name"].get (foo);
            if (assets_to_publish.size () == 0)
                asset_names.push_back (foo);
            else
            if (assets_to_publish.count (foo) == 1)
                asset_names.push_back (foo);
        };

    // select all assets
    int rv = select_assets (cb);
    if ( rv != 0 ) {
        zsys_warning ("%s:\tCannot list all assets", cfg->name);
        return;
    }

    // For every asset we need to form new message!
    for ( const auto &asset_name : asset_names ) {
        s_update_asset (cfg, asset_name);
    }
}

static void
s_repeat_all (fty_asset_server_t *cfg)
{
    return s_repeat_all (cfg, {});
}

void
fty_asset_server (zsock_t *pipe, void *args)
{
    assert (pipe);
    assert (args);

    fty_asset_server_t *cfg = fty_asset_server_new ();
    assert (cfg);
    cfg->name = strdup ((char*) args);
    assert (cfg->name);

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe(cfg->client), NULL);
    assert (poller);

    // Signal need to be send as it is required by "actor_new"
    zsock_signal (pipe, 0);
    zsys_info ("%s:\tStarted", cfg->name);

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
            if ( cfg->verbose ) {
                zsys_debug ("%s:\tActor command=%s", cfg->name, cmd);
            }

            if (streq (cmd, "$TERM")) {
                if ( !cfg->verbose ) // ! is here intentionally, to get rid of duplication information
                    zsys_info ("%s:\tGot $TERM", cfg->name);
                zstr_free (&cmd);
                zmsg_destroy (&msg);
                goto exit;
            }
            else
            if (streq (cmd, "VERBOSE")) {
                cfg->verbose = true;
            }
            else
            if (streq (cmd, "CONNECT")) {
                char* endpoint = zmsg_popstr (msg);
                int rv = mlm_client_connect (cfg->client, endpoint, 1000, cfg->name);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't connect to malamute endpoint '%s'", cfg->name, endpoint);
                }
                zstr_free (&endpoint);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "PRODUCER")) {
                char* stream = zmsg_popstr (msg);
                int rv = mlm_client_set_producer (cfg->client, stream);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't set producer on stream '%s'", cfg->name, stream);
                }
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "CONSUMER")) {
                char* stream = zmsg_popstr (msg);
                char* pattern = zmsg_popstr (msg);
                int rv = mlm_client_set_consumer (cfg->client, stream, pattern);
                if (rv == -1) {
                    zsys_error ("%s:\tCan't set consumer on stream '%s', '%s'", cfg->name, stream, pattern);
                }
                zstr_free (&pattern);
                zstr_free (&stream);
                zsock_signal (pipe, 0);
            }
            else
            if (streq (cmd, "REPEAT_ALL")) {
                if ( cfg->verbose )
                    zsys_debug ("%s:\tREPEAT_ALL start", cfg->name);
                s_repeat_all (cfg);
                if ( cfg->verbose )
                    zsys_debug ("%s:\tREPEAT_ALL end", cfg->name);
            }
            else
            {
                zsys_info ("%s:\tUnhandled command %s", cfg->name, cmd);
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
            continue;
        }

        // This agent is a reactive agent, it reacts only on messages
        // and doesn't do anything if there are no messages
        zmsg_t *zmessage = mlm_client_recv (cfg->client);
        if ( zmessage == NULL ) {
            continue;
        }
        std::string subject = mlm_client_subject (cfg->client);
        std::string command = mlm_client_command (cfg->client);
        if ( cfg->verbose )
            zsys_debug("%s:\tGot message subject='%s', command='%s'", cfg->name, subject.c_str (), command.c_str ());

        if (command == "STREAM DELIVER") {
            if ( is_fty_proto (zmessage) ) {
                fty_proto_t *bmsg = fty_proto_decode (&zmessage);
                if ( fty_proto_id (bmsg) == FTY_PROTO_ASSET ) {
                    s_update_topology (cfg, bmsg);
                }
                fty_proto_destroy (&bmsg);
            }
            else {
                // DO NOTHING for now
            }
        }
        else
        if (command == "MAILBOX DELIVER") {
            if (subject == "TOPOLOGY")
                s_handle_subject_topology (cfg, zmessage);
            else
            if (subject == "ASSETS_IN_CONTAINER")
                s_handle_subject_assets_in_container (cfg, zmessage);
            else
            if (subject == "ASSETS")
                s_handle_subject_assets (cfg, zmessage);
            else
            if (subject == "REPUBLISH") {

                zmsg_print (zmessage);
                char *asset = zmsg_popstr (zmessage);
                if (!asset || streq (asset, "$all"))
                    s_repeat_all (cfg);
                else {
                    std::set <std::string> assets_to_publish;
                    while (asset) {
                        assets_to_publish.insert (asset);
                        zstr_free (&asset);
                        asset = zmsg_popstr (zmessage);
                    }
                    s_repeat_all (cfg, assets_to_publish);
                }
            }
            else
                zsys_info ("%s:\tUnexpected subject '%s'", cfg->name, subject.c_str ());
        }
        else {
            // DO NOTHING for now
        }
        zmsg_destroy (&zmessage);
    }
exit:
    zsys_info ("%s:\tended", cfg->name);
    //TODO:  save info to persistence before I die
    zpoller_destroy (&poller);
    fty_asset_server_destroy (&cfg);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_asset_server_test (bool verbose)
{
    printf (" * fty_asset_server: ");

    //  @selftest
    //  Simple create/destroy test
    fty_asset_server_t *self = fty_asset_server_new ();
    assert (self);
    fty_asset_server_destroy (&self);
    // Everything is commented out because: need to have a proper database
    // --> move it out of "make check"
    //  @selftest
    static const char* endpoint = "inproc://fty_asset_server-test";

    // malamute broker
    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    assert ( server != NULL );

    zstr_sendx (server, "BIND", endpoint, NULL);

    // NOT legacy assets
    zactor_t *la_server = zactor_new (fty_asset_server, (void*)"AGENT_ASSET");
    if (verbose) {
        zstr_send (la_server, "VERBOSE");
    }
    zstr_sendx (la_server, "CONNECT", endpoint, NULL);
    zsock_wait (la_server);

    mlm_client_t *client = mlm_client_new ();
    mlm_client_connect (client, endpoint, 5000, "topology-peer");
    // scenario name
    std::string scenario;

    // all topology messages has the same subject

    /*
    static const char* subject = "TOPOLOGY";

    // ====================================================
    scenario = "scenario 1";
    zsys_info ("# %s:", scenario.c_str());
    //      prepare and send request message
    zmsg_t *msg = zmsg_new();
    zmsg_addstr (msg, "TOPOLOGY_POWER");
    zmsg_addstr (msg, "RACK1-LAB");
    mlm_client_sendto (client, "AGENT_ASSET", subject, NULL, 5000, &msg);

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
    mlm_client_sendto (client, "AGENT_ASSET", subject, NULL, 5000, &msg);

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
    */

    // commented out - test doesnt work
    // // scenario3 ASSETS_IN_CONTAINER
    // mlm_client_sendtox (client, "AGENT_ASSET", "ASSETS_IN_CONTAINER", "GET", "DC007", NULL);

    // char *recv_subject, *reply, *reason;
    // mlm_client_recvx (client, &recv_subject, &reply, &reason, NULL);

    // assert (streq (recv_subject, "ASSETS_IN_CONTAINER"));
    // assert (streq (reply, "ERROR"));
    // assert (streq (reason, "INTERNAL_ERROR"));

    // zstr_free (&recv_subject);
    // zstr_free (&reply);
    // zstr_free (&reason);

    // selftest should clear after itself
    mlm_client_destroy (&client);
    zactor_destroy (&la_server);

    zactor_destroy (&server);

    //  @end
    printf ("OK\n");
}
