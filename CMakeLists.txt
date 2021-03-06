project(client)
cmake_minimum_required(VERSION 2.8)

include(ExternalProject)
include(FindPkgConfig)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

SET(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/)
SET(HDR ${CMAKE_CURRENT_SOURCE_DIR}/include/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libpiga/)

SET(HDRS
    ${HDR}/NetworkedClient/Application.hpp
    ${HDR}/NetworkedClient/Client.hpp
    ${HDR}/NetworkedClient/PlayerManager.hpp
)

SET(SRCS
    ${SRC}/Application.cpp
    ${SRC}/Client.cpp
    ${SRC}/PlayerManager.cpp
)

find_package(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${SCHEMAS})

add_executable(client ${SRCS} ${HDRS} ${PROTO_SRCS} ${PROTO_HDRS})
add_library(networked_client SHARED ${SRCS} ${HDRS} ${PROTO_SRCS} ${PROTO_HDRS})

if(${PROTOBUF_FOUND})
    target_link_libraries(client ${PROTOBUF_LIBRARIES})
    target_link_libraries(networked_client ${PROTOBUF_LIBRARIES})
    include_directories(${PROTOBUF_INCLUDE_DIRS})
endif()

find_package(Boost REQUIRED COMPONENTS system thread filesystem)

if(${Boost_FOUND})
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
    target_link_libraries(client ${Boost_LIBRARIES})
    target_link_libraries(networked_client ${Boost_LIBRARIES})
endif()

target_link_libraries(client pthread)

# Add sigc++2
find_package(SigC++ REQUIRED)
if(${SIGC++_FOUND})
    include_directories(${SIGC++_INCLUDE_DIR})
    target_link_libraries(client ${SIGC++_LIBRARY})
    target_link_libraries(networked_client ${SIGC++_LIBRARY})
elseif()
    message(STATUS "Did not find Sigc++2! Please install it before you try to compile again.")
endif()

ExternalProject_Add(GLM_Project
    PREFIX glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.5.4
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

add_dependencies(client GLM_Project)

include_directories(${INCLUDE})
# GLM Headers in the glm/ path.
include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR}/glm/src/GLM_Project/)

# Add yaml-cpp
find_package(YamlCpp REQUIRED)
if(${YAMLCPP_FOUND})
    include_directories(${YAMLCPP_INCLUDE_DIR})
    target_link_libraries(client ${YAMLCPP_LIBRARY})
    target_link_libraries(networked_client ${YAMLCPP_LIBRARY})
elseif()
    message(STATUS "Did not find Yaml-Cpp! Please install it before you try to compile again.")
endif()

# Add enet
find_package(ENet REQUIRED)
if(${ENET_FOUND})
    include_directories(${ENet_INCLUDE_DIRS})
    target_link_libraries(client ${ENet_LIBRARIES})
    target_link_libraries(networked_client ${ENet_LIBRARIES})
endif()

add_dependencies(networked_client piga)
add_dependencies(client networked_client)
include_directories(${LIBPIGA_INCLUDE_DIRECTORIES})
target_link_libraries(client piga)
target_link_libraries(networked_client piga)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Activate C++11
list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -lrt -ldl ${CMAKE_CXX_FLAGS}")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    list(APPEND ${CMAKE_CXX_FLAGS} "-Wall -std=c++11 -lrt -ldl ${CMAKE_CXX_FLAGS}")
endif()
