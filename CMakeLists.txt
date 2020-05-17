cmake_minimum_required(VERSION 3.12)
project(FEC_Erasure_Code)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_BUILD_TYPE Debug)

# Compiler Flags
set (CMAKE_CXX_FLAGS "-g")
set (CMAKE_BUILD_TYPE Debug)

# Directories
set (PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})
set (PROJECT_SOURCE_DIR ${PROJECT_ROOT_DIR}/src)
set (PROJECT_HEADER_DIR ${PROJECT_ROOT_DIR}/include)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_DIR}/bin)

find_package (Boost 1.67.0 COMPONENTS REQUIRED
        system date_time thread log coroutine)

# Includes
include_directories ("/usr/local/include")
include_directories ("${PROJECT_HEADER_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("/usr/include")

# Source Files
file (GLOB FEC_PARTIAL_SOURCES "${PROJECT_SOURCE_DIR}/*.cpp")

# Executables

#include_directories(${CMAKE_SOURCE_DIR}/include)
#link_directories(${CMAKE_SOURCE_DIR}/lib)

link_directories("/usr/local/lib")
link_directories("/usr/lib")

add_library (fec_part "${FEC_PARTIAL_SOURCES}")

#relay_main.cpp
add_executable (FEC_Erasure_Code_Relay
        ${PROJECT_ROOT_DIR}/application_relay_main.cpp)
target_link_libraries (FEC_Erasure_Code_Relay
        fec_part
        ${Boost_LIBRARIES}
        isal)

#sender_main.cpp
add_executable (FEC_Erasure_Code_Sender
        ${PROJECT_ROOT_DIR}/application_sender_main.cpp)
target_link_libraries (FEC_Erasure_Code_Sender
        fec_part
        ${Boost_LIBRARIES}
        isal)

#receiver_main.cpp
add_executable (FEC_Erasure_Code_Receiver
        ${PROJECT_ROOT_DIR}/application_receiver_main.cpp)
target_link_libraries (FEC_Erasure_Code_Receiver
        fec_part
        ${Boost_LIBRARIES}
        isal)

#local_simulation_main.cpp
add_executable (FEC_Erasure_Code_Local
        ${PROJECT_ROOT_DIR}/application_local_simulation.cpp)
target_link_libraries (FEC_Erasure_Code_Local
        fec_part
        ${Boost_LIBRARIES}
        isal)