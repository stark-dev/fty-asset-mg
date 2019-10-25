/*  =========================================================================
    topology_power_topology_power - class description

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

#ifndef TOPOLOGY_POWER_TOPOLOGY_POWER_H_INCLUDED
#define TOPOLOGY_POWER_TOPOLOGY_POWER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

//  topology_power main entry
//  PARAM map keys in from/to/filter_dc/filter_group
//  attempt: from/to: assetID, filter_dc/filter_group:<to be completed>
//  source: fty-rest src/web/src/topology_power.ecpp
//  returns 0 if success (json payload is valid), else <0

FTY_ASSET_PRIVATE int
    topology_power (std::map<std::string, std::string> & param, std::string & json);

//  @end

#ifdef __cplusplus
}
#endif

#endif
