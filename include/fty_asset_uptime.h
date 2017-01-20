/*  =========================================================================
    fty_asset_uptime - Sending asset information necessary for fty-kpi-power-uptime

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

#ifndef FTY_ASSET_UPTIME_H_INCLUDED
#define FTY_ASSET_UPTIME_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new fty_asset_uptime
FTY_ASSET_EXPORT fty_asset_uptime_t *
    fty_asset_uptime_new (void);

//  Destroy the fty_asset_uptime
FTY_ASSET_EXPORT void
    fty_asset_uptime_destroy (fty_asset_uptime_t **self_p);

//  Self test of this class
FTY_ASSET_EXPORT void
    fty_asset_uptime_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
