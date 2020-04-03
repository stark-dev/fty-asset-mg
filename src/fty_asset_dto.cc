/*  =========================================================================
    fty_asset_dto - Asset DTO: defines ASSET attributes for data exchange

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
    fty_asset_dto - Asset DTO: defines ASSET attributes for data exchange
@discuss
@end
*/

#include "../include/fty_asset_dto.h"

#include <zhash.h>

// for fty-proto conversion
static fty::Asset::HashMap zhashToMap(zhash_t *hash)
{
    fty::Asset::HashMap map;

    for (auto* item = zhash_first(hash); item; item = zhash_next(hash))
    {
        map.emplace(zhash_cursor(hash), static_cast<const char *>(item));
    }

    return map;
}


static zhash_t* mapToZhash(const fty::Asset::HashMap &map)
{
    zhash_t *hash = zhash_new ();
    for (const auto & i :map) {
        zhash_insert (hash, i.first.c_str (), (void*) i.second.c_str());
    }

    return hash;
}

namespace fty
{
    std::map<AssetStatus, std::string> AssetStatusToProto =
    {
        {AssetStatus::Unknown, "unknown"},
        {AssetStatus::Active, "active"},
        {AssetStatus::Nonactive, "nonactive"}
    };

    std::map<std::string, fty::AssetStatus> ProtoToAssetStatus = 
    {
        {"unknown", AssetStatus::Unknown},
        {"active", AssetStatus::Active},
        {"nonactive", AssetStatus::Nonactive}
    };

    const std::string Asset::internalName() const
    {
        return m_assetSubtype + "-" + std::to_string(m_id);
    }

    // getters
    const int Asset::getId() const
    {
        return m_id;
    }

    const AssetStatus Asset::getAssetStatus() const
    {
        return m_assetStatus;
    }

    const std::string & Asset::getAssetType() const
    {
        return m_assetType;
    }

    const std::string & Asset::getAssetSubtype() const
    {
        return m_assetSubtype;
    }

    const std::string & Asset::getFriendlyName() const
    {
        return m_friendlyName;
    }

    const std::string & Asset::getParentId() const
    {
        return m_parentId;
    }

    const int Asset::getPriority() const
    {
        return m_priority;
    }

    const Asset::HashMap & Asset::getExt() const
    {
        return m_ext;
    }
    
    // setters
    void Asset::setId(const int id)
    {
        m_id = id;
    }

    void Asset::setAssetStatus(const AssetStatus assetStatus)
    {
        m_assetStatus = assetStatus;
    }

    void Asset::setAssetType(const std::string & assetType)
    {
        m_assetType = assetType;
    }

    void Asset::setAssetSubtype(const std::string & assetSubtype)
    {
        m_assetSubtype = assetSubtype;
    }

    void Asset::setFriendlyName(const std::string & friendlyName)
    {
        m_friendlyName = friendlyName;
    }

    void Asset::setParentId(const std::string & parendId)
    {
        m_parentId = parendId;
    }

    void Asset::setPriority(const int priority)
    {
        m_priority = priority;
    }

    void Asset::setExt(const Asset::HashMap & ext)
    {
        m_ext = ext;
    }

    bool Asset::operator== (const Asset &asset) const
    {
        if(
            m_id           != asset.m_id           ||
            m_assetStatus  != asset.m_assetStatus  ||
            m_assetType    != asset.m_assetType    ||
            m_assetSubtype != asset.m_assetSubtype ||
            m_friendlyName != asset.m_friendlyName ||
            m_parentId     != asset.m_parentId     ||
            m_parentId     != asset.m_parentId     ||
            m_priority     != asset.m_priority
        )
        {
            std::cout << m_id           << " : " <<  asset.m_id << std::endl;
            std::cout << (int)m_assetStatus  << " : " << (int)asset.m_assetStatus << std::endl;
            std::cout << m_assetType    << " : " <<  asset.m_assetType << std::endl;
            std::cout << m_assetSubtype << " : " <<  asset.m_assetSubtype << std::endl;
            std::cout << m_friendlyName << " : " <<  asset.m_friendlyName << std::endl;
            std::cout << m_parentId     << " : " <<  asset.m_parentId << std::endl;
            std::cout << m_parentId     << " : " <<  asset.m_parentId << std::endl;
            std::cout << m_priority     << " : " <<  asset.m_priority << std::endl;
            return false;
        }

        for(const auto & p : m_ext)
        {
            auto search = asset.m_ext.find(p.first);
            if(search == asset.m_ext.end())
            {
                return false;
            }
            else if(search->second != p.second)
            {
                return false;
            }
        }

        return true;
    }

    bool Asset::operator!= (const Asset &asset) const
    {
        return !(*this == asset);
    }

