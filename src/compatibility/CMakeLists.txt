# xi_compatibility: older clients, the parts both the map and the search server need. Platforms and versions
# (version.h), each platform's releases and the protocol each speaks (releases.h, platforms/) and item sets
# (item_set.h).
#
# Nothing here may depend on map or search code. Each server keeps its protocol specifics in its own compatibility
# folder: src/map/compatibility/ (profiles, protocols and their packet translators, id maps, guards) and
# src/search/compatibility/ (search layouts).
file(GLOB_RECURSE COMPATIBILITY_SHARED_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

xi_add_library(xi_compatibility ${COMPATIBILITY_SHARED_SOURCES})
# Its headers are included as "compatibility/..."
target_include_directories(xi_compatibility PUBLIC ${CMAKE_SOURCE_DIR}/src)
