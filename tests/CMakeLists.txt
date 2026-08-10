if(${${PROJECT_NAME}_BUILD_TESTS})
    set(TESTPROJ ${PROJECT_NAME}_tests)

    set( CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    set( CMAKE_CXX_STANDARD_REQUIRED On)
    set( CMAKE_CXX_EXTENSIONS Off)
    # This will allow cmake aware IDE to pickup on library definitions
    set( CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    add_executable(${TESTPROJ})

    target_compile_features(${TESTPROJ} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})
    target_compile_options( ${TESTPROJ}
                            PRIVATE
                            $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest> )

    # ASAN and Coverage for MacOS and Linux (using Clang)
    if(((CMAKE_CXX_COMPILER_ID MATCHES [Cc][Ll][Aa][Nn][Gg])))
        message(STATUS "Using sanitizers for leak and address on ${CMAKE_CXX_COMPILER_ID}")
        target_compile_options(${TESTPROJ} PRIVATE "SHELL:-fsanitize=address,undefined -fprofile-instr-generate -fcoverage-mapping")
        target_link_options(${TESTPROJ} PRIVATE "SHELL:-fsanitize=address,undefined -fprofile-instr-generate -fcoverage-mapping")
        message(STATUS "Enable instrumentation for code coverage")
        target_compile_options(${TESTPROJ} PRIVATE -coverage -O0 -g)
        target_link_options(${TESTPROJ} PRIVATE -coverage)
    endif()

    if( CMAKE_BUILD_TYPE MATCHES [Dd][Ee][Bb][Uu][Gg])
        message(STATUS "Enable DEBUG pre-processor macros.. cmake_build_type:${CMAKE_BUILD_TYPE}")
        target_compile_definitions(${TESTPROJ} PRIVATE DEBUG=1 _DEBUG=1)
    endif()
    # Our tests depend on this preprocessor definition during CI/test mode to enable
    # access to protected members.
    target_compile_definitions(${TESTPROJ} PRIVATE cosmoscl_TESTING_MODE=1)

    # Dependencies
    message(STATUS "  Adding dependencies for CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
    CPMAddPackage("gh:google/googletest#v1.17.0")
    target_sources( ${TESTPROJ}
                    PRIVATE
                    ${PROJECT_SOURCE_DIR}/tests/testall.cpp)
    # Link dependencies..
    target_link_libraries(${TESTPROJ} PRIVATE
            GTest::gtest_main
            nlohmann_json::nlohmann_json
            restcl::restcl
            RunOnEnd::RunOnEnd
            SplitUri::SplitUri
            string2map::string2map
            asynchrony::asynchrony
            timethis::timethis
            AzureCppUtils::AzureCppUtils
            StringHelpers::StringHelpers)

    include(GoogleTest)
    gtest_discover_tests(${TESTPROJ} XML_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/tests/results")

    include(CTest)
    message(STATUS "  Finished configuring for ${PROJECT_NAME} -- ${PROJECT_NAME}_BUILD_TESTS = ${${PROJECT_NAME}_BUILD_TESTS}")
endif()
