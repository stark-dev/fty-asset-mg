/*  =========================================================================
    asset_conversion_json - asset/conversion/json

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

#include "asset/conversion/json.h"

#include <fty_asset_dto.h>
#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>
#include <sstream>

namespace fty { namespace conversion {

    std::string toJson(const Asset& asset)
    {
        std::ostringstream output;

        cxxtools::SerializationInfo si;
        cxxtools::JsonSerializer    serializer(output);

        si <<= asset;
        serializer.serialize(si);

        std::string json = output.str();

        return json;
    }

    void fromJson(const std::string& json, fty::Asset& asset)
    {
        std::istringstream input(json);

        cxxtools::SerializationInfo si;
        cxxtools::JsonDeserializer  deserializer(input);

        deserializer.deserialize(si);

        si >>= asset;
    }

}} // namespace fty::conversion
