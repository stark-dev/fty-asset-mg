/*  =========================================================================
    fty_asset_manipulation - Helper functions to perform asset manipulation

    Copyright (C) 2016 - 2020 Eaton

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
    fty_asset_manipulation - Helper functions to perform asset manipulation
@discuss
@end
*/


#include "fty_asset_manipulation.h"
#include "dbhelpers.h"
#include <ftyproto.h>
#include <openssl/ssl.h>
#include <stdlib.h>
#include <string>
#include <tntdb/connect.h>
#include <tntdb/row.h>
#include <uuid/uuid.h>
#include <zhash.h>

fty::Asset getAsset(const std::string& assetInternalName, bool test)
{
    fty::Asset a;

    if (test) {
        log_debug("[asset_manipulation]: getAsset test mode");
        a.setInternalName(assetInternalName);
    } else {
        a = getAssetFromDB(assetInternalName);
    }

    return a;
}

std::vector<fty::Asset> listAssets(bool test)
{
    std::vector<fty::Asset> v;

    if (test) {
        log_debug("[asset_manipulation]: listAssets test mode");

        fty::Asset a;

        // TODO improve test
        v.push_back(a);
        v.push_back(a);
        v.push_back(a);
        v.push_back(a);
    } else {
        v = getAllAssetsFromDB();
    }
    return v;
}
