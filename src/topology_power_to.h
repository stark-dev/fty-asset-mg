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

#ifndef TOPOLOGY_POWER_TO_H_INCLUDED
#define TOPOLOGY_POWER_TO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new topology_power_to
FTY_ASSET_PRIVATE topology_power_to_t *
    topology_power_to_new (void);

//  Destroy the topology_power_to
FTY_ASSET_PRIVATE void
    topology_power_to_destroy (topology_power_to_t **self_p);

//  Self test of this class
FTY_ASSET_PRIVATE void
    topology_power_to_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
