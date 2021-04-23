cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

##############################################################################################################

etn_target(shared ${PROJECT_NAME}-accessor
    SOURCES
        src/fty_asset_accessor.cc
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        fty_asset_accessor.h
    USES_PUBLIC
        fty-asset
        fty-utils
    USES_PRIVATE
        cxxtools
        fty_common
        fty_common_messagebus
        czmq
        mlm
)


##############################################################################################################

# etn_test_target(${PROJECT_NAME}-accessor
#     SOURCES
#         test/*.cpp
#     USES
#         cxxtools
#         fty_common_logging
# )
