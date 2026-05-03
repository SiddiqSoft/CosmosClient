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
    # Test Executable: testall.cpp
    # ============================================================================
    # This executable contains all tests (validation, connection, endpoint, integration)
    # Tests are organized by category:
    # - Validation tests (no emulator required) - can run in parallel
    # - Connection tests (no emulator required) - can run in parallel
    # - Endpoint tests (no emulator required) - can run in parallel
    # - Integration tests (requires emulator) - must run sequentially
    
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
    # CTest Configuration - Parallel & Sequential Test Execution
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
    
    # ============================================================================
    # Catch-All Test - Runs all tests in the executable
    # This ensures ctest discovers and runs all tests when invoked without filters
    # ============================================================================
    add_test(
        NAME ${TEST_ALL_TARGET}_all
        COMMAND ${TEST_ALL_TARGET} --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/all_tests_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_all PROPERTIES TIMEOUT 3600)
    
    # ============================================================================
    # Non-Emulator Tests (Validation, Connection, Endpoint)
    # These tests can run in PARALLEL since they don't require emulator
    # ============================================================================
    add_test(
        NAME ${TEST_ALL_TARGET}_validation
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=Validation.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/validation_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_validation PROPERTIES TIMEOUT 300)
    
    add_test(
        NAME ${TEST_ALL_TARGET}_connection
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=CosmosConnection.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/connection_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_connection PROPERTIES TIMEOUT 300)
    
    add_test(
        NAME ${TEST_ALL_TARGET}_endpoint
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=CosmosEndpoint.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/endpoint_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_endpoint PROPERTIES TIMEOUT 300)
    
    add_test(
        NAME ${TEST_ALL_TARGET}_comprehensive_connection
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=ComprehensiveConnectionTests.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/comprehensive_connection_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_comprehensive_connection PROPERTIES TIMEOUT 300)
    
    add_test(
        NAME ${TEST_ALL_TARGET}_comprehensive_response
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=ComprehensiveResponseTypeTests.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/comprehensive_response_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_comprehensive_response PROPERTIES TIMEOUT 300)
    
    add_test(
        NAME ${TEST_ALL_TARGET}_comprehensive_endpoint
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=ComprehensiveEndpointTests.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/comprehensive_endpoint_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_comprehensive_endpoint PROPERTIES TIMEOUT 300)
    
    add_test(
        NAME ${TEST_ALL_TARGET}_comprehensive_error
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=ComprehensiveErrorHandlingTests.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/comprehensive_error_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_comprehensive_error PROPERTIES TIMEOUT 300)
    
    # ============================================================================
    # Emulator-Required Tests (Integration & Comprehensive API)
    # These tests MUST run SEQUENTIALLY due to shared database/collection setup
    # ============================================================================
    add_test(
        NAME ${TEST_ALL_TARGET}_integration
        COMMAND ${TEST_ALL_TARGET} --gtest_filter=CosmosIntegrationTests.* --gtest_output=xml:${PROJECT_SOURCE_DIR}/tests/results/integration_test_detail.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(${TEST_ALL_TARGET}_integration PROPERTIES TIMEOUT 1800)
    
    # ============================================================================
    # Test Execution Strategy
    # ============================================================================
    # Non-emulator tests can run in parallel (up to 7 jobs)
    # Emulator tests must run sequentially (1 job)
    # Set CTEST_PARALLEL_LEVEL environment variable to control parallelism:
    #   - For parallel non-emulator tests: CTEST_PARALLEL_LEVEL=7
    #   - For sequential emulator tests: CTEST_PARALLEL_LEVEL=1
    # ============================================================================
    
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
    message(STATUS "  ")
    message(STATUS "  CTest configured with PARALLEL and SEQUENTIAL test execution:")
    message(STATUS "  ")
    message(STATUS "  CATCH-ALL TEST (Runs all tests):")
    message(STATUS "    - ${TEST_ALL_TARGET}_all (All tests in the executable)")
    message(STATUS "  ")
    message(STATUS "  PARALLEL TESTS (No Emulator Required) - 7 tests:")
    message(STATUS "    - ${TEST_ALL_TARGET}_validation (Validation tests)")
    message(STATUS "    - ${TEST_ALL_TARGET}_connection (Connection tests)")
    message(STATUS "    - ${TEST_ALL_TARGET}_endpoint (Endpoint tests)")
    message(STATUS "    - ${TEST_ALL_TARGET}_comprehensive_connection (Comprehensive connection tests)")
    message(STATUS "    - ${TEST_ALL_TARGET}_comprehensive_response (Response type tests)")
    message(STATUS "    - ${TEST_ALL_TARGET}_comprehensive_endpoint (Endpoint rotation tests)")
    message(STATUS "    - ${TEST_ALL_TARGET}_comprehensive_error (Error handling tests)")
    message(STATUS "  ")
    message(STATUS "  SEQUENTIAL TESTS (Requires Emulator) - 1 test:")
    message(STATUS "    - ${TEST_ALL_TARGET}_integration (Integration tests with shared setup/teardown)")
    message(STATUS "  ")
    message(STATUS "  Test output directory: ${PROJECT_SOURCE_DIR}/tests/results/")
    message(STATUS "  Catch-all test timeout: 3600 seconds")
    message(STATUS "  Parallel test timeout: 300 seconds each")
    message(STATUS "  Sequential test timeout: 1800 seconds")
    message(STATUS "  ")
    message(STATUS "  To run all tests (catch-all):")
    message(STATUS "    ctest -R '_all'")
    message(STATUS "  ")
    message(STATUS "  To run all tests with parallelism:")
    message(STATUS "    ctest --parallel 7")
    message(STATUS "  ")
    message(STATUS "  To run only parallel tests (no emulator required):")
    message(STATUS "    ctest --parallel 7 -R '_validation|_connection|_endpoint|_comprehensive'")
    message(STATUS "  ")
    message(STATUS "  To run only emulator tests sequentially:")
    message(STATUS "    ctest --parallel 1 -R '_integration'")
endif()
