cmake_minimum_required(VERSION 3.7)
project(rfcommunity)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost QUIET REQUIRED program_options system thread regex)
set(INCLUDE_DIRS
        include
        ${Boost_INCLUDE_DIRECTORIES}
        )

include_directories(${INCLUDE_DIRS})

add_library(rfcommunity include/rfcommunity.cxx)

set(SOURCE_FILES main.cpp)
add_executable(main ${SOURCE_FILES})

target_link_libraries(main
        ${Bluetooth_LIBRARIES}
        Boost::system
        Boost::thread
        Boost::regex
        Boost::program_options
        ${Boost_LIBRARIES}
        "${YARP_LIBRARIES};m;bluetooth"
        )

