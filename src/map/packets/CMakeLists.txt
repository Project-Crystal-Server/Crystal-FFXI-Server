add_subdirectory(c2s)
add_subdirectory(s2c)

# Retail PS2 client translation (packets/ps2/ps2_client.h)
file(GLOB PACKET_PS2_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ps2/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/ps2/*.h)

set(PACKET_C2S_SOURCES
    ${PACKET_C2S_SOURCES}
    PARENT_SCOPE
)

set(PACKET_S2C_SOURCES
    ${PACKET_S2C_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/basic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/char_status.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/char_status.h
    ${CMAKE_CURRENT_SOURCE_DIR}/char_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/char_sync.h
    ${CMAKE_CURRENT_SOURCE_DIR}/char_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/char_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/entity_set_name.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/entity_set_name.h
    ${CMAKE_CURRENT_SOURCE_DIR}/entity_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/entity_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pet_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pet_sync.h
    ${PACKET_PS2_SOURCES}
    PARENT_SCOPE
)
