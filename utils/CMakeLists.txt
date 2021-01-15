cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-asset-utils
    VERSION 1.0.0
)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################


##############################################################################################################

etn_target(shared ${PROJECT_NAME}
    SOURCES
        src/asset/conversion/utils/msgbus-utils.cc
        src/asset/conversion/full-asset.cc
        src/asset/conversion/json.cc
        src/asset/conversion/proto.cc
        src/asset/serialization/serialization.cc
    INCLUDE_DIRS
        include
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        asset/conversion/full-asset.h
        asset/conversion/json.h
        asset/conversion/proto.h
        asset/serialization/serialization.h
    USES_PRIVATE
        cxxtools  # cxxtools cannot be use as public because we do not have the cmake package yet
        fty_common_messagebus
        fty_proto
        fty-utils
)


##############################################################################################################
