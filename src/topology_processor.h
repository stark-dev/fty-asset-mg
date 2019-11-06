/*  =========================================================================
    topology_processor - Topology requests processor

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

#ifndef TOPOLOGY_PROCESSOR_H_INCLUDED
#define TOPOLOGY_PROCESSOR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

// Retrieve the powerchains which powers a requested target asset
// Implementation of REST /api/v1/topology/power?[from/to/filter_dc/filter_group] (see RFC11)
// COMMAND is in {"from", "to", "filter_dc", "filter_group"} tokens set
// ASSETNAME is the subject of the command
// On success, RESULT is valid (JSON payload)
// Returns 0 if success, else <0

FTY_ASSET_PRIVATE int
    topology_power_process (const std::string & command, const std::string & assetName, std::string & result, bool beautify = true);

// Retrieve the closest powerchain which powers a requested target asset
// implementation of REST /api/v1/topology/power?to (see RFC11) **filtered** on dst-id == assetName
// ASSETNAME is the target asset
// On success, RESULT is valid (JSON payload)
// Returns 0 if success, else <0

FTY_ASSET_PRIVATE int
    topology_power_to (const std::string & assetName, std::string & result, bool beautify = true);

// Retrieve location topology for a requested target asset
// Implementation of REST /api/v1/topology/location?[from/to] (see RFC11)
// COMMAND is in {"to", "from"} tokens set
// ASSETNAME is the subject of the command (can be "none" if command is "from")
// OPTIONS:
//    if 'to'  : must be empty (no option allowed)
//    if 'from': json payload as { "recursive": <true|false>, "filter": <element_kind>, "feed_by": <asset_id> }
//               where <element_kind> is in {"rooms" , "rows", "racks", "devices", "groups"}
//               if "feed_by" is specified, "filter" *must* be set to "devices", ASSETNAME *must* not be set to "none"
//               defaults are {'recursive': false, "filter": "", "feed_by": "" }
// On success, RESULT is valid (JSON payload)
// Returns 0 if success, else <0

FTY_ASSET_PRIVATE int
    topology_location_process (const std::string & command, const std::string & assetName, const std::string & options, std::string & result, bool beautify = true);

//  Self test of this class

FTY_ASSET_PRIVATE void
    topology_processor_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
