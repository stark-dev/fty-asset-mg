/*  =========================================================================
    fty_asset_configurator - Configator class

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

#ifndef FTY_ASSET_CONFIGURATOR_H_INCLUDED
#define FTY_ASSET_CONFIGURATOR_H_INCLUDED

#include <string>
#include <map>
#include <vector>
#include <malamute.h>
#include "../src/preproc.h"


struct AutoConfigurationInfo
{
    uint32_t type = 0;
    uint32_t subtype = 0;
    int8_t operation;
    bool configured = false;
    uint64_t date = 0;
    std::map <std::string, std::string> attributes;
};

class Configurator
{
 public:
    bool configure (const std::string& name, const AutoConfigurationInfo& info)
    {
        return configure (name, info, NULL);
    }

    bool configure (const std::string& name, const AutoConfigurationInfo& info, mlm_client_t *client)
    {
        return v_configure (name, info, client);
    }

    virtual bool isApplicable (UNUSED_PARAM const AutoConfigurationInfo& info)
    {
        return false;
    }

    virtual ~Configurator() {};

 protected:
    virtual bool v_configure (const std::string& name, const AutoConfigurationInfo& info, mlm_client_t *client) = 0;
};


void fty_asset_configurator_test (bool verbose);     



#endif // FTY_ASSET_CONFIGURATOR_H_INCLUDED
