/*  =========================================================================
    asset_conversion_proto - asset/conversion/proto

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

#include "proto.h"
#include "include/fty_asset_dto.h"
#include "src/dbhelpers.h"
#include <fty_proto.h>

namespace fty { namespace conversion {
    // for fty-proto conversion helpers
    static fty::Asset::ExtMap zhashToExtMap(zhash_t* hash, bool readOnly)
    {
        fty::Asset::ExtMap map;

        for (auto* item = zhash_first(hash); item; item = zhash_next(hash)) {
            map.emplace(zhash_cursor(hash), std::make_pair(static_cast<const char*>(item), readOnly));
        }

        return map;
    }


    static zhash_t* extMapToZhash(const fty::Asset::ExtMap& map)
    {
        zhash_t* hash = zhash_new();
        for (const auto& i : map) {
            zhash_insert(hash, i.first.c_str(),
                const_cast<void*>(reinterpret_cast<const void*>(i.second.first.c_str())));
        }

        return hash;
    }

    // fty-proto/Asset conversion
    fty_proto_t* toFtyProto(const fty::Asset& asset, const std::string& operation, bool test)
    {
        fty_proto_t* proto = fty_proto_new(FTY_PROTO_ASSET);

        // no need to free, as fty_proto_set_aux transfers ownership to caller
        zhash_t* aux = zhash_new();
        zhash_autofree(aux);

        std::string priority = std::to_string(asset.getPriority());
        zhash_insert(aux, "priority", const_cast<void*>(reinterpret_cast<const void*>(priority.c_str())));
        zhash_insert(
            aux, "type", const_cast<void*>(reinterpret_cast<const void*>(asset.getAssetType().c_str())));
        zhash_insert(aux, "subtype",
            const_cast<void*>(reinterpret_cast<const void*>(asset.getAssetSubtype().c_str())));
        if (test) {
            zhash_insert(aux, "parent", const_cast<void*>(reinterpret_cast<const void*>("0")));
        } else {
            std::string parentIname = asset.getInternalName();
            if (!parentIname.empty()) {
                std::string parentId =
                    std::to_string(selectAssetProperty<int>("id_parent", "name", asset.getInternalName()));
                zhash_insert(
                    aux, "parent", const_cast<void*>(reinterpret_cast<const void*>(parentId.c_str())));
            } else {
                zhash_insert(aux, "parent", const_cast<void*>(reinterpret_cast<const void*>("0")));
            }
        }
        zhash_insert(aux, "status",
            const_cast<void*>(
                reinterpret_cast<const void*>(assetStatusToString(asset.getAssetStatus()).c_str())));

        // no need to free, as fty_proto_set_ext transfers ownership to caller
        zhash_t* ext = extMapToZhash(asset.getExt());

        fty_proto_set_aux(proto, &aux);
        fty_proto_set_name(proto, "%s", asset.getInternalName().c_str());
        fty_proto_set_operation(proto, "%s", operation.c_str());
        fty_proto_set_ext(proto, &ext);

        return proto;
    }

    fty::Asset fromFtyProto(fty_proto_t* proto, bool extAttributeReadOnly, bool test)
    {
        if (fty_proto_id(proto) != FTY_PROTO_ASSET) {
            throw std::invalid_argument("Wrong message type");
        }

        fty::Asset asset;
        asset.setInternalName(fty_proto_name(proto));

        std::string assetStatus(fty_proto_aux_string(proto, "status", "active"));
        asset.setAssetStatus(fty::stringToAssetStatus(assetStatus));
        asset.setAssetType(fty_proto_aux_string(proto, "type", ""));
        asset.setAssetSubtype(fty_proto_aux_string(proto, "subtype", ""));
        if (test) {
            asset.setParentIname("test-parent");
        } else {
            int parentId = fty_proto_aux_number(proto, "parent", 0);
            if (parentId != 0) {
                asset.setParentIname(selectAssetProperty<std::string>("name", "id_asset_element", parentId));
            } else {
                asset.setParentIname("");
            }
        }
        asset.setPriority(fty_proto_aux_number(proto, "priority", 5));

        zhash_t* ext = fty_proto_ext(proto);
        asset.setExt(zhashToExtMap(ext, extAttributeReadOnly));

        return asset;
    }

}} // namespace fty::conversion
