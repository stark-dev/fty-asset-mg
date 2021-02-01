#pragma once
#include <fty/expected.h>
#include <string>

namespace fty {

class TestDb
{
public:
    Expected<std::string> create();
    Expected<void>        destroy();

private:
    std::string m_path;
};

} // namespace fty
