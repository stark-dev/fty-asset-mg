cmake_minimum_required(VERSION 3.10)
cmake_policy(VERSION 3.10)
find_package(raven-cmake PATHS ${CMAKE_BINARY_DIR})

##############################################################################################################

project(fty-asset VERSION 1.0.0)

##############################################################################################################

raven_target(shared ${PROJECT_NAME}
    PUBLIC
        include/fty_asset_autoupdate.h
        include/fty_asset_dto.h
        include/fty-asset.h
        include/fty_asset_inventory.h
        include/fty_asset_library.h
        include/fty_asset_server.h
    SOURCES
        src/dbhelpers.cc
        src/dbhelpers.h
        src/fty_asset_autoupdate.cc
        src/fty_asset_inventory.cc
        src/fty_asset_private_selftest.cc
        src/fty_asset_server.cc
        src/total_power.cc
        src/total_power.h
        src/dns.cc
        src/dns.h
        src/fty-asset.cc
        src/fty_asset_dto.cc
        src/fty_asset_manipulation.cc
        src/fty_asset_manipulation.h
        src/topology_processor.cc
        src/topology_processor.h
        src/fty_asset_classes.h
        src/asset-server.cc
        src/asset-server.h

        src/topology/cleanup.h
        src/topology/dbtypes.h
        src/topology/topology_input_powerchain.cc
        src/topology/topology_input_powerchain.h
        src/topology/topology_location.cc
        src/topology/topology_location.h
        src/topology/topology_power.cc
        src/topology/topology_power.h

        src/topology/msg/asset_msg.cc
        src/topology/msg/asset_msg.h
        src/topology/msg/common_msg.cc
        src/topology/msg/common_msg.h

        src/topology/persist/assetcrud.cc
        src/topology/persist/assetcrud.h
        src/topology/persist/assettopology.cc
        src/topology/persist/assettopology.h
        src/topology/persist/dbhelpers2.cc
        src/topology/persist/dbhelpers2.h
        src/topology/persist/monitor.cc
        src/topology/persist/monitor.h
        src/topology/persist/persist_error.cc
        src/topology/persist/persist_error.h

        src/topology/shared/asset_general.cc
        src/topology/shared/asset_general.h
        src/topology/shared/data.cc
        src/topology/shared/data.h
        src/topology/shared/location_helpers.cc
        src/topology/shared/location_helpers.h
        src/topology/shared/utilspp.cc
        src/topology/shared/utilspp.h

        src/topology/db/topology2.cc
        src/topology/db/topology2.h

        src/actions/delete.cc
        src/actions/delete.h
    FLAGS
        -Wno-unused-parameter
        -Wno-ignored-qualifiers
    INCLUDE_DIRS include
    USES
        libczmq
        log4cplus
        cxxtools
        libfty_common_logging
        libfty_common
        libfty_proto
        libfty_common_messagebus
        libfty_asset_activator
        libfty_common_db
        libfty_common_mlm
        tntdb
        libmlm
        uuid
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

raven_target(exe ${PROJECT_NAME}-test
    SOURCES
        src/fty_asset_selftest.cc
    INCLUDE_DIRS include
    USES
        cxxtools
        libczmq
        tntdb
        ${PROJECT_NAME}
)

##############################################################################################################

