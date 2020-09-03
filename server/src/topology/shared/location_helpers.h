/*  =========================================================================
    topology_shared_location_helpers - class description

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

#ifndef TOPOLOGY_SHARED_LOCATION_HELPERS_H_INCLUDED
#define TOPOLOGY_SHARED_LOCATION_HELPERS_H_INCLUDED

#include <string>
#include "asset_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

/*!
 * \file location_helpers.h
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \brief Not yet documented file
 */

 int
    asset_location_r(asset_msg_t** asset_msg, std::string& json);

//  @end

#ifdef __cplusplus
}
#endif

#endif
