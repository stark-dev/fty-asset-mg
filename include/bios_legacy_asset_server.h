/*  =========================================================================
    bios_legacy_asset_server - Server translating legacy configure messages to new protocl

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

#ifndef BIOS_LEGACY_ASSET_SERVER_H_INCLUDED
#define BIOS_LEGACY_ASSET_SERVER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _bios_legacy_asset_server_t bios_legacy_asset_server_t;

//  @interface
//  Server converting old configure messages sent by REST API to new protocol
AGENT_ASSET_EXPORT void
    bios_legacy_asset_server (zsock_t *pipe, void *args);

//  Self test of this class
AGENT_ASSET_EXPORT void
    bios_legacy_asset_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
