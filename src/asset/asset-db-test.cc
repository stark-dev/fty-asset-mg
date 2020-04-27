/*  =========================================================================
    asset_asset_db_test - asset/asset-db-test

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
    asset_asset_db_test - asset/asset-db-test
@discuss
@end
*/

#include "asset-db-test.h"

namespace fty {

AssetImpl::DBTest::DBTest()
{
    std::cerr << "Test DB impl" << std::endl;
}

void AssetImpl::DBTest::init()
{
    std::cerr << "init" << std::endl;
}

void AssetImpl::DBTest::loadAsset(const std::string& nameId, Asset& asset)
{
    // clang-format off
    asset.setInternalName(nameId);
    asset.setAssetStatus(fty::AssetStatus::Nonactive);
    asset.setAssetType(fty::TYPE_DEVICE);
    asset.setAssetSubtype(fty::SUB_UPS);
    asset.setParentIname("abc123");
    asset.setPriority(4);
}

void AssetImpl::DBTest::loadExtMap(Asset& asset)
{
    asset.setExtEntry("uuid", "123-456-789", true);
    asset.setExtEntry("name", "My Asset", false);
}

void AssetImpl::DBTest::loadChildren(Asset& asset)
{
    std::vector<std::string> children;

    children.push_back("child-1");
    children.push_back("child-2");

    asset.setChildren(children);
}

void AssetImpl::DBTest::loadLinkedAssets(Asset& asset)
{
    std::vector<std::string> links;

    links.push_back("asset-1");
    links.push_back("asset-2");

    asset.setLinkedAssets(links);
}


bool AssetImpl::DBTest::isLastDataCenter(Asset& asset)
{
    return asset.getInternalName() == "dc-0";
}

void AssetImpl::DBTest::removeFromGroups(Asset& asset)
{
}

void AssetImpl::DBTest::removeFromRelations(Asset& asset)
{
}

void AssetImpl::DBTest::removeAsset(Asset& asset)
{
}

void AssetImpl::DBTest::removeExtMap(Asset& asset)
{
}

void AssetImpl::DBTest::clearGroup(Asset& asset)
{
}

void AssetImpl::DBTest::unlinkFrom(Asset& asset)
{
}

void AssetImpl::DBTest::beginTransaction()
{
}

void AssetImpl::DBTest::rollbackTransaction()
{
}

void AssetImpl::DBTest::commitTransaction()
{
}

void AssetImpl::DBTest::update(Asset& asset)
{
}

void AssetImpl::DBTest::insert(Asset& asset)
{
}

std::string AssetImpl::DBTest::unameById(uint32_t id)
{
    return "DC-1";
}

void AssetImpl::DBTest::saveLinkedAssets(Asset& asset)
{
}

void AssetImpl::DBTest::saveExtMap(Asset& asset)
{
}

std::vector<std::string> AssetImpl::DBTest::listAllAssets()
{
    std::vector<std::string> assetList;

    assetList.push_back("asset-1");
    assetList.push_back("asset-2");
    assetList.push_back("asset-3");

    return assetList;
}

} // namespace fty
