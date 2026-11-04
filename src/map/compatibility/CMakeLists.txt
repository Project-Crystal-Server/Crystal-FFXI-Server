# Older clients, the map server's part: client identity (client.h), profiles (profile.h), protocols (protocol.h)
# and their packet translators (protocols/<name>/), id maps (id_maps.h) and soft-lock guards (guards.h).
# What it shares with the search server (versions, releases, item sets) is xi_compatibility, in src/compatibility/.
file(GLOB_RECURSE COMPATIBILITY_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(COMPATIBILITY_SOURCES
    ${COMPATIBILITY_SOURCES}
    PARENT_SCOPE
)
