/*  =========================================================================

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

#include "asset/serialization/serialization.h"

#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>
#include <sstream>

namespace fty { namespace assetutils {
    // JSON serialize/deserialize
    std::string serialize(const cxxtools::SerializationInfo& si)
    {
        std::string returnData("");

        try {
            std::stringstream        output;
            cxxtools::JsonSerializer serializer(output);
            serializer.serialize(si);

            returnData = output.str();
        } catch (const std::exception& e) {
            throw std::runtime_error("Error while creating json " + std::string(e.what()));
        }

        return returnData;
    }

    cxxtools::SerializationInfo deserialize(const std::string& json)
    {
        cxxtools::SerializationInfo si;

        try {
            std::stringstream input;
            input << json;
            cxxtools::JsonDeserializer deserializer(input);
            deserializer.deserialize(si);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error in the json from server: " + std::string(e.what()));
        }

        return si;
    }
}} // namespace fty::assetutils
