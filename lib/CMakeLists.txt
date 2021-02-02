cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

##############################################################################################################

etn_target(shared ${PROJECT_NAME}
    SOURCES
        src/fty_asset_dto.cc
        src/fty_common_asset.cc
        src/conversion/full-asset.cc
        src/conversion/json.cc
        src/conversion/proto.cc
        src/conversion/utils/msgbus-utils.cc
    INCLUDE_DIRS
        include
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        fty_asset_dto.h
        fty_common_asset.h
#    USES_PUBLIC
#        fty_proto
    USES_PRIVATE
        fty_proto
        cxxtools  # cxxtools cannot be use as public because we do not have the cmake package yet
        fty_common
        fty_common_logging
        fty_common_messagebus
        fty_common_mlm
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
