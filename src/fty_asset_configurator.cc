/*  =========================================================================
    fty_asset_configurator - Configure class

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

/*
@header
    fty_asset_configurator - Configure class
@discuss
@end
*/

#include "fty_asset_classes.h"

//  Structure of our class

struct _fty_asset_configurator_t {
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new fty_asset_configurator

fty_asset_configurator_t *
fty_asset_configurator_new (void)
{
    fty_asset_configurator_t *self = (fty_asset_configurator_t *) zmalloc (sizeof (fty_asset_configurator_t));
    assert (self);
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the fty_asset_configurator

void
fty_asset_configurator_destroy (fty_asset_configurator_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        fty_asset_configurator_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_asset_configurator_test (bool verbose)
{
    printf (" * fty_asset_configurator: ");

    //  @selftest
    //  Simple create/destroy test
    fty_asset_configurator_t *self = fty_asset_configurator_new ();
    assert (self);
    fty_asset_configurator_destroy (&self);
    //  @end
    printf ("OK\n");
}
