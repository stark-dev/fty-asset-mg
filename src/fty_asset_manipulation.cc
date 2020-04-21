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

#define AGENT_ASSET_ACTIVATOR   "etn-licensing-credits"

#include "dbhelpers.h"
#include "fty_asset_manipulation.h"

#include <string>

#include <tntdb/row.h>
#include <tntdb/connect.h>
#include <fty_common_asset.h>

/// create asset name from type/subtype and integer ID
static std::string createAssetName(const std::string& type, const std::string& subtype, long index)
{
    std::string assetName;

    if(type == fty::TYPE_DEVICE) {
        assetName = subtype + "-" + std::to_string(index);
    } else {
        assetName = type + "-" + std::to_string(index);
    }

    return assetName;
}

// TODO remove as soon as fty::Asset activation is supported
/// convert fty::Asset to fty::FullAsset
static fty::FullAsset assetToFullAsset(const fty::Asset& asset)
{
    fty::FullAsset::HashMap auxMap; // does not exist in new Asset implementation
    fty::FullAsset::HashMap extMap;
   
    for (const auto& element : asset.getExt())
    {
        // FullAsset hash map has no readOnly parameter
        extMap[element.first] = element.second.first;
    }

    fty::FullAsset fa(
        asset.getInternalName(),
        fty::assetStatusToString(asset.getAssetStatus()),
        asset.getAssetType(),
        asset.getAssetSubtype(),
        asset.getExtEntry("name"), // asset name is stored in ext structure
        asset.getParentIname(),
        asset.getPriority(),
        auxMap,
        extMap
    );

    return fa;
}

/// test if asset activation is possible
static bool testAssetActivation(const fty::Asset& asset)
{
    mlm::MlmSyncClient client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
    fty::AssetActivator activationAccessor(client);
    
    // TODO remove as soon as fty::Asset activation is supported
    fty::FullAsset fa = assetToFullAsset(asset);

    return (!activationAccessor.isActivable(fa));
}

/// perform asset activation
static void activateAsset(const fty::Asset& asset)
{
    mlm::MlmSyncClient client(AGENT_FTY_ASSET, AGENT_ASSET_ACTIVATOR);
    fty::AssetActivator activationAccessor(client);
    
    // TODO remove as soon as fty::Asset activation is supported
    fty::FullAsset fa = assetToFullAsset(asset);

    activationAccessor.activate(fa);
}

fty::Asset createAsset(const fty::Asset& asset, bool tryActivate, bool test)
{
    fty::Asset createdAsset;
    createdAsset = asset;

    if(test)
    {
        log_debug ("[createAsset]: runs in test mode");
        test_map_asset_state[asset.getInternalName()] = fty::assetStatusToString(asset.getAssetStatus());
    }
    else
    {
        // check if asset can be activated (if needed)
        if((createdAsset.getAssetStatus() == fty::AssetStatus::Active) && !testAssetActivation(createdAsset))
        {
            if(tryActivate) // if activation fails and tryActivate is true -> create anyway wih status "non active"
            {
                createdAsset.setAssetStatus(fty::AssetStatus::Nonactive);
            }
            else            // do not create asset in db
            {
                throw std::runtime_error("Licensing limitation hit - maximum amount of active power devices allowed in license reached.");
            }
        }

        // datacenter parent ID must be null
        if(createdAsset.getAssetType() == fty::TYPE_DATACENTER && !createdAsset.getParentIname().empty())
        {
            throw std::runtime_error("Invalid parent ID for asset type DataCenter");
        }

        // first insertion with random unique name, to get the int id later
        // (@ is prohibited in name => name-@@-123 is unique)
        createdAsset.setInternalName(asset.getInternalName() + std::string("-@@-") + std::to_string(rand()));

        // TODO get id of last inserted element (race condition may occur, use a select statement instead)
        long assetIndex = insertAssetToDB(createdAsset);

        // create and update asset internal name (<type/subtype>-<id>)
        std::string assetName = createAssetName(asset.getAssetType(), asset.getAssetSubtype(), assetIndex);
        createdAsset.setInternalName(assetName);

        updateAssetProperty("name", assetName, "id_asset_element", assetIndex);

        // update external properties
        log_debug ("Processing inventory for asset %s", createdAsset.getInternalName().c_str());
        updateAssetExtProperties(createdAsset);

        // activate asset
        if(createdAsset.getAssetStatus() == fty::AssetStatus::Active)
        {
            activateAsset(asset);
        }
    }

    return createdAsset;
}

fty::Asset updateAsset(const fty::Asset& asset, bool test)
{
    fty::Asset updatedAsset;
    updatedAsset = asset;

    if(test)
    {
        log_debug ("[updateAsset]: runs in test mode");
        test_map_asset_state[asset.getInternalName()] = fty::assetStatusToString(asset.getAssetStatus());
    }
    else
    {
        try
        {
            // datacenter parent ID must be null
            if(updatedAsset.getAssetType() == fty::TYPE_DATACENTER && !updatedAsset.getParentIname().empty())
            {
                throw std::runtime_error("Invalid parent ID for asset type DataCenter");
            }

            // get current asset status (as stored in db)
            std::string currentStatus;
            currentStatus = selectAssetProperty<std::string>("status", "name", updatedAsset.getInternalName());

            // activate if needed
            // (old status is non active and requested status is active)
            bool needActivation = ((updatedAsset.getAssetStatus() == fty::AssetStatus::Active) && (currentStatus == fty::assetStatusToString(fty::AssetStatus::Nonactive)));
            if(needActivation)
            {
                // check if asset can be activated
                if(!testAssetActivation(updatedAsset))
                {
                    // TODO should update proceed on activation failure? (setting status to non active)
                    throw std::runtime_error("Licensing limitation hit - maximum amount of active power devices allowed in license reached.");
                }
            }

            // TODO use update query instead of insert
            // perform update
            updateAssetToDB(updatedAsset);

            if(needActivation)
            {
                activateAsset(updatedAsset);
            }

            log_debug ("Processing inventory for asset %s", updatedAsset.getInternalName().c_str());
            updateAssetExtProperties(updatedAsset);
        }

        catch(const std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }

    return updatedAsset;
}
