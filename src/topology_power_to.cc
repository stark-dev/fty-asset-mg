/*  =========================================================================
    topology_power_to - Retrieve the full or closest power chain which powers a requested target asset

    Copyright (C) 2014 - 2018 Eaton

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
    topology_power_to - Retrieve the closest powerchain which powers a requested target asset
@discuss
@end
*/

#include "fty_asset_classes.h"

// implementation of REST /api/v1/topology/power?to (see RFC11)
// filtered on dst-id == assetName
// returns 0 if success, else <0

int topology_power_to (const std::string & assetName, cxxtools::SerializationInfo& siResult)
{
    siResult.clear();

    std::map<std::string, std::string> param;
    std::string sResult; // JSON payload
    param["to"] = assetName;
    int r = topology_power (param, sResult);
    if (r != 0) {
        log_error("topology_power failed, r: %d", r);
        return -1;
    }

    //log_trace("topology_power() success, result: \n%s", sResult.c_str());

    // deserialize result to si
    cxxtools::SerializationInfo si;
    try {
        std::istringstream input(sResult);
        cxxtools::JsonDeserializer deserializer(input);
        deserializer.deserialize(si);
    }
    catch (...) {
        log_error("json deserialize failed (exception reached), payload:\n%s", sResult.c_str());
        return -2;
    }

    cxxtools::SerializationInfo *si_powerchains = si.findMember("powerchains");
    if (si_powerchains == 0) {
        log_error("powerchains member not defined, payload:\n%s", sResult.c_str());
        return -3;
    }
    if (si_powerchains->category() != cxxtools::SerializationInfo::Category::Array) {
        log_error("powerchains member category != Array, payload:\n%s", sResult.c_str());
        return -4;
    }

    // prepare result (asset-id member + powerchains array member)
    siResult.addMember("asset-id") <<= assetName;
    siResult.addMember("powerchains").setCategory(cxxtools::SerializationInfo::Array);
    cxxtools::SerializationInfo *siResulPowerchains = siResult.findMember("powerchains");
    if (siResulPowerchains == 0) {
        log_error("powerchains member creation failed");
        return -5;
    }

    // powerchains member is defined (array), loop on items...
    // filter on dst-id == assetName, check src-id & src-socket,
    // keep entries in siResulPowerchains
    cxxtools::SerializationInfo::Iterator it;
    for (it = si_powerchains->begin(); it != si_powerchains->end(); ++it) {
        cxxtools::SerializationInfo *xsi = &(*it);
        if (xsi == 0) continue; //

        cxxtools::SerializationInfo *si_dstId = xsi->findMember("dst-id");
        if (si_dstId == 0 || si_dstId->isNull())
            { log_error("dst-id member is missing"); continue; }
        std::string dstId;
        si_dstId->getValue(dstId);
        if (dstId != assetName) continue;

        cxxtools::SerializationInfo *si_srcId, *si_srcSock;
        si_srcId = xsi->findMember("src-id");
        if (si_srcId == 0 || si_srcId->isNull())
            { log_error("src-id member is missing"); continue; }
        si_srcSock = xsi->findMember("src-socket");
        if (si_srcSock == 0 || si_srcSock->isNull())
            { log_error("src-socket member is missing"); continue; }

        // keep that entry
        siResulPowerchains->addMember("") <<= (*xsi);
    }

    log_debug("topology_power_to() success");
    return 0; // ok
}

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

//#define __local_selftest__
#undef __local_selftest__

#ifdef __local_selftest__
//  dump SI to string (JSON payload)
static std::string selftest_si_to_string (cxxtools::SerializationInfo& si)
{
    try {
        std::ostringstream output;
        cxxtools::JsonSerializer s;
        s.beautify(true);
        s.begin(output);
        s.serialize(si);
        s.finish();
        return output.str();
    }
    catch (...) {
        log_error("Exception reached");
    }
    return "";
}
#endif

void
topology_power_to_test (bool verbose)
{
    printf (" * topology_power_to: \n");
    //  @selftest

    printf("test empty param\n");
    {
        cxxtools::SerializationInfo si;
        int r = topology_power_to("", si);
        assert(r != 0);
    }

#if 0
    // disabled
    // local memcheck is ok (no memleak), but Jenkins get some tntdb memleaks
    printf("tests bad param\n");
    {
        cxxtools::SerializationInfo si;
        int r = topology_power_to("bad_asset_name", si);
        assert(r != 0);
    }
#endif

#ifdef __local_selftest__
#pragma message "=== __local_selftest__ ==="

    // **run** this as admin (DB access rights)
    printf("local test on powered asset\n");
    {
        // assume server-17 asset exists, powered by two epdu's
        // see README.md example
        cxxtools::SerializationInfo si;
        int r = topology_power_to("server-17", si);
        assert(r == 0);
        std::string json = selftest_si_to_string(si);
        printf("json:\n%s\n", json.c_str());
    }
#endif

    //  @end
    printf ("topology_power_to: OK\n");
}
