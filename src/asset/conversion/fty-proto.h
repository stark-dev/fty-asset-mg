/*  =========================================================================
    asset_conversion_fty_proto - asset/conversion/fty-proto

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

#pragma once

#include <fty_asset_dto.h>
#include <fty_proto.h>

namespace fty { namespace conversion {
    fty_proto_t* assetToFtyProto(const fty::Asset& asset, const std::string& operation, bool test);
    fty::Asset ftyProtoToAsset(fty_proto_t* proto, bool extAttributeReadOnly, bool test);
}} // namespace fty::conversion
