add_subdirectory(packets)

# Older clients, the search server's part: each protocol's search layout (compatibility/search_layout.h,
# compatibility/protocols/<name>/). What it shares with the map server is xi_compatibility, in src/compatibility/.
file(GLOB_RECURSE SEARCH_COMPATIBILITY_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.h)

set(SOURCES
    ${PACKET_SOURCES}
    ${SEARCH_COMPATIBILITY_SOURCES}
    data_loader.cpp
    data_loader.h
    enums/search_type.h
    search_listener.h
    search_application.h
    search_application.cpp
    search_engine.h
    search_engine.cpp
    search_handler.h
    search_handler.cpp
)

xi_add_library(xi_search_lib ${SOURCES})
target_include_directories(xi_search_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xi_search_lib PUBLIC ${SEARCH_ONLY_EXTERNAL_LIBS} xi_compatibility)

xi_add_executable(xi_search searchserver.rc ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_link_libraries(xi_search PRIVATE xi_search_lib)
