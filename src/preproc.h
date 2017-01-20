/*  =========================================================================
    preproc - Preprocessor setting for unused parameters

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

#ifndef PREPROC_H_INCLUDED
#define PREPROC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


// marker to tell humans and GCC that the unused parameter is there for some
// reason (i.e. API compatibility) and compiler should not warn if not used
#ifndef UNUSED_PARAM
# ifdef __GNUC__
#  define UNUSED_PARAM __attribute__ ((__unused__))
# else
#  define UNUSED_PARAM
# endif
#endif


//  Self test of this class
void
    preproc_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
