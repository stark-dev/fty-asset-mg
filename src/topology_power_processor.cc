/*  =========================================================================
    topology_power_processor - Topology power requests processor

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
    topology_power_processor - Topology power requests processor
@discuss
@end
*/

#include "fty_asset_classes.h"

// fwd decl.
static int si_to_string (const cxxtools::SerializationInfo & si, std::string & s);
static int string_to_si (const std::string & s, cxxtools::SerializationInfo & si);

// --------------------------------------------------------------------------
// Retrieve the powerchains which powers a requested target asset
// Implementation of REST /api/v1/topology/power?[from/to/filter_dc/filter_group] (see RFC11)
// COMMAND is in {"from", "to", "filter_dc", "filter_group"} tokens set
// ASSETNAME is the subject of the command
// On success, RESULT is valid (JSON payload)
// Returns 0 if success, else <0

int topology_power_process (const std::string & command, const std::string & assetName, std::string & result, bool beautify)
{
    result = "";

    std::map<std::string, std::string> param;
    param[command] = assetName;
    int r = topology_power (param, result);
    if (r != 0) {
        log_error("topology_power() failed, r: %d, command: %s, assetName: %s",
            r, command.c_str(), assetName.c_str());
        return -1;
    }

    // beautify result, optional
    if (beautify) {
        cxxtools::SerializationInfo si;
        std::string beauty;
        r = string_to_si (result, si);
        if (r == 0) r = si_to_string (si, beauty);

        if (r != 0) {
            log_error("beautification failed, r: %d, result: \n%s", r, result.c_str());
            return -2;
        }

        result = beauty;
    }

    log_debug("topology_power_process() success, command: %s, assetName: %s, result:\n%s",
        command.c_str(), assetName.c_str(), result.c_str());

    return 0; // ok
}

// --------------------------------------------------------------------------
// Retrieve the closest powerchain which powers a requested target asset
// implementation of REST /api/v1/topology/power?to (see RFC11) **filtered** on dst-id == assetName
// ASSETNAME is the target asset
// On success, RESULT is valid (JSON payload)
// Returns 0 if success, else <0

int topology_power_to (const std::string & assetName, std::string & result, bool beautify)
{
    result = "";

    int r = topology_power_process("to", assetName, result, beautify);
    if (r != 0) {
        log_error("topology_power_process 'to' failed, r: %d", r);
        return -1;
    }

    // deserialize result to si
    cxxtools::SerializationInfo si;
    r = string_to_si(result, si);
    if (r != 0) {
        log_error("json deserialize failed (exception reached), payload:\n%s", result.c_str());
        return -2;
    }

    // check si structure
    cxxtools::SerializationInfo *si_powerchains = si.findMember("powerchains");
    if (si_powerchains == 0) {
        log_error("powerchains member not defined, payload:\n%s", result.c_str());
        return -3;
    }
    if (si_powerchains->category() != cxxtools::SerializationInfo::Category::Array) {
        log_error("powerchains member category != Array, payload:\n%s", result.c_str());
        return -4;
    }

    result = ""; // useless, we work now on siResult

    // prepare siResult (asset-id member + powerchains array member)
    cxxtools::SerializationInfo siResult;
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
        if (dstId != assetName) continue; // filtered

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

    // dump siResult to result
    r = si_to_string(siResult, result);
    if (r != 0) {
        log_error("json serialize failed (exception reached)");
        return -6;
    }

    log_debug("topology_power_to() success, assetName: %s, result:\n%s",
        assetName.c_str(), result.c_str());

    return 0; // ok
}

//  --------------------------------------------------------------------------
//  cxxtools, dump SI to S string

static int si_to_string (const cxxtools::SerializationInfo & si, std::string & s)
{
    try {
        s = "";
        std::ostringstream output;
        cxxtools::JsonSerializer js;
        js.beautify(true);
        js.begin(output);
        js.serialize(si);
        js.finish();
        s = output.str();
        return 0; // ok
    }
    catch (...) {
        log_error("Exception reached");
    }
    return -1;
}

//  --------------------------------------------------------------------------
//  cxxtools, set SI from S string

static int string_to_si (const std::string & s, cxxtools::SerializationInfo & si)
{
    try {
        si.clear();
        std::istringstream input(s);
        cxxtools::JsonDeserializer deserializer(input);
        deserializer.deserialize(si);
        return 0; // ok
    }
    catch (...) {
        log_error("Exception reached");
    }
    return -1;
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

void
topology_power_processor_test (bool verbose)
{
    printf (" * topology_power_processor: \n");

    //  @selftest
    //  @end

    printf ("topology_power_processor: OK\n");
}
