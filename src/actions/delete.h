#pragma once
#include <string>
#include <vector>

namespace fty {
class Asset;

std::string deleteAsset(const fty::Asset& asset, bool recursive);

}
