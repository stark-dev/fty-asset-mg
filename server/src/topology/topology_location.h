/*  =========================================================================
    topology_topology_location - class description

    Copyright (C) 2014 - 2020 Eaton

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

#ifndef TOPOLOGY_TOPOLOGY_LOCATION_H_INCLUDED
#define TOPOLOGY_TOPOLOGY_LOCATION_H_INCLUDED

#include <map>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

//  topology_location main entry
//  PARAM map keys in from/to with recursive/filter/feed_by specific arguments
//  attempt: from/to: assetID, recursive in 'true'/'false', filter asset, feed_by device
//  source: fty-rest /api/v1/topology/location REST api
//      src/web/tntnet.xml, src/web/src/topology_location_from[2].ecpp, src/web/src/topology_location_to.ecpp
//  returns 0 if success (json payload is valid), else <0

 int
    topology_location (std::map<std::string, std::string> & param, std::string & json);

//  @end

#ifdef __cplusplus
}
#endif

#endif
