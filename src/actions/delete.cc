#include "delete.h"
#include <cxxtools/split.h>
#include <fty_common_db_dbpath.h>
#include <fty_common_messagebus.h>
#include <tntdb.h>
#include "include/fty_asset_dto.h"

namespace fty {

static constexpr char* RC0_INAME = "rackcontroller-0";

std::string deleteAsset(const Asset& asset, bool recursive)
{
    tntdb::Connection conn = tntdb::connect(DBConn::url);

    if (RC0_INAME == asset.getInternalName()) {
        log_debug("Prevented deleting RC-0");
        throw std::runtime_error("Prevented deleting RC-0");
    }

    return {};
}

}
