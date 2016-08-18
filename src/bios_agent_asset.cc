/*  =========================================================================
    bios_agent_asset - Agent managing assets

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
    bios_agent_asset - Agent managing assets
@discuss
@end
*/

#include "agent_asset_classes.h"

static int
s_autoupdate_timer (zloop_t *loop, int timer_id, void *output)
{
    zstr_send (output, "WAKEUP");
    return 0;
}

int main (int argc, char *argv [])
{
    const char* endpoint = "ipc://@/malamute";

    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("bios-agent-asset [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    //  Insert main code here
    if (verbose)
        zsys_info ("bios_agent_asset - Agent managing assets");

    zactor_t *la_server = zactor_new (bios_legacy_asset_server, (void*) "legacy-assets");
    if (verbose)
        zstr_send (la_server, "VERBOSE");
    zstr_sendx (la_server, "CONNECT", endpoint, NULL);
    zstr_sendx (la_server, "PRODUCER", bios_get_stream_main (), NULL);
    zstr_sendx (la_server, "CONSUMER", "ASSETS", ".*", NULL);

    zactor_t *asset_server = zactor_new (bios_asset_server, (void*) "asset-agent");
    if (verbose)
        zstr_send (asset_server, "VERBOSE");
    zstr_sendx (asset_server, "CONNECT", endpoint, NULL);
    zsock_wait (asset_server);

    zactor_t *autoupdate_server = zactor_new (bios_asset_autoupdate_server, (void*) "asset-autoupdate");
    if (verbose)
        zstr_send (la_server, "VERBOSE");
    zstr_sendx (autoupdate_server, "CONNECT", endpoint, NULL);
    zsock_wait (autoupdate_server);
    zstr_sendx (autoupdate_server, "PRODUCER", "ASSETS", NULL);
    zstr_sendx (autoupdate_server, "CONSUMER", "ASSETS", ".*", NULL);
    zstr_sendx (autoupdate_server, "WAKEUP", NULL);

    // create regular event for autoupdate agent
    zloop_t *autoupdate_wakeup = zloop_new();
    zloop_timer (autoupdate_wakeup, 5*60*1000, 0, s_autoupdate_timer, autoupdate_server);
    zloop_start (autoupdate_wakeup);

    //  Accept and print any message back from server
    //  copy from src/malamute.c under MPL license
    zpoller_t *poller = zpoller_new (la_server, asset_server, autoupdate_server, NULL);
    while (true) {
        void *which = zpoller_wait (poller, -1);
        char *message = NULL;
        if ( which == la_server) {
            message = zstr_recv (la_server);
        }
        else if ( which == asset_server) {
            message = zstr_recv (asset_server);
        }
        else if ( which == autoupdate_server) {
            message = zstr_recv (autoupdate_server);
        }
        if (message) {
            puts (message);
            free (message);
        }
        else {
            puts ("interrupted");
            break;
        }
    }
    zloop_destroy (&autoupdate_wakeup);
    zpoller_destroy (&poller);
    zactor_destroy (&autoupdate_server);
    zactor_destroy (&asset_server);
    zactor_destroy (&la_server);
    return 0;
}
