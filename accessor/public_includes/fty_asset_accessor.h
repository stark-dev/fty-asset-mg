#pragma once

#include <fty_asset_dto.h>
#include <fty_common_client.h>
#include <fty_common_mlm_sync_client.h>
#include <memory>
#include <string>

namespace fty
{

    class AssetAccessor
    {
    public:
        AssetAccessor();
        ~AssetAccessor();

        uint32_t assetInameToID(const std::string &iname);
    };

} // namespace fty
