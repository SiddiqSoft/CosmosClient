if(${${PROJECT_NAME}_BUILD_TESTS})
    set( CMAKE_CXX_STANDARD 23)
    set( CMAKE_CXX_STANDARD_REQUIRED On)
    set( CMAKE_CXX_EXTENSIONS Off)
    
    # Common compiler configuration
    set(COMMON_COMPILE_OPTIONS
        $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
        $<$<CXX_COMPILER_ID:Clang>:-fexperimental-library>
    )
    
    # ASAN only for Debug builds on Linux (coverage profiling removed)
    if(((CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg]) 
        OR (CMAKE_CXX_COMPILER_ID MATCHES [Gg][Nn][Uu]))
       AND (CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg]))
        #message(STATUS "Using sanitizers for leak and address on ${CMAKE_CXX_COMPILER_ID}")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,leak")
        #target_link_libraries(${TESTPROJ} PRIVATE -fsanitize=address,leak)
        #message(STATUS "Enable instrumentation for code coverage")
        #target_compile_options(${TESTPROJ} PRIVATE -coverage -O0 -g)
        #target_link_options(${TESTPROJ} PRIVATE -coverage)
    endif()

    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        message(STATUS "  Setting DEBUG macro..")
        set(DEBUG_DEFS DEBUG=1 _DEBUG=1)
    endif()

    # On macOS with Homebrew LLVM, the CURL::libcurl imported target adds the Xcode SDK
    # include path (-isystem .../MacOSX.sdk/usr/include) which conflicts with LLVM's own
    # libc++ headers. The SDK's C headers get found before LLVM's stddef.h wrapper,
    # causing "unknown type name 'size_t'" errors.
    # Fix: Tell CMake that the SDK include path is an implicit system directory so it
    # won't be emitted as an explicit -isystem flag on the command line.
    if(CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg] AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        execute_process(
            COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE _sdk_path
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_sdk_path AND EXISTS "${_sdk_path}/usr/include")
            list(APPEND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "${_sdk_path}/usr/include")
        endif()
    endif()

    # Dependencies
    CPMAddPackage("gh:google/googletest#v1.17.0")
    
    # Ensure test results directory exists
    file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/results")
    
    # ============================================================================
    # Unified Test Executable: testall.cpp
    # ============================================================================
    # This single executable contains all integration tests with shared setup/teardown
    # Tests are organized by category:
    # - Validation tests (no emulator required)
    # - Connection tests (no emulator required)
    # - Endpoint tests (no emulator required)
    # - Integration tests (requires emulator)
    
    set(TEST_ALL_TARGET ${PROJECT_NAME}_tests)
    add_executable(${TEST_ALL_TARGET})
    
    target_compile_features(${TEST_ALL_TARGET} PRIVATE cxx_std_23)
    target_compile_options(${TEST_ALL_TARGET} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_compile_definitions(${TEST_ALL_TARGET} PRIVATE cosmoscl_TESTING_MODE=1 ${DEBUG_DEFS})
    
    target_sources(${TEST_ALL_TARGET} PRIVATE 
        ${PROJECT_SOURCE_DIR}/tests/testall.cpp
    )
    
    target_include_directories(${TEST_ALL_TARGET} PRIVATE
        ${PROJECT_SOURCE_DIR}/tests
        ${PROJECT_SOURCE_DIR}/include
    )
    
    target_link_libraries(${TEST_ALL_TARGET} PRIVATE
            GTest::gtest_main
            nlohmann_json::nlohmann_json
            restcl::restcl
            RunOnEnd::RunOnEnd
            timethis::timethis
            SplitUri::SplitUri
            string2map::string2map
            asynchrony::asynchrony
            AzureCppUtils::AzureCppUtils
            StringHelpers::StringHelpers)
    
    # ============================================================================
    # CTest Configuration
    # ============================================================================
    include(CTest)
    
    # Determine output file name based on preset or build type
    if(DEFINED CMAKE_BUILD_TYPE)
        if(CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
            set(TEST_OUTPUT_NAME "debug_test_detail.xml")
        else()
            set(TEST_OUTPUT_NAME "release_test_detail.xml")
        endif()
    else()
        set(TEST_OUTPUT_NAME "test_detail.xml")
    endif()
    
    # Add single test entry - all tests run in one process with shared setup/teardown
    add_test(
        NAME ${TEST_ALL_TARGET}
        COMMAND ${TEST_ALL_TARGET} --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/${TEST_OUTPUT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Set timeout for the entire test suite
    set_tests_properties(${TEST_ALL_TARGET} PROPERTIES TIMEOUT 1800)
    
    # Configure CTest to run tests sequentially (single process)
    set(CTEST_PARALLEL_LEVEL 1 CACHE STRING "Number of parallel test jobs (1 = sequential)")
    
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    message(STATUS "  CTest configured to run ALL tests in a single process:")
    message(STATUS "    - ${TEST_ALL_TARGET} (unified integration tests)")
    message(STATUS "  SetUpTestCase/TearDownTestCase run once for all tests")
    message(STATUS "  Total test timeout: 1800 seconds (30 minutes)")
    message(STATUS "  Test output: ${PROJECT_SOURCE_DIR}/tests/results/${TEST_OUTPUT_NAME}")
endif()
