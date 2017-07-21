cmake_minimum_required(VERSION 3.5.1)
project(rfcommunity)

set(CMAKE_CXX_STANDARD 14)

## LIBRARY DECLERATION

find_package(Boost QUIET REQUIRED program_options system thread regex)
set(INCLUDE_DIRS
        include
        ${Boost_INCLUDE_DIRECTORIES}
        )

include_directories(${INCLUDE_DIRS})

add_library(rfcommunity ../../catkin_ws/src/bluetooth_action/include/rfcommunity/rfcommunity.cxx)

target_include_directories(rfcommunity PRIVATE
        ${INCLUDE_DIRS}
        )

target_link_libraries(rfcommunity
        ${Bluetooth_LIBRARIES}
        Boost::system
        Boost::thread
        Boost::regex
        Boost::program_options
        ${Boost_LIBRARIES}
        "${YARP_LIBRARIES};m;bluetooth"
        )


## EXECUTABLE DECLERATION

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
        rfcommunity
        )