    // serialization and deserialization operators
    void operator<<= (cxxtools::SerializationInfo & si, const Asset & asset)
    {
        // basic
        si.addMember("id") <<= asset.getId();
        si.addMember("status") <<= int(asset.getAssetStatus());
        si.addMember("type") <<= asset.getAssetType();
        si.addMember("sub_type") <<= asset.getAssetSubtype();
        si.addMember("name") <<= asset.getFriendlyName();
        si.addMember("priority") <<= asset.getPriority();
        si.addMember("location") <<= asset.getParentId();

        cxxtools::SerializationInfo &extSi = si.addMember("ext");
        extSi.setCategory (cxxtools::SerializationInfo::Object);
        for (const auto keyValue : asset.getExt())
        {
            auto key = keyValue.first;
            auto value = keyValue.second;
            cxxtools::SerializationInfo &keyValueObject = extSi.addMember (key);
            keyValueObject.setCategory (cxxtools::SerializationInfo::Object);
            keyValueObject.setValue (value);
        }
    }

    void operator>>= (const cxxtools::SerializationInfo & si, Asset & asset)
    {
        int tmpInt;
        std::string tmpString;
        Asset::HashMap tmpMap;

        // status
        si.getMember("status") >>= tmpInt;
        asset.setAssetStatus(AssetStatus(tmpInt));

        // type
        si.getMember("type") >>= tmpString;
        asset.setAssetType(tmpString);

        // subtype
        si.getMember("sub_type") >>= tmpString;
        asset.setAssetSubtype(tmpString);

        // external name
        si.getMember("name") >>= tmpString;
        asset.setFriendlyName(tmpString);

        // priority
        si.getMember("priority") >>= tmpInt;
        asset.setPriority(tmpInt);

        // parend id
        si.getMember("location") >>= tmpString;
        asset.setParentId(tmpString);

        // id (if missing assign -1)
        if (!si.findMember("id"))
        {
            tmpInt = -1;
        }
        else
        {
            si.getMember("id") >>= tmpInt;
        }
        asset.setId(tmpInt);

        // ext map
        tmpMap.clear();
        if (si.findMember("ext"))
        {
            const cxxtools::SerializationInfo extSi = si.getMember("ext");
            for (const auto & element : extSi)
            {
                auto key = element.name();
                // ext from UI behaves as an object of objects with empty 1st level keys
                if (key.empty())
                {
                    for (const auto innerElement : element)
                    {
                        auto innerKey = innerElement.name();
                        log_debug ("inner key = %s", innerKey.c_str ());
                        // only DB is interested in read_only attribute
                        if (innerKey != "read_only")
                        {
                            std::string value;
                            innerElement >>= value;
                            tmpMap[innerKey] = value;
                        }
                    }
                }
                else
                {
                    std::string value;
                    element >>= value;
                    log_debug ("key = %s, value = %s", key.c_str (), value.c_str ());
                    tmpMap[key] = value;
                }
            }
        }
        asset.setExt(tmpMap);
    }

    fty_proto_t * assetToFtyProto(const Asset & asset, const std::string & operation)
    {
        fty_proto_t *proto = fty_proto_new(FTY_PROTO_ASSET);

        zhash_t *aux = zhash_new();
        zhash_autofree (aux);

        zhash_insert(aux, "priority", (void*) std::to_string(asset.getPriority()).c_str());
        zhash_insert(aux, "type", (void*) asset.getAssetType().c_str());
        zhash_insert(aux, "subtype", (void*) asset.getAssetSubtype().c_str());
        zhash_insert(aux, "parent", (void*) asset.getParentId().c_str());
        zhash_insert(aux, "status", (void*) AssetStatusToProto.at(asset.getAssetStatus()).c_str());

        zhash_t *ext = mapToZhash(asset.getExt());

        fty_proto_set_aux(proto, &aux);
        fty_proto_set_name(proto, "%s", asset.internalName().c_str());
        fty_proto_set_operation(proto, "%s", operation.c_str());
        fty_proto_set_ext(proto, &ext);

        zhash_destroy(&aux);
        zhash_destroy(&ext);

        return proto;
    }

    Asset ftyProtoToAsset(fty_proto_t * proto)
    {
        if (fty_proto_id(proto) != FTY_PROTO_ASSET)
        {
            throw std::invalid_argument("Wrong fty-proto type");
        }

        Asset asset;
        asset.setAssetStatus(ProtoToAssetStatus.at(fty_proto_aux_string(proto, "status", "active")));
        asset.setAssetType(fty_proto_aux_string(proto, "type", ""));
        asset.setAssetSubtype(fty_proto_aux_string(proto, "subtype", ""));
        // asset.setFriendlyName(fty_proto_name(proto));
        asset.setParentId(fty_proto_aux_string(proto, "parent", ""));
        asset.setPriority(fty_proto_aux_number(proto, "priority", 5));

        zhash_t *ext = fty_proto_ext(proto);
        asset.setExt(zhashToMap(ext));

        return asset;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

// color output definition for test function
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>

void fty_asset_dto_test(bool verbose)
{
    printf (" * fty_asset_dto: ");

    std::vector<std::pair<std::string, bool>> testsResults;

    std::string testNumber;
    std::string testName;

    //Next test
    testNumber = "1.1";
    testName = "Create fty::Asset object";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset asset;

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }

    //Next test
    testNumber = "2.1";
    testName = "Equality check - good case";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset::HashMap ext;
            ext.emplace(std::make_pair("testKey", "testValue"));

            Asset asset;
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            // asset.setFriendlyName("test-device");
            asset.setParentId("abc123");
            asset.setExt(ext);
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;

            if (!(asset == asset2))
            {
                throw std::runtime_error("Equality check failed");
            }

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }

