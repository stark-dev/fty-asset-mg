#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include "db.h"
#include "src/asset/asset.h"
#include "testdb.h"
#include <fty_common_db_dbpath.h>
#include <tntdb.h>

TEST_CASE("Assets")
{
    SECTION("Create asset")
    {
        fty::AssetImpl asset;
        asset.setPriority(1);
        asset.setAssetType("device");
        asset.setAssetSubtype("router");
        asset.setAssetStatus(fty::AssetStatus::Active);

        REQUIRE_NOTHROW(asset.save());

        fty::AssetImpl asset2;
        asset2.setPriority(1);
        asset2.setAssetType("device");
        asset2.setAssetSubtype("router");
        asset2.setAssetStatus(fty::AssetStatus::Active);
        asset2.setExtEntry("name", "Router 2");

        REQUIRE_NOTHROW(asset2.save());

        asset.setPriority(2);
        CHECK_NOTHROW(asset.save());

        fty::AssetImpl load1(asset.getInternalName());
        CHECK(load1 == asset);
    }

    SECTION("Delete simple")
    {
        fty::AssetImpl asset;
        asset.setPriority(1);
        asset.setAssetType("device");
        asset.setAssetSubtype("router");
        asset.setAssetStatus(fty::AssetStatus::Active);

        REQUIRE_NOTHROW(asset.save());
        CHECK(asset.getId() > 0);

        std::string name = asset.getInternalName();
        asset.remove();

        REQUIRE_THROWS(fty::AssetImpl{name});
    }

    SECTION("Delete parent")
    {
        fty::AssetImpl parent;
        parent.setPriority(1);
        parent.setAssetType("device");
        parent.setAssetSubtype("switch");
        parent.setAssetStatus(fty::AssetStatus::Active);
        REQUIRE_NOTHROW(parent.save());

        fty::AssetImpl child;
        child.setPriority(1);
        child.setAssetType("device");
        child.setAssetSubtype("server");
        child.setAssetStatus(fty::AssetStatus::Active);
        child.setParentIname(parent.getInternalName());
        REQUIRE_NOTHROW(child.save());

        REQUIRE_NOTHROW(parent.reload());

        REQUIRE_THROWS_WITH(parent.remove(),
            Catch::Matchers::Contains("can't delete the asset because it has at least one child"));

        REQUIRE_NOTHROW(child.remove());
        REQUIRE_NOTHROW(parent.reload());
        REQUIRE_NOTHROW(parent.remove());

        REQUIRE_THROWS_WITH(fty::AssetImpl{parent.getInternalName()}, Catch::Matchers::Contains("not found"));
        REQUIRE_THROWS_WITH(fty::AssetImpl{child.getInternalName()}, Catch::Matchers::Contains("not found"));
    }

    SECTION("Delete parent recursive")
    {
        fty::AssetImpl parent;
        parent.setPriority(1);
        parent.setAssetType("device");
        parent.setAssetSubtype("switch");
        parent.setAssetStatus(fty::AssetStatus::Active);
        REQUIRE_NOTHROW(parent.save());

        fty::AssetImpl child;
        child.setPriority(1);
        child.setAssetType("device");
        child.setAssetSubtype("server");
        child.setAssetStatus(fty::AssetStatus::Active);
        child.setParentIname(parent.getInternalName());
        REQUIRE_NOTHROW(child.save());

        REQUIRE_NOTHROW(parent.reload());

        REQUIRE_NOTHROW(parent.remove(true));

        REQUIRE_THROWS_WITH(fty::AssetImpl{parent.getInternalName()}, Catch::Matchers::Contains("not found"));
        REQUIRE_THROWS_WITH(fty::AssetImpl{child.getInternalName()}, Catch::Matchers::Contains("not found"));
    }

    SECTION("Delete linked")
    {
        fty::AssetImpl asset;
        asset.setPriority(1);
        asset.setAssetType("device");
        asset.setAssetSubtype("switch");
        asset.setAssetStatus(fty::AssetStatus::Active);
        REQUIRE_NOTHROW(asset.save());

        fty::AssetImpl src;
        src.setPriority(1);
        src.setAssetType("device");
        src.setAssetSubtype("server");
        src.setAssetStatus(fty::AssetStatus::Active);
        src.setLinkedAssets({asset.getInternalName()});
        REQUIRE_NOTHROW(src.save());

        REQUIRE_THROWS_WITH(src.remove(), Catch::Matchers::Contains("can't delete the asset because it is linked to others"));
    }
}


int main(int argc, char* argv[])
{
    TestDB db("asset");
    createDb();
    return Catch::Session().run(argc, argv);
}
