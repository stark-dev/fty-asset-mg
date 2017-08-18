/*  =========================================================================
    fty_asset - Agent managing assets

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
    fty_asset - Agent managing assets
@discuss
@end
*/

#include "fty_asset_classes.h"

static int
s_autoupdate_timer (zloop_t *loop, int timer_id, void *output)
{
    zstr_send (output, "WAKEUP");
    return 0;
}

static int
s_repeat_assets_timer (zloop_t *loop, int timer_id, void *output)
{
    zstr_send (output, "REPEAT_ALL");
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
            puts ("fty-asset [options] ...");
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
        }
    }
    if (verbose)
        zsys_info ("fty_asset - Agent managing assets");

    zactor_t *asset_server = zactor_new (fty_asset_server, (void*) "asset-agent");
    if (verbose)
        zstr_send (asset_server, "VERBOSE");
    zstr_sendx (asset_server, "CONNECTSTREAM", endpoint, NULL);
    zsock_wait (asset_server);
    zstr_sendx (asset_server, "PRODUCER", "ASSETS", NULL);
    zsock_wait (asset_server);
    zstr_sendx (asset_server, "CONSUMER", "ASSETS", ".*", NULL);
    zsock_wait (asset_server);
    zstr_sendx (asset_server, "CONNECTMAILBOX", endpoint, NULL);
    zsock_wait (asset_server);
    zstr_sendx (asset_server, "REPEAT_ALL", NULL);

    zactor_t *autoupdate_server = zactor_new (fty_asset_autoupdate_server, (void*) "asset-autoupdate");
    if (verbose)
        zstr_send (autoupdate_server, "VERBOSE");
    zstr_sendx (autoupdate_server, "CONNECT", endpoint, NULL);
    zsock_wait (autoupdate_server);
    zstr_sendx (autoupdate_server, "PRODUCER", "ASSETS", NULL);
    zsock_wait (autoupdate_server);
    zstr_sendx (autoupdate_server, "WAKEUP", NULL);

    // set up how ofter assets should be repeated
    char *repeat_interval = getenv("BIOS_ASSETS_REPEAT");
    int repeat_interval_s = repeat_interval ? std::stoi (repeat_interval) : 60*60;

    zactor_t *inventory_server = zactor_new (fty_asset_inventory_server, (void*) "asset-inventory");
    zstr_sendx (inventory_server, "CONNECT", endpoint, NULL);
    zsock_wait (inventory_server);
    zstr_sendx (inventory_server, "CONSUMER", "ASSETS", "inventory@.*", NULL);

    // create regular event for autoupdate agent
    zloop_t *loop = zloop_new();
    // once in 5 minutes
    zloop_timer (loop, 5*60*1000, 0, s_autoupdate_timer, autoupdate_server);
    // every repeat_interval_s
    zloop_timer (loop, repeat_interval_s * 1000, 0, s_repeat_assets_timer, asset_server);
    zloop_start (loop);
    // zloop_start takes ownership of this thread! and waits for interrupt!
    zloop_destroy (&loop);
    zactor_destroy (&inventory_server);
    zactor_destroy (&autoupdate_server);
    zactor_destroy (&asset_server);
    return 0;
}