    //Next test
    testNumber = "2.2";
    testName = "Equality check - bad case";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset::HashMap ext;
            ext.emplace(std::make_pair("testKey", "testValue"));

            Asset asset;
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            // asset.setFriendlyName("test-device");
            asset.setParentId("abc123");
            asset.setExt(ext);
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;

            asset2.setParentId("wrong-name");

            if (asset == asset2)
            {
                throw std::runtime_error("Equality check failed");
            }

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }

    //Next test
    testNumber = "2.3";
    testName = "Inequality check - good case";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset::HashMap ext;
            ext.emplace(std::make_pair("testKey", "testValue"));

            Asset asset;
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            // asset.setFriendlyName("test-device");
            asset.setParentId("abc123");
            asset.setExt(ext);
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;
            asset2.setAssetType(SUB_RACK_CONTROLLER);

            if (!(asset != asset2))
            {
                throw std::runtime_error("Inequality check failed");
            }

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }

    //Next test
    testNumber = "2.4";
    testName = "Inequality check - bad case";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset::HashMap ext;
            ext.emplace(std::make_pair("testKey", "testValue"));

            Asset asset;
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            // asset.setFriendlyName("test-device");
            asset.setParentId("abc123");
            asset.setExt(ext);
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;

            if (asset != asset2)
            {
                throw std::runtime_error("Inequality check failed");
            }

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }

    // JSON serialization/deserialization
    testNumber = "3.1";
    testName = "JSON serialization and deserialization";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset::HashMap ext;
            ext.emplace(std::make_pair("testKey", "testValue"));

            Asset asset;
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            // asset.setFriendlyName("test-device");
            asset.setParentId("abc123");
            asset.setExt(ext);
            asset.setPriority(4);

            cxxtools::SerializationInfo si;
            si <<= asset;

            Asset asset2;

            si >>= asset2;

            if (asset != asset2)
            {
                throw std::runtime_error("Assets do not match");
            }

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }
    
    // fty-proto encoding/decoding
    testNumber = "4.1";
    testName = "fty-proto encoding and decoding";
    printf ("\n----------------------------------------------------------------"
            "-------\n");
    {
        printf (" *=>  Test #%s %s\n", testNumber.c_str (), testName.c_str ());

        try {
            using namespace fty;

            Asset::HashMap ext;
            ext.emplace(std::make_pair("testKey", "testValue"));

            Asset asset;
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            // asset.setFriendlyName("test-device");
            asset.setParentId("abc123");
            asset.setExt(ext);
            asset.setPriority(4);

            fty_proto_t * p = assetToFtyProto(asset, "UPDATE");

            Asset asset2;

            asset2 = ftyProtoToAsset(p);
            fty_proto_destroy(&p);

            if (asset != asset2)
            {
                throw std::runtime_error("Assets do not match");
            }

            printf (" *<=  Test #%s > OK\n", testNumber.c_str ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, true);
        }
        catch (const std::exception &e) {
            printf (" *<=  Test #%s > Failed\n", testNumber.c_str ());
            printf ("Error: %s\n", e.what ());
            testsResults.emplace_back (" Test #" + testNumber + " " + testName, false);
        }
    }

    // collect results

    printf("\n-----------------------------------------------------------------------\n");

	uint32_t testsPassed = 0;
	uint32_t testsFailed = 0;


	printf("\tSummary tests from fty_asset_dto\n");
	for(const auto & result : testsResults)
	{
		if(result.second)
		{
			printf(ANSI_COLOR_GREEN"\tOK " ANSI_COLOR_RESET "\t%s\n",result.first.c_str());
			testsPassed++;
		}
		else
		{
			printf(ANSI_COLOR_RED"\tNOK" ANSI_COLOR_RESET "\t%s\n",result.first.c_str());
			testsFailed++;
		}
	}

    printf("\n-----------------------------------------------------------------------\n");

	if(testsFailed == 0)
	{
		printf(ANSI_COLOR_GREEN"\n %i tests passed, everything is ok\n" ANSI_COLOR_RESET "\n",testsPassed);
	}
	else
	{
		printf(ANSI_COLOR_RED"\n!!!!!!!! %i/%i tests did not pass !!!!!!!! \n" ANSI_COLOR_RESET "\n",testsFailed,(testsPassed+testsFailed));

		assert(false);
	}
}
