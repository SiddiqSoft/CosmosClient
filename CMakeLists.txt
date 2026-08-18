#   SiddiqSoft CMakeLists 
#   Copyright 2024 Abdulkareem Siddiq. All rights reserved.
#   See LICENSE file
#
#   When building from our CI server, the following options must be set
#   BUILD_TESTS ON
#   CI_BUILDID must be set to the gitversion
# 
cmake_minimum_required(VERSION 3.29)

# _________________
# Version detection; must preceed the declaration of the actual project()
# Figures out the value ${CURRENT_PROJECT_VERSION} and therefore the ${PROJECT_VERSION}
# Brings in the CPM module into pack/CPM.make
if(WIN32)
    if(DEFINED ENV{LOCALAPPDATA})
        set(CPM_SOURCE_CACHE "$ENV{LOCALAPPDATA}/CPM/.cpmcache")
    elseif(DEFINED ENV{USERPROFILE})
        set(CPM_SOURCE_CACHE "$ENV{USERPROFILE}/AppData/Local/CPM/.cpmcache")
    else()
        set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/.cpmcache")
    endif()
else()
    set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/.cpmcache")
endif()
set(CPM_SOURCE_CACHE ${CPM_SOURCE_CACHE} CACHE PATH "CPM source cache")
include(pack/CMakeCommonHelpers.cmake)

# ______________________
# Project Metadata block
project(cosmoscl
        VERSION         "${CURRENT_PROJECT_VERSION}"
        LANGUAGES       CXX
        HOMEPAGE_URL    "%%myurl%%"
        DESCRIPTION     "Azure Cosmos DB REST client for modern C++")


# ____________________________________
# Options; define and declare defaults
option(${PROJECT_NAME}_BUILD_TESTS "${PROJECT_NAME} - Build tests. Uncheck for install only runs" OFF)
option(CI_BUILDID "Version of this build" "0.0.0")
option(${PROJECT_NAME}_DEBUG_TRACE "${PROJECT_NAME} - Extended debug mode. Only enable for local debugging." OFF)

if(NOT DEFINED CMAKE_CXX_STANDARD OR "${CMAKE_CXX_STANDARD}" STREQUAL "")
    set(CMAKE_CXX_STANDARD 23)
endif()
set( CMAKE_CXX_STANDARD_REQUIRED On)
set( CMAKE_CXX_EXTENSIONS OFF)
# This will allow cmake aware IDE to pickup on library definitions
set( CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ____________________________________
#  Library Definition
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                            INTERFACE
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

# The following is needed to make the .natvis file available to the IDE for debugging
target_sources(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SiddiqSoft.CosmosClient.natvis>)

# Install natvis file for Visual Studio debugger visualization
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SiddiqSoft.CosmosClient.natvis
        DESTINATION share/natvis
        OPTIONAL
        COMPONENT natvis)

# ____________________________________
# Dependencies
if(((CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg]) 
        OR (CMAKE_CXX_COMPILER_ID MATCHES [Gg][Nn][Uu])))
    # Linux/Unix specific
    # Make sure you install libcurl-devel on Linux
    find_package(CURL 8.7 REQUIRED)
    # Note: Do not use include_directories(${CURL_INCLUDE_DIRS}) here.
    # The CURL::libcurl imported target already provides include paths as SYSTEM includes,
    # which avoids conflicts between Homebrew LLVM's C++ headers and the macOS SDK C headers.
    target_link_libraries(${PROJECT_NAME} INTERFACE CURL::libcurl)
elseif((CMAKE_CXX_COMPILER_ID MATCHES [Mm][Ss][Vv][Cc]))
    # Windows specific work
    CPMAddPackage("gh:SiddiqSoft/acw32h#2.7.4")
    target_link_libraries(${PROJECT_NAME} INTERFACE acw32h::acw32h)
endif()

CPMAddPackage("gh:nlohmann/json#v3.12.0")
target_link_libraries(${PROJECT_NAME} INTERFACE nlohmann_json::nlohmann_json)

CPMAddPackage("gh:SiddiqSoft/SplitUri#3.0.3")
target_link_libraries(${PROJECT_NAME} INTERFACE SplitUri::SplitUri)
CPMAddPackage("gh:SiddiqSoft/string2map#2.6.1")
target_link_libraries(${PROJECT_NAME} INTERFACE string2map::string2map)
CPMAddPackage("gh:SiddiqSoft/GenProcessInfo#2.6.1")
target_link_libraries(${PROJECT_NAME} INTERFACE GenProcessInfo::GenProcessInfo)
CPMAddPackage("gh:SiddiqSoft/TimeThis#2.4.1")
target_link_libraries(${PROJECT_NAME} INTERFACE timethis::timethis)

# This will bring in a set of dependencies that are required for the restcl library to work.
# arrp, asynchrony, azurecpputils, runonend and ctre
CPMAddPackage("gh:SiddiqSoft/restcl#2.4.2")
target_compile_options(${PROJECT_NAME} INTERFACE restcl_DEBUG_TRACE ${${PROJECT_NAME}_DEBUG_TRACE})
target_link_libraries(${PROJECT_NAME} INTERFACE restcl::restcl)

# ____________________________________
# Testing
# Available only when building in our repo; controlled by the switch: ${PROJECT_NAME}_BUILD_TESTS = ON
if(${${PROJECT_NAME}_BUILD_TESTS} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" )
    message(STATUS "${PROJECT_NAME} - Asked to build tests.. ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    enable_testing()
    add_subdirectory(tests)
endif()
