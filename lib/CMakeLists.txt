cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-asset
    VERSION 1.0.0
)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################


##############################################################################################################

etn_target(shared ${PROJECT_NAME}
    SOURCES
        src/*.cc
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        fty_asset_dto.h
        fty_common_asset.h
    USES_PRIVATE
        cxxtools  # cxxtools cannot be use as public because we do not have the cmake package yet
        fty_common_logging
        fty_common_mlm
        fty_proto
        fty-utils
)


##############################################################################################################

if(BUILD_TESTING)
    enable_testing()

    # Create a target for the tests
    etn_test(${PROJECT_NAME}-test
        SOURCES
            test/*.cpp
        USES
            ${PROJECT_NAME}
            Catch2::Catch2
            cxxtools 
    )
    
endif()
