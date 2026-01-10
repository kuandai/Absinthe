cmake_minimum_required(VERSION 3.20)
project(absinthe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

set(BOTCRAFT_ROOT "/usr/local/lib/botcraft")
set(BOTCRAFT_LIB_DIR "${BOTCRAFT_ROOT}/bin")
set(BOTCRAFT_INCLUDE_DIR "${BOTCRAFT_ROOT}/botcraft/include")
set(PROTOCOLCRAFT_INCLUDE_DIR "${BOTCRAFT_ROOT}/protocolCraft/include")
set(BOTCRAFT_BUILD_DIR "${BOTCRAFT_ROOT}/build-headless")

if(NOT EXISTS "${BOTCRAFT_BUILD_DIR}/protocol.txt")
    set(BOTCRAFT_BUILD_DIR "${BOTCRAFT_ROOT}/build")
endif()

set(BOTCRAFT_PROTOCOL_VERSION 774)
if(EXISTS "${BOTCRAFT_BUILD_DIR}/protocol.txt")
    file(READ "${BOTCRAFT_BUILD_DIR}/protocol.txt" BOTCRAFT_PROTOCOL_VERSION)
    string(STRIP "${BOTCRAFT_PROTOCOL_VERSION}" BOTCRAFT_PROTOCOL_VERSION)
endif()

set(BOTCRAFT_ENABLE_ENCRYPTION OFF)
set(BOTCRAFT_ENABLE_COMPRESSION OFF)
if(EXISTS "${BOTCRAFT_BUILD_DIR}/CMakeCache.txt")
    file(READ "${BOTCRAFT_BUILD_DIR}/CMakeCache.txt" BOTCRAFT_CMAKE_CACHE)
    if(BOTCRAFT_CMAKE_CACHE MATCHES "BOTCRAFT_ENCRYPTION:BOOL=ON")
        set(BOTCRAFT_ENABLE_ENCRYPTION ON)
    endif()
    if(BOTCRAFT_CMAKE_CACHE MATCHES "BOTCRAFT_COMPRESSION:BOOL=ON")
        set(BOTCRAFT_ENABLE_COMPRESSION ON)
    endif()
endif()

add_library(botcraft SHARED IMPORTED)
set_target_properties(botcraft PROPERTIES
    IMPORTED_LOCATION "${BOTCRAFT_LIB_DIR}/libbotcraft.so"
)

add_library(protocolCraft SHARED IMPORTED)
set_target_properties(protocolCraft PROPERTIES
    IMPORTED_LOCATION "${BOTCRAFT_LIB_DIR}/libprotocolCraft.so"
)


file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

list(REMOVE_ITEM SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli/main.cpp
)

add_library(Absinthe ${SOURCES})

target_include_directories(Absinthe
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        "${BOTCRAFT_INCLUDE_DIR}"
        "${PROTOCOLCRAFT_INCLUDE_DIR}"
)
target_link_libraries(Absinthe
    PUBLIC
        botcraft
        protocolCraft
)
target_compile_definitions(Absinthe
    PRIVATE
        PROTOCOL_VERSION=${BOTCRAFT_PROTOCOL_VERSION}
)
if(BOTCRAFT_ENABLE_ENCRYPTION)
    target_compile_definitions(Absinthe PRIVATE USE_ENCRYPTION=1)
endif()
if(BOTCRAFT_ENABLE_COMPRESSION)
    target_compile_definitions(Absinthe PRIVATE USE_COMPRESSION=1)
endif()

add_executable(${PROJECT_NAME} src/cli/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${BOTCRAFT_INCLUDE_DIR}"
        "${PROTOCOLCRAFT_INCLUDE_DIR}"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Absinthe
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH "${BOTCRAFT_LIB_DIR}"
)
