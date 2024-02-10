cmake_minimum_required(VERSION 3.23)
project(DisruptorTests)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost 1.80.0 COMPONENTS system filesystem REQUIRED)
find_package(fmt REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${fmt_INCLUDE_DIRS})

add_executable(DisruptorTests main.cpp MoodyTests.cpp MoodyTests.h TestCommon.h DisruptorTests.cpp DisruptorTests.h)
include_directories(disruptor)
include_directories(Moody)

