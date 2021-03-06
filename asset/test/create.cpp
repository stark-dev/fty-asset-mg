#include "asset/asset-manager.h"
#include <catch2/catch.hpp>
#include <fty_common_db_connection.h>
#include <test-db/sample-db.h>

TEST_CASE("Create asset")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
        )");

    static std::string json = R"({
        "location" :            "Data center",
        "name" :                "dev1",
        "powers":               [],
        "priority" :            "P2",
        "status" :              "active",
        "sub_type" :            "N_A",
        "type" :                "room",
        "ext": [
            {"asset_tag": "", "read_only": false},
            {"contact_name": "", "read_only": false},
            {"contact_email": "", "read_only": false},
            {"contact_phone": "", "read_only": false},
            {"description": "", "read_only": false},
            {"create_mode": "", "read_only": false},
            {"update_ts": "", "read_only": false}
        ]
    })";

    auto ret = fty::asset::AssetManager::createAsset(json, "dummy", false);
    REQUIRE_EXP(ret);
    CHECK(*ret > 0);

    CHECK(fty::asset::AssetManager::deleteAsset(*ret, false));
}

TEST_CASE("Wrong power")
{
    fty::SampleDb db(R"(
        items:
            - type     : Feed
              name     : feed
        )");

    fty::db::Connection       conn;
    fty::asset::db::AssetLink link;
    link.dest = db.idByName("feed");
    link.src  = db.idByName("feed");
    link.type = 1;

    auto ret = fty::asset::db::insertIntoAssetLink(conn, link);
    REQUIRE(!ret);
    CHECK(ret.error() == "connection loop was detected");
}

TEST_CASE("Feed in same DC")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter1
              ext-name : Data Center 1
              items:
                  - type     : Feed
                    name     : feed1
                    ext-name : Feed 1
            - type     : Datacenter
              name     : datacenter2
              ext-name : Data Center 2
              items:
                  - type     : Feed
                    name     : feed2
                    ext-name : Feed 2
    )");

    static std::string okJson = R"({
        "location" :            "Data center 1",
        "name" :                "dev1",
        "powers":               [{"src_id": "feed1", "src_name": "Feed 1", "src_socket": null}],
        "priority" :            "P2",
        "status" :              "active",
        "sub_type" :            "N_A",
        "type" :                "room"
    })";

    static std::string wrongJson = R"({
        "location" :            "Data center 1",
        "name" :                "dev2",
        "powers":               [{"src_id": "feed2", "src_name": "Feed 2", "src_socket": null}],
        "priority" :            "P2",
        "status" :              "active",
        "sub_type" :            "N_A",
        "type" :                "room"
    })";

    {
        auto ret = fty::asset::AssetManager::createAsset(okJson, "dummy", false);
        REQUIRE_EXP(ret);
        CHECK(*ret > 0);

        CHECK(fty::asset::AssetManager::deleteAsset(*ret, false));
    }

    {
        auto ret = fty::asset::AssetManager::createAsset(wrongJson, "dummy", false);
        REQUIRE(!ret);
        CHECK(ret.error().message() == "Request CREATE asset dev2 FAILED: Power source is not in same DC");
    }
}
