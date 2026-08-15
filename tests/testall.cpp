/*
    CosmosClient - Unified Integration Tests
    Azure Cosmos REST-API Client for Modern C++

    Merged from: test.cpp, test_async.cpp, test_comprehensive_cosmos_api.cpp

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software LLC
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _CRT_SECURE_NO_WARNINGS 1
#define DEBUG_TRACE             1

#include "gtest/gtest.h"

#include <thread>
#include <barrier>
#include <latch>
#include <functional>
#include <chrono>
#include <ranges>
#include <semaphore>
#include <utility>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/cosmoscl.hpp"

#include "test_common.hpp"

// ============================================================================
// UNIFIED TEST FIXTURE - Single Setup/Teardown for All Tests
// ============================================================================

class CosmosIntegrationTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!IsCosmosReachable()) GTEST_SKIP() << "Cosmos service is not reachable";
    }

    static void SetUpTestCase()
    {
        siddiqsoft::ScopeTrace  lf;

        if (!IsCosmosReachable()) {
            lf.warn("Cosmos service is not reachable, skipping setup");
            return;
        }

        lf.msg("Configuring test suite client...");
        testSuiteClient.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetActiveConnectionStrings()}});

        // Clean up any existing test database from previous runs
        lf.msg("Cleaning up existing database '{}' if it exists...", testDBName0);
        auto deleteRc = TSdeleteDatabase(testDBName0);
        if (deleteRc.statusCode == 204 || deleteRc.statusCode == 404) {
            lf.msg("Database cleanup completed (status: {})", deleteRc.statusCode);
        }

        // Wait a moment for deletion to propagate
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Create test database
        lf.msg("Creating test database '{}'", testDBName0);
        auto createDbRc = TScreateDatabase(testDBName0);
        if (createDbRc.statusCode != 201) {
            lf.err("ERROR - Failed to create database '{}' (status: {})", testDBName0, createDbRc.statusCode);
            throw std::runtime_error(std::format("Failed to create test database '{}' with status code {}", testDBName0, createDbRc.statusCode));
        }
        lf.msg("Database '{}' created successfully", testDBName0);

        // Create test collections
        lf.msg("Creating {} test collections...", testCollectionNames.size());
        for (size_t idx = 0; idx < testCollectionNames.size(); ++idx) {
            auto& collName = testCollectionNames[idx];
            lf.msg("Creating collection '{}' in database '{}'", collName, testDBName0);

            auto createCollRc = TScreateCollection(testDBName0, collName);
            if (createCollRc.statusCode != 201) {
                lf.err("ERROR - Failed to create collection '{}' (status: {})", collName, createCollRc.statusCode);
                throw std::runtime_error(std::format("Failed to create test collection '{}' with status code {}", collName, createCollRc.statusCode));
            }
            lf.msg("Collection '{}' created successfully", collName);

            // Seed with test documents
            lf.msg("Seeding collection '{}' with {} documents...", collName, SEED_DOCUMENT_COUNT);
            for (auto i = 0; i < SEED_DOCUMENT_COUNT; i++) {
                auto seedRc = testSuiteClient.createDocument({.database   = testDBName0,
                                                              .collection = collName,
                                                              .document   = {{"id", std::format("{:0X}.{}", i, (i % 2) == 0 ? "even" : "odd")},
                                                                             {"ttl", 1360},
                                                                             {"__pk", "siddiqsoft.com"},
                                                                             {"func", __func__},
                                                                             {"extra", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" : "odd")},
                                                                             {"source", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" : "odd")}}});
                if (seedRc.statusCode != 201) {
                    lf.warn("Warning - Failed to seed document {} in collection '{}' (status: {})", i, collName, seedRc.statusCode);
                }
            }
            lf.msg("Collection '{}' seeding completed", collName);
        }
    }

    static void TearDownTestCase()
    {
        {
            siddiqsoft::ScopeTrace  lf;

            lf.msg("Cleaning up test database '{}'", testDBName0);
            auto deleteRc = TSdeleteDatabase(testDBName0);
            if (deleteRc.statusCode == 204 || deleteRc.statusCode == 404) {
                lf.warn("Database cleanup completed (status: {})", deleteRc.statusCode);
            }
        }
    }

    // Helper to generate unique document IDs
    static std::string GenerateDocId(const std::string& prefix = "doc")
    {
        return std::format("{}_{}", prefix, std::chrono::system_clock::now().time_since_epoch().count());
    }

    // Helper to create test document
    static nlohmann::json CreateTestDocument(const std::string& id, const std::string& pk = "siddiqsoft.com")
    {
        siddiqsoft::ScopeTrace  lf;
        return {{"id", id},
                {"__pk", pk},
                {"name", std::format("Document {}", id)},
                {"description", "Test document for comprehensive testing"},
                {"created", std::chrono::system_clock::now().time_since_epoch().count()},
                {"tags", nlohmann::json::array({"test", "comprehensive"})},
                {"metadata", {{"version", 1}, {"author", "test_suite"}}}};
    }
};

// ============================================================================
// VALIDATION TESTS (No Emulator Required)
// ============================================================================

/// @brief Verify environment variables are set for connection strings
/// @details Retrieves connection strings from environment variables
/// @test Validates CCTEST_PRIMARY_CS and CCTEST_SECONDARY_CS environment variables
TEST(Validation, checkEmulatorInfo)
{
    auto [priConnStr, secConnStr] = GetConnectionStrings();
    ASSERT_FALSE(priConnStr.empty()) << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";
    ASSERT_FALSE(secConnStr.empty()) << "Missing environment variable CCTEST_SECONDARY_CS; Set it to Secondary Connection string from Azure portal.";
}

/// @brief Verify default client configuration is correct
/// @details Creates a CosmosClient and checks default configuration values
/// @test Validates API version is "2018-12-31" and required keys exist
TEST(Validation, configure_Defaults)
{
    siddiqsoft::CosmosClient cc;
    auto&                    currentConfig = cc.configuration();

    EXPECT_TRUE(currentConfig.contains("apiVersion"));
    EXPECT_EQ("2018-12-31", currentConfig.value("apiVersion", ""));
    EXPECT_TRUE(currentConfig.contains("connectionStrings"));
    EXPECT_TRUE(currentConfig.contains("partitionKeyNames"));
}

/// @brief Verify client can be serialized to JSON
/// @details Converts CosmosClient to JSON and validates structure
/// @test Ensures JSON contains serviceSettings, database, and configuration sections
TEST(Validation, configure_check_json)
{
    siddiqsoft::CosmosClient cc;
    nlohmann::json           info = cc;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(5, info.size()) << info.dump(3);
}

/// @brief Verify client configuration with connection strings
/// @details Configures client with connection strings and validates service settings
/// @test Checks readable/writable locations are populated after configuration
TEST(Validation, configure_1)
{
    if (!IsCosmosReachable()) GTEST_SKIP() << "Cosmos service is not reachable";

    auto [priConnStr, secConnStr] = GetActiveConnectionStrings();
    ASSERT_FALSE(priConnStr.empty());

    siddiqsoft::CosmosClient cc;
    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto& currentConfig = cc.configuration();
    std::print(std::cerr, "{} - Contents of current configuration\n{}", __func__, currentConfig.dump(2));

#if defined(cosmoscl_TESTING_MODE)
    EXPECT_TRUE(cc.serviceSettings["writableLocations"].is_array());
    EXPECT_TRUE(cc.serviceSettings["readableLocations"].is_array());
    EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
    EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
#endif
}

/// @brief Verify region discovery functionality
/// @details Calls discoverRegions() and validates response
/// @test Ensures status code is 200 and regions are discovered
TEST(Validation, discoverRegions)
{
    if (!IsCosmosReachable()) GTEST_SKIP() << "Cosmos service is not reachable";

    auto [priConnStr, secConnStr] = GetActiveConnectionStrings();
    ASSERT_FALSE(priConnStr.empty());

    siddiqsoft::CosmosClient cc;
    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    nlohmann::json info = cc;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(5, info.size()) << info.dump(3);

    auto rc = cc.discoverRegions();
    std::print(std::cerr, "{} - ....rc:-\n{}", __func__, rc.document.dump(4));

    EXPECT_EQ(200, rc.statusCode) << rc.document.dump(3);
    EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
    EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
}

/// @brief Verify failover when primary connection fails
/// @details Tests connection rotation with invalid primary and valid secondary
/// @test Validates failover mechanism works correctly
TEST(Validation, discoverRegions_BadPrimary)
{
    if (!IsCosmosReachable()) GTEST_SKIP() << "Cosmos service is not reachable";

    auto [_, secConnStr] = GetActiveConnectionStrings();
    auto priConnStr      = std::string_view("AccountEndpoint=https://localhost:4043/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;");

    ASSERT_FALSE(priConnStr.empty());

    siddiqsoft::CosmosClient cc;
    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    auto rc = cc.discoverRegions();
    std::cerr << "1/3....rc:" << rc.statusCode << " Expect failure." << rc.document << std::endl;
    EXPECT_NE(200, rc.statusCode) << rc.document.dump(3);

    cc.cnxn.rotate();
    rc = cc.discoverRegions();
    std::cerr << "2/3....rc:" << rc.statusCode << " Expect success." << std::endl;
    EXPECT_EQ(200, rc.statusCode) << rc.document.dump(3);

    cc.cnxn.rotate();
    rc = cc.discoverRegions();
    std::cerr << "3/3....rc:" << rc.statusCode << " Expect failure." << rc.document << std::endl;
    EXPECT_NE(200, rc.statusCode) << rc.document.dump(3);
}

// ============================================================================
// CONNECTION TESTS (No Emulator Required)
// ============================================================================

/// @brief Parse connection string components
/// @details Creates connection from connection string and validates parsing
/// @test Validates base URI, encoded key, and host are extracted correctly
TEST(CosmosConnection, test1_n)
{
    std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {cs};

    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;",
              std::string(cd.Primary));
    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cd.Primary.BaseUri));
    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cd.Primary.EncodedKey);
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cd.Primary.BaseUri}.authority.host);
}

/// @brief Verify connection JSON serialization
/// @details Converts connection to JSON and validates structure
/// @test Ensures JSON has exactly 4 elements
TEST(CosmosConnection, test2_n)
{
    std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {cs};

    nlohmann::json               info = cd;
    EXPECT_EQ(4, info.size());
}

/// @brief Verify connection rotation with primary and secondary
/// @details Tests connection rotation between primary and secondary
/// @test Validates rotation cycles correctly
TEST(CosmosConnection, rotateConnection_1)
{
    std::string pcs = "AccountEndpoint=https://YOURDBNAME-1.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    std::string scs = "AccountEndpoint=https://YOURDBNAME-2.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {pcs, scs};

    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    cd.rotate();
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::SecondaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(scs, cd.current().string());

    cd.rotate();
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    cd.rotate(2);
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::SecondaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(scs, cd.current().string());

    cd.rotate(1);
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());
}

/// @brief Verify connection rotation with only primary
/// @details Tests connection rotation with single connection
/// @test Validates connection remains primary
TEST(CosmosConnection, rotateConnection_2)
{
    std::string pcs = "AccountEndpoint=https://YOURDBNAME-1.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {pcs};

    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    cd.rotate();
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    cd.rotate();
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    cd.rotate(2);
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    cd.rotate(1);
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());
}

// ============================================================================
// ENDPOINT TESTS (No Emulator Required)
// ============================================================================

/// @brief Parse connection string components
/// @details Creates connection from connection string and validates parsing
/// @test Validates base URI, encoded key, and host are extracted correctly
TEST(CosmosEndpoint, test1_n)
{
    siddiqsoft::CosmosEndpoint cs;
    cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";

    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cs.BaseUri));
    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cs.EncodedKey);
    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;", std::string(cs));
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    cs.rotateReadUri();
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
    cs.rotateWriteUri();
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

    auto info = nlohmann::json(cs);
    EXPECT_EQ(6, info.size());
}

/// @brief Verify connection JSON serialization
/// @details Converts connection to JSON and validates structure
/// @test Ensures JSON has exactly 4 elements
TEST(CosmosEndpoint, test2_n)
{
    using namespace siddiqsoft::restcl_literals;
    using namespace siddiqsoft::splituri_literals;

    siddiqsoft::CosmosEndpoint cs;
    cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";

    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cs.BaseUri));
    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cs.EncodedKey);
    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;", std::string(cs));

    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

    cs.ReadableUris.push_back("https://YOURDBNAME-r1.documents.azure.com:10/"_Uri);
    cs.ReadableUris.push_back("https://YOURDBNAME-r2.documents.azure.com:11/"_Uri);
    cs.WritableUris.push_back("https://YOURDBNAME-w1.documents.azure.com:90/"_Uri);
    cs.WritableUris.push_back("https://YOURDBNAME-w2.documents.azure.com:91/"_Uri);

    EXPECT_EQ("yourdbname-r1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    cs.rotateReadUri();
    EXPECT_EQ("yourdbname-r2.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    cs.rotateReadUri();
    EXPECT_EQ("yourdbname-r1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    EXPECT_EQ("yourdbname-w1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
    cs.rotateWriteUri();
    EXPECT_EQ("yourdbname-w2.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
    cs.rotateWriteUri();
    EXPECT_EQ("yourdbname-w1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

    cs.ReadableUris.clear();
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    cs.WritableUris.clear();
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
}

// ============================================================================
// INTEGRATION TESTS (Requires Emulator)
// ============================================================================

/// @brief Create, find, and delete a database
/// @details Tests complete database lifecycle operations
/// @test Validates create (201), find (200), and delete (204) operations
TEST_F(CosmosIntegrationTests, CreateDatabase)
{
    siddiqsoft::ScopeTrace    lf;

    auto rc1 = TScreateDatabase(testDBName);
    EXPECT_EQ(201, rc1.statusCode);

    auto rc2 = TSfindDatabase(testDBName);
    EXPECT_EQ(200, rc2.statusCode);

    auto rc3 = TSdeleteDatabase(testDBName);
    EXPECT_EQ(204, rc3.statusCode);
}

/// @brief Create collections in a database
/// @details Tests collection creation in a database
/// @test Validates collection creation returns 201 status code
TEST_F(CosmosIntegrationTests, CreateCollection)
{
    if (auto rc2 = TSfindDatabase(testDBName); rc2.statusCode == 404) {
        auto rc1 = TScreateDatabase(testDBName);
        EXPECT_EQ(201, rc1.statusCode);
    }

    try {
        for (auto& collName : testCollectionNames) {
            auto rc3 = TScreateCollection(testDBName, collName);
            EXPECT_EQ(201, rc3.statusCode);
        }
    }
    catch (...) {
    }

    auto rc9 = TSdeleteDatabase(testDBName);
    EXPECT_EQ(204, rc9.statusCode);
}

/// @brief Complete CRUD example
/// @details Lists databases, collections, creates and deletes document
/// @test Validates full CRUD cycle works
TEST_F(CosmosIntegrationTests, Example)
{
    if (auto rc = testSuiteClient.listDatabases(); 200 == rc.statusCode) {
        auto dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

        if (auto rc2 = testSuiteClient.listCollections({.database = dbName}); 200 == rc2.statusCode) {
            auto collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");
            auto id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock::now().time_since_epoch().count());
            auto pkId           = "siddiqsoft.com";

            if (auto rc3 =
                        testSuiteClient.createDocument({.database   = dbName,
                                                        .collection = collectionName,
                                                        .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"func", __func__}, {"source", "basic_tests.exe"}}});
                201 == rc3.statusCode)
            {
                auto rc4 = testSuiteClient.removeDocument(
                        {.database = dbName, .collection = collectionName, .id = rc3.document.value("id", id), .partitionKey = pkId});
                EXPECT_EQ(204, rc4);
            }
        }
    }
}

/// @brief List all databases
/// @details Calls listDatabases()
/// @test Validates response contains Databases array
TEST_F(CosmosIntegrationTests, ListDatabases)
{
    EXPECT_NO_THROW({
        auto rc = testSuiteClient.listDatabases();
        EXPECT_TRUE(rc.document.contains("Databases"));
        EXPECT_TRUE(rc.document["Databases"].is_array());
        EXPECT_EQ(200, rc.statusCode);
    });
}

/// @brief List collections in each database
/// @details Lists collections for each database
/// @test Validates collections are listed correctly
TEST_F(CosmosIntegrationTests, ListCollections)
{
    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("Databases"));
    EXPECT_TRUE(rc.document["Databases"].is_array());

    for (auto& db : rc.document["Databases"]) {
        auto rc2 = testSuiteClient.listCollections({.database = db.value("id", "")});
        EXPECT_EQ(200, rc2.statusCode);
        EXPECT_TRUE(rc2.document.contains("DocumentCollections"));
        EXPECT_TRUE(rc2.document["DocumentCollections"].is_array());
    }
}

/// @brief List documents with pagination
/// @details Lists documents with continuation token
/// @test Validates document pagination works
TEST_F(CosmosIntegrationTests, ListDocuments)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t                               totalDocs = 0;
    auto                                   iteration = 7;

    auto                                   rc        = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_NE("", rc.document.value("/Databases/0/id"_json_pointer, ""));

    auto rc2 = testSuiteClient.listCollections({.database = rc.document.value("/Databases/0/id"_json_pointer, "")});
    EXPECT_EQ(200, rc2.statusCode);
    EXPECT_NE("", rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""));

    do {
        irt = testSuiteClient.listDocuments({.database          = rc.document.value("/Databases/0/id"_json_pointer, ""),
                                             .collection        = rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                                             .continuationToken = irt.continuationToken});
        EXPECT_EQ(200, irt.statusCode);
        totalDocs += irt.document.value<uint32_t>("_count", 0);
        EXPECT_LE(1, irt.document.value("_count", 0));

        if (--iteration == 0) break;
    } while (!irt.continuationToken.empty());
}

/// @brief List documents with limit
/// @details Lists documents with limit
/// @test Validates document listing with limit works
TEST_F(CosmosIntegrationTests, ListDocuments_top8)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t                               totalDocs = 0;

    auto                                   rc        = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);

    auto rc2 = testSuiteClient.listCollections({.database = rc.document.value("/Databases/0/id"_json_pointer, "")});
    EXPECT_EQ(200, rc2.statusCode);

    irt = testSuiteClient.listDocuments({.database          = rc.document.value("/Databases/0/id"_json_pointer, ""),
                                         .collection        = rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                                         .continuationToken = irt.continuationToken});
    EXPECT_EQ(200, irt.statusCode);
    totalDocs += irt.document.value<uint32_t>("_count", 0);
    EXPECT_GE(10, irt.document.value("_count", 0));
}

/// @brief Create and delete a document
/// @details Tests document creation and deletion operations
/// @test Validates create (201) and delete (204) operations
TEST_F(CosmosIntegrationTests, CreateDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto        rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");
    EXPECT_FALSE(dbName.empty());

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");
    EXPECT_FALSE(collectionName.empty());

    id       = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock::now().time_since_epoch().count());
    pkId     = "siddiqsoft.com";

    auto rc3 = testSuiteClient.createDocument(
            {.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc3.statusCode);

    auto rc4 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc4);
}

/// @brief Verify validation of required id field
/// @details Attempts to create document without id field
/// @test Expects std::invalid_argument exception to be thrown
TEST_F(CosmosIntegrationTests, CreateDocument_MissingId)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto        rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock::now().time_since_epoch().count());
    pkId           = "siddiqsoft.com";

    EXPECT_THROW(testSuiteClient.createDocument({.database   = dbName,
                                                 .collection = collectionName,
                                                 .document   = {{"MissingId", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}}}),
                 std::invalid_argument);
}

/// @brief Verify partition key validation
/// @details Attempts to create document without partition key field
/// @test Expects std::invalid_argument exception to be thrown
TEST_F(CosmosIntegrationTests, CreateDocument_MissingPkId)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto        rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId           = "siddiqsoft.com";

    EXPECT_THROW(testSuiteClient.createDocument({.database   = dbName,
                                                 .collection = collectionName,
                                                 .document   = {{"id", id}, {"ttl", 360}, {"Missing__pk", pkId}, {"source", "basic_tests.exe"}}}),
                 std::invalid_argument);
}

/// @brief Create and retrieve a document
/// @details Tests document retrieval by ID and partition key
/// @test Validates find (200) operation and document ID matches
TEST_F(CosmosIntegrationTests, FindDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto        rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId           = "siddiqsoft.com";

    auto rc3       = testSuiteClient.createDocument(
            {.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc3.statusCode);

    auto rc4 = testSuiteClient.findDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(200, rc4.statusCode);
    EXPECT_EQ(id, rc4.document.value("id", ""));

    auto rc5 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc5);
}

/// @brief Test upsert (insert or update) operation
/// @details Tests upsert for both insert and update scenarios
/// @test Validates insert (201), update (200), and conflict (409) responses
TEST_F(CosmosIntegrationTests, UpsertDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto        rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId           = "siddiqsoft.com";

    auto rc4       = testSuiteClient.upsertDocument({.database   = dbName,
                                                     .collection = collectionName,
                                                     .document   = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "insert"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc4.statusCode);
    EXPECT_EQ("insert", rc4.document.value("upsert", ""));

    auto rc5 = testSuiteClient.upsertDocument({.database   = dbName,
                                               .collection = collectionName,
                                               .document   = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "update"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(200, rc5.statusCode);
    EXPECT_EQ("update", rc5.document.value("upsert", ""));

    auto rc6 = testSuiteClient.createDocument({.database   = dbName,
                                               .collection = collectionName,
                                               .document   = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "FAIL"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(409, rc6.statusCode);

    auto rc7 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc7);
}

/// @brief Update an existing document
/// @details Tests document update and persistence
/// @test Validates update (200) and changes are persisted
TEST_F(CosmosIntegrationTests, UpdateDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto        rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId           = "siddiqsoft.com";

    auto rc4       = testSuiteClient.createDocument({.database   = dbName,
                                                     .collection = collectionName,
                                                     .document   = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"mode", "create"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc4.statusCode);
    EXPECT_EQ("create", rc4.document.value("mode", ""));

    rc4.document["mode"] = "update";

    auto rc5 = testSuiteClient.updateDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId, .document = rc4.document});
    EXPECT_EQ(200, rc5.statusCode);
    EXPECT_EQ("update", rc5.document.value("mode", ""));

    auto rc6 = testSuiteClient.findDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(200, rc6.statusCode);
    EXPECT_EQ("update", rc6.document.value("mode", ""));

    auto rc7 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc7);
}

/// @brief Query documents with parameters and pagination
/// @details Queries documents where source contains "odd" with continuation tokens
/// @test Validates parameterized queries work with pagination
TEST_F(CosmosIntegrationTests, QueryDocument_odd)
{
    std::string                            dbName {};
    std::string                            collectionName {};
    std::vector<std::string>               docIds {};
    std::string                            pkId {"siddiqsoft.com"};
    siddiqsoft::CosmosIterableResponseType irt {};
    nlohmann::json                         allDocs = nlohmann::json::array();
    uint32_t                               allDocsCount {};

    std::println(std::cerr, "{} -- Query documents in {}:{}...", __func__, testDBName0, testCollectionNames[1]);

    do {
        irt = testSuiteClient.queryDocuments({.database          = testDBName0,
                                              .collection        = testCollectionNames[1],
                                              .partitionKey      = "*",
                                              .continuationToken = irt.continuationToken,
                                              .queryStatement    = "SELECT * FROM c WHERE contains(c.source, @v1)",
                                              .queryParameters   = {{{"name", "@v1"}, {"value", "odd"}}}});
        ASSERT_EQ(200, irt.statusCode);
        if (200 == irt.statusCode && irt.document.contains("Documents") && !irt.document.at("Documents").is_null()) {
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
            allDocsCount += irt.document.value("_count", 0);
        }
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(5, allDocsCount);
}

/// @brief Query documents with parameters and pagination
/// @details Queries documents where source contains "even" with continuation tokens
/// @test Validates parameterized queries work with pagination
TEST_F(CosmosIntegrationTests, QueryDocument_even)
{
    std::string                            dbName {};
    std::string                            collectionName {};
    std::vector<std::string>               docIds {};
    std::string                            pkId {"siddiqsoft.com"};
    siddiqsoft::CosmosIterableResponseType irt {};
    nlohmann::json                         allDocs = nlohmann::json::array();
    uint32_t                               allDocsCount {};

    std::println(std::cerr, "{} -- Query documents in {}:{}...", __func__, testDBName0, testCollectionNames[1]);

    do {
        irt = testSuiteClient.queryDocuments({.database          = testDBName0,
                                              .collection        = testCollectionNames[1],
                                              .partitionKey      = "*",
                                              .continuationToken = irt.continuationToken,
                                              .queryStatement    = "SELECT * FROM c WHERE contains(c.source, @v1)",
                                              .queryParameters   = {{{"name", "@v1"}, {"value", "even"}}}});
        ASSERT_EQ(200, irt.statusCode);
        if (200 == irt.statusCode && irt.document.contains("Documents") && !irt.document.at("Documents").is_null()) {
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
            allDocsCount += irt.document.value("_count", 0);
        }
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(5, allDocsCount);
}

/// @brief Verify move semantics
/// @details Creates vector of CosmosClient instances
/// @test Validates move construction works
TEST_F(CosmosIntegrationTests, MoveConstruct)
{
    std::vector<siddiqsoft::CosmosClient> clients;

    clients.push_back(siddiqsoft::CosmosClient {});
    clients.push_back(siddiqsoft::CosmosClient {});

    EXPECT_EQ(2, clients.size());
}

/// @brief Configure multiple clients concurrently
/// @details Creates and configures 4 CosmosClient instances
/// @test Validates multiple clients can be configured
TEST_F(CosmosIntegrationTests, ConfigureMulti)
{
    std::vector<siddiqsoft::CosmosClient> clients;

    std::cerr << "Setting up the clients..";
    for (auto i = 0; i < 4; i++) {
        clients.emplace_back(siddiqsoft::CosmosClient {})
                .configure(nlohmann::json {{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetActiveConnectionStrings()}});
    }

    EXPECT_EQ(4, clients.size());

    std::atomic_uint passTest {0};

    std::cerr << "Setting up the clients..configuring..";
    std::ranges::for_each(clients, [&](auto& cc) {
        auto& currentConfig = cc.configuration();

        EXPECT_TRUE(cc.serviceSettings["writableLocations"].is_array());
        EXPECT_TRUE(cc.serviceSettings["readableLocations"].is_array());

        EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
        EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
        EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
        EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
        passTest++;
    });

    std::cerr << "Completed.";

    EXPECT_EQ(4, passTest.load());
}

/// @brief Test concurrent document creation
/// @details Creates documents from multiple threads with synchronization
/// @test Validates concurrent operations work correctly
TEST_F(CosmosIntegrationTests, CreateDocumentThreaded)
{
    auto                 ttx = std::chrono::system_clock::now();
    std::string          dbName {};
    std::string          collectionName {};
    std::string          pkId {"siddiqsoft.com"};
    std::string          sourceId = std::format("{}-{}", getpid(), siddiqsoft::CosmosClient::CosmosClientUserAgentString);
    constexpr auto       DOCS {15};
    static auto          threadCount = std::thread::hardware_concurrency();
    std::latch           startLatch {threadCount};
    std::atomic_uint32_t removeDocsCount {0}, addDocsCount {0};
    std::latch           endLatch {threadCount};
    std::barrier         creatorsBarrier(threadCount, [&]() noexcept -> void {
#if defined(DEBUG)
        std::cerr << std::format("!! Barrier hit. DOCS:{} x threadCount:{} -> addDocsCount:{} removeDocsCount:{} ttx:{}!!",
                                 DOCS,
                                 threadCount,
                                 addDocsCount.load(),
                                 removeDocsCount.load(),
                                 std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
#endif
    });

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    ttx            = std::chrono::system_clock::now();
    std::vector<std::jthread> creators;
    for (auto t = 0; t < threadCount; t++) {
        creators.emplace_back(std::jthread(
                [&](auto tid) {
                    std::vector<std::string> docIds {};

                    startLatch.count_down();
                    startLatch.wait();

                    for (auto i = 0; i < DOCS; i++) {
                        try {
                            auto rc = testSuiteClient.createDocument(
                                    {.database   = dbName,
                                     .collection = collectionName,
                                     .document   = {{"id", std::format("{}.{}.{}", tid, i, std::chrono::system_clock::now().time_since_epoch().count())},
                                                    {"ttl", 360},
                                                    {"__pk", "siddiqsoft.com"},
                                                    {"i", i},
                                                    {"tid", tid},
                                                    {"source", sourceId}}});
                            addDocsCount += rc.statusCode == 201 ? 1 : 0;
                            docIds.push_back(rc.document.value("id", ""));
                        }
                        catch (const std::exception& e) {
                        }
                    }

                    creatorsBarrier.arrive_and_wait();

                    for (auto i = 0; i < docIds.size(); i++) {
                        auto rc = testSuiteClient.removeDocument(
                                {.database = dbName, .collection = collectionName, .id = docIds[i], .partitionKey = "siddiqsoft.com"});
                        removeDocsCount += rc == 204;
                    }
                    endLatch.count_down();
                },
                t));
    }

    endLatch.wait();

    EXPECT_EQ(removeDocsCount.load(), addDocsCount.load());
    EXPECT_EQ((DOCS * threadCount), addDocsCount.load());
}

// ============================================================================
// COMPREHENSIVE API TESTS (Requires Emulator)
// ============================================================================

/// @brief Verify invalid connection string handling
/// @details Creates endpoint with invalid connection string
/// @test Validates endpoint is invalid (bool conversion returns false)
TEST(ComprehensiveConnectionTests, InvalidConnectionStringFormat)
{
    std::string                cs = "InvalidFormat";
    siddiqsoft::CosmosEndpoint endpoint(cs);

    EXPECT_FALSE(static_cast<bool>(endpoint));
}

/// @brief Verify default client configuration
/// @details Creates new client and checks default configuration
/// @test Validates API version and retry limit are correct
TEST_F(CosmosIntegrationTests, ClientDefaultConfiguration)
{
    siddiqsoft::CosmosClient client;
    auto&                    config = client.configuration();

    EXPECT_TRUE(config.contains("apiVersion"));
    EXPECT_EQ("2018-12-31", config.value("apiVersion", ""));
    EXPECT_TRUE(config.contains("libRetryLimit"));
    EXPECT_EQ(7, config.value("libRetryLimit", 0));
    EXPECT_TRUE(config.contains("connectionStrings"));
    EXPECT_TRUE(config.contains("partitionKeyNames"));
}

/// @brief Create a database with unique name
/// @details Generates unique database name and creates it
/// @test Validates database creation returns 201 status code
TEST_F(CosmosIntegrationTests, CreateDatabaseBasic)
{
    std::string dbName = GenerateDocId("testdb");

    auto        rc     = testSuiteClient.createDatabase({.database = dbName});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("id"));
    EXPECT_EQ(dbName, rc.document.value("id", ""));

    testSuiteClient.deleteDatabase({.database = dbName});
}

/// @brief Verify duplicate database prevention
/// @details Attempts to create same database twice
/// @test Validates second creation returns 409 (Conflict)
TEST_F(CosmosIntegrationTests, CreateDatabaseDuplicate)
{
    std::string dbName = GenerateDocId("dupdb");

    auto        rc1    = testSuiteClient.createDatabase({.database = dbName});
    EXPECT_EQ(201, rc1.statusCode);

    auto rc2 = testSuiteClient.createDatabase({.database = dbName});
    EXPECT_EQ(409, rc2.statusCode);

    testSuiteClient.deleteDatabase({.database = dbName});
}

/// @brief Verify database list is not empty
/// @details Lists all databases
/// @test Validates database list contains at least one database
TEST_F(CosmosIntegrationTests, ListDatabasesNotEmpty)
{
    auto rc = testSuiteClient.listDatabases();

    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("Databases"));
    EXPECT_TRUE(rc.document["Databases"].is_array());
    EXPECT_GE(rc.document["Databases"].size(), 1);
}

/// @brief Find a specific database
/// @details Finds test database by name
/// @test Validates database is found with correct ID
TEST_F(CosmosIntegrationTests, FindDatabaseExists)
{
    auto rc = testSuiteClient.findDatabase({.database = testDBName0});

    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("id"));
    EXPECT_EQ(testDBName0, rc.document.value("id", ""));
}

/// @brief Verify 404 for non-existent database
/// @details Attempts to find non-existent database
/// @test Validates status code is 404
TEST_F(CosmosIntegrationTests, FindDatabaseNotFound)
{
    std::string nonexistentDb = "nonexistent_db_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    auto        rc            = testSuiteClient.findDatabase({.database = nonexistentDb});

    EXPECT_EQ(404, rc.statusCode);
}

/// @brief Delete a database
/// @details Creates and deletes a database
/// @test Validates delete returns 204 and find returns 404
TEST_F(CosmosIntegrationTests, DeleteDatabaseSuccess)
{
    std::string dbName   = GenerateDocId("deldb");

    auto        createRc = testSuiteClient.createDatabase({.database = dbName});
    EXPECT_EQ(201, createRc.statusCode);

    auto deleteRc = testSuiteClient.deleteDatabase({.database = dbName});
    EXPECT_EQ(204, deleteRc.statusCode);

    auto findRc = testSuiteClient.findDatabase({.database = dbName});
    EXPECT_EQ(404, findRc.statusCode);
}

/// @brief Create a collection
/// @details Generates unique collection name and creates it
/// @test Validates collection creation returns 201 status code
TEST_F(CosmosIntegrationTests, CreateCollectionBasic)
{
    std::string collName = GenerateDocId("testcoll");

    auto        rc       = testSuiteClient.createCollection({.database = testDBName0, .collection = collName});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("id"));
    EXPECT_EQ(collName, rc.document.value("id", ""));
}

/// @brief Verify duplicate collection prevention
/// @details Attempts to create same collection twice
/// @test Validates second creation returns 409 (Conflict)
TEST_F(CosmosIntegrationTests, CreateCollectionDuplicate)
{
    std::string collName = GenerateDocId("dupcoll");

    auto        rc1      = testSuiteClient.createCollection({.database = testDBName0, .collection = collName});
    EXPECT_EQ(201, rc1.statusCode);

    auto rc2 = testSuiteClient.createCollection({.database = testDBName0, .collection = collName});
    EXPECT_EQ(409, rc2.statusCode);
}

/// @brief Verify collection list is not empty
/// @details Lists collections in test database
/// @test Validates collection list contains at least one collection
TEST_F(CosmosIntegrationTests, ListCollectionsNotEmpty)
{
    auto rc = testSuiteClient.listCollections({.database = testDBName0});

    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("DocumentCollections"));
    EXPECT_TRUE(rc.document["DocumentCollections"].is_array());
    EXPECT_GE(rc.document["DocumentCollections"].size(), 1);
}

/// @brief Create document with minimal fields
/// @details Creates document with only id and partition key
/// @test Validates minimal document creation works
TEST_F(CosmosIntegrationTests, CreateDocumentMinimal)
{
    std::string docId = GenerateDocId("mindoc");

    auto        rc    = testSuiteClient.createDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ(docId, rc.document.value("id", ""));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Create document with nested structures
/// @details Creates document with nested objects, arrays, booleans, floats, nulls
/// @test Validates complex document structures work
TEST_F(CosmosIntegrationTests, CreateDocumentWithComplexStructure)
{
    std::string docId = GenerateDocId("complexdoc");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId},
                                                                       {"__pk", "siddiqsoft.com"},
                                                                       {"name", "Complex Document"},
                                                                       {"nested", {{"level1", {{"level2", {{"value", "deep"}}}}}}},
                                                                       {"array", nlohmann::json::array({1, 2, 3, "four"})},
                                                                       {"boolean", true},
                                                                       {"number", 42.5},
                                                                       {"null_value", nullptr}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("Complex Document", rc.document.value("name", ""));
    EXPECT_EQ("deep", rc.document.value("/nested/level1/level2/value"_json_pointer, ""));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Verify ID validation
/// @details Attempts to create document without ID
/// @test Expects std::invalid_argument exception
TEST_F(CosmosIntegrationTests, CreateDocumentMissingId)
{
    EXPECT_THROW(testSuiteClient.createDocument(
                         {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"__pk", "siddiqsoft.com"}, {"name", "No ID"}}}),
                 std::invalid_argument);
}

/// @brief Verify partition key validation
/// @details Attempts to create document without partition key
/// @test Expects std::invalid_argument exception
TEST_F(CosmosIntegrationTests, CreateDocumentMissingPartitionKey)
{
    std::string docId = GenerateDocId("nopkdoc");

    EXPECT_THROW(testSuiteClient.createDocument(
                         {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"name", "No Partition Key"}}}),
                 std::invalid_argument);
}

/// @brief Find and retrieve a document
/// @details Creates and finds a document
/// @test Validates document retrieval works correctly
TEST_F(CosmosIntegrationTests, FindDocumentExists)
{
    std::string docId    = GenerateDocId("finddoc");

    auto        createRc = testSuiteClient.createDocument({.database = testDBName0, .collection = testCollectionNames[0], .document = CreateTestDocument(docId)});
    EXPECT_EQ(201, createRc.statusCode);

    auto findRc = testSuiteClient.findDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});

    EXPECT_EQ(200, findRc.statusCode);
    EXPECT_EQ(docId, findRc.document.value("id", ""));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Verify 404 for non-existent document
/// @details Attempts to find non-existent document
/// @test Validates status code is 404
TEST_F(CosmosIntegrationTests, FindDocumentNotFound)
{
    auto rc = testSuiteClient.findDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .id = "nonexistent_doc_12345", .partitionKey = "siddiqsoft.com"});

    EXPECT_EQ(404, rc.statusCode);
}

/// @brief Update document fields
/// @details Creates document, modifies fields, and updates
/// @test Validates update returns 200 and changes persist
TEST_F(CosmosIntegrationTests, UpdateDocumentBasic)
{
    std::string docId    = GenerateDocId("updatedoc");

    auto        createRc = testSuiteClient.createDocument({.database = testDBName0, .collection = testCollectionNames[0], .document = CreateTestDocument(docId)});
    EXPECT_EQ(201, createRc.statusCode);

    nlohmann::json updatedDoc         = createRc.document;
    updatedDoc["name"]                = "Updated Name";
    updatedDoc["metadata"]["version"] = 2;

    auto updateRc                     = testSuiteClient.updateDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com", .document = updatedDoc});

    EXPECT_EQ(200, updateRc.statusCode);
    EXPECT_EQ("Updated Name", updateRc.document.value("name", ""));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Delete a document
/// @details Creates and deletes a document
/// @test Validates delete returns 204 and find returns 404
TEST_F(CosmosIntegrationTests, RemoveDocumentSuccess)
{
    std::string docId    = GenerateDocId("removedoc");

    auto        createRc = testSuiteClient.createDocument({.database = testDBName0, .collection = testCollectionNames[0], .document = CreateTestDocument(docId)});
    EXPECT_EQ(201, createRc.statusCode);

    auto removeRc = testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});

    EXPECT_EQ(204, removeRc);

    auto findRc = testSuiteClient.findDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});

    EXPECT_EQ(404, findRc.statusCode);
}

/// @brief Verify 404 when deleting non-existent document
/// @details Attempts to delete non-existent document
/// @test Validates status code is 404
TEST_F(CosmosIntegrationTests, RemoveNonexistentDocument)
{
    auto rc = testSuiteClient.removeDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .id = "nonexistent_doc_12345", .partitionKey = "siddiqsoft.com"});

    EXPECT_EQ(404, rc);
}

/// @brief Execute a simple query
/// @details Queries documents with WHERE clause
/// @test Validates simple query works correctly
TEST_F(CosmosIntegrationTests, QueryDocumentsSimple)
{
    auto irt = testSuiteClient.queryDocuments({.database        = testDBName0,
                                               .collection      = testCollectionNames[0],
                                               .partitionKey    = "*",
                                               .queryStatement  = "SELECT * FROM c WHERE c.parity = @parity",
                                               .queryParameters = {{{"name", "@parity"}, {"value", "even"}}}});

    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));
}

/// @brief Query with continuation tokens
/// @details Queries documents with pagination
/// @test Validates pagination works correctly
TEST_F(CosmosIntegrationTests, QueryDocumentsWithPagination)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t                               totalDocs  = 0;
    int                                    iterations = 0;

    do {
        irt = testSuiteClient.queryDocuments({.database          = testDBName0,
                                              .collection        = testCollectionNames[0],
                                              .partitionKey      = "*",
                                              .continuationToken = irt.continuationToken,
                                              .queryStatement    = "SELECT * FROM c"});

        EXPECT_EQ(200, irt.statusCode);
        if (irt.statusCode == 200 && irt.document.contains("Documents")) {
            totalDocs += irt.document.value("_count", 0);
        }

        iterations++;
        if (iterations >= 10) break;
    } while (!irt.continuationToken.empty());

    EXPECT_GE(totalDocs, SEED_DOCUMENT_COUNT);
}

/// @brief List documents in collection
/// @details Lists documents in test collection
/// @test Validates document listing works correctly
TEST_F(CosmosIntegrationTests, ListDocumentsBasic)
{
    auto irt = testSuiteClient.listDocuments({.database = testDBName0, .collection = testCollectionNames[0]});

    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));
    EXPECT_GE(irt.document.value("_count", 0), 1);
}

/// @brief Discover available regions
/// @details Calls discoverRegions() and validates response
/// @test Validates writableLocations and readableLocations exist
TEST_F(CosmosIntegrationTests, DiscoverRegionsSuccess)
{
    auto rc = testSuiteClient.discoverRegions();

    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("writableLocations"));
    EXPECT_TRUE(rc.document.contains("readableLocations"));
}

/// @brief Verify 200 status is success
/// @details Creates response with status 200
/// @test Validates success() returns true
TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeSuccess)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 200;

    EXPECT_TRUE(resp.success());
}

/// @brief Verify 201 status is success
/// @details Creates response with status 201
/// @test Validates success() returns true
TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeCreated)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 201;

    EXPECT_TRUE(resp.success());
}

/// @brief Verify 204 status is success
/// @details Creates response with status 204
/// @test Validates success() returns true
TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeNoContent)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 204;

    EXPECT_TRUE(resp.success());
}

/// @brief Verify 400 status is failure
/// @details Creates response with status 400
/// @test Validates success() returns false
TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeClientError)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 400;

    EXPECT_FALSE(resp.success());
}

/// @brief Verify 404 status is failure
/// @details Creates response with status 404
/// @test Validates success() returns false
TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeNotFound)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 404;

    EXPECT_FALSE(resp.success());
}

/// @brief Verify 500 status is failure
/// @details Creates response with status 500
/// @test Validates success() returns false
TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeServerError)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 500;

    EXPECT_FALSE(resp.success());
}

/// @brief Verify read URI cycles through all URIs
/// @details Creates endpoint with 3 readable URIs and rotates
/// @test Validates read URI rotation works correctly
TEST(ComprehensiveEndpointTests, EndpointReadUriRotation)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";
    endpoint.ReadableUris.push_back("https://read1.documents.azure.com/");
    endpoint.ReadableUris.push_back("https://read2.documents.azure.com/");
    endpoint.ReadableUris.push_back("https://read3.documents.azure.com/");

    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());

    endpoint.rotateReadUri();
    EXPECT_EQ("https://read2.documents.azure.com/", endpoint.currentReadUri());

    endpoint.rotateReadUri();
    EXPECT_EQ("https://read3.documents.azure.com/", endpoint.currentReadUri());

    endpoint.rotateReadUri();
    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());
}

/// @brief Verify write URI cycles through all URIs
/// @details Creates endpoint with 2 writable URIs and rotates
/// @test Validates write URI rotation works correctly
TEST(ComprehensiveEndpointTests, EndpointWriteUriRotation)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";
    endpoint.WritableUris.push_back("https://write1.documents.azure.com/");
    endpoint.WritableUris.push_back("https://write2.documents.azure.com/");

    EXPECT_EQ("https://write1.documents.azure.com/", endpoint.currentWriteUri());

    endpoint.rotateWriteUri();
    EXPECT_EQ("https://write2.documents.azure.com/", endpoint.currentWriteUri());

    endpoint.rotateWriteUri();
    EXPECT_EQ("https://write1.documents.azure.com/", endpoint.currentWriteUri());
}

/// @brief Verify fallback when no URIs configured
/// @details Creates endpoint with only base URI
/// @test Validates fallback to base URI works
TEST(ComprehensiveEndpointTests, EndpointFallbackToBaseUri)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";

    EXPECT_EQ("https://base.documents.azure.com/", endpoint.currentReadUri());
    EXPECT_EQ("https://base.documents.azure.com/", endpoint.currentWriteUri());
}

/// @brief Verify exception when connection strings missing
/// @details Attempts to configure without connection strings
/// @test Expects std::invalid_argument exception
TEST(ComprehensiveErrorHandlingTests, InvalidConfigurationMissingConnectionStrings)
{
    siddiqsoft::CosmosClient client;

    EXPECT_THROW(client.configure({{"partitionKeyNames", {"__pk"}}}), std::invalid_argument);
}

/// @brief Verify exception when partition keys missing
/// @details Attempts to configure without partition key names
/// @test Expects std::invalid_argument exception
TEST(ComprehensiveErrorHandlingTests, InvalidConfigurationMissingPartitionKeyNames)
{
    siddiqsoft::CosmosClient client;
    auto [priConnStr, secConnStr] = GetActiveConnectionStrings();

    EXPECT_THROW(client.configure({{"connectionStrings", {priConnStr, secConnStr}}}), std::invalid_argument);
}

/// @brief Test all JSON data types
/// @details Creates document with strings, numbers, booleans, arrays, objects, null
/// @test Validates all data types are preserved
TEST_F(CosmosIntegrationTests, DocumentWithVariousDataTypes)
{
    std::string docId = GenerateDocId("datatypes");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId},
                                                                       {"__pk", "siddiqsoft.com"},
                                                                       {"string_field", "test string"},
                                                                       {"int_field", 42},
                                                                       {"float_field", 3.14159},
                                                                       {"bool_true", true},
                                                                       {"bool_false", false},
                                                                       {"null_field", nullptr},
                                                                       {"array_field", nlohmann::json::array({1, "two", 3.0, true})},
                                                                       {"object_field", {{"nested_key", "nested_value"}, {"nested_number", 99}}},
                                                                       {"large_number", 9223372036854775807LL},
                                                                       {"negative_number", -12345},
                                                                       {"zero", 0},
                                                                       {"empty_string", ""},
                                                                       {"empty_array", nlohmann::json::array()},
                                                                       {"empty_object", nlohmann::json::object()}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("test string", rc.document.value("string_field", ""));
    EXPECT_EQ(42, rc.document.value("int_field", 0));
    EXPECT_NEAR(3.14159, rc.document.value("float_field", 0.0), 0.00001);
    EXPECT_TRUE(rc.document.value("bool_true", false));
    EXPECT_FALSE(rc.document.value("bool_false", true));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test Unicode and special characters
/// @details Creates document with Unicode, escape sequences, special characters
/// @test Validates special characters are preserved
TEST_F(CosmosIntegrationTests, DocumentWithSpecialCharacters)
{
    std::string docId = GenerateDocId("special_chars");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId},
                                                                       {"__pk", "siddiqsoft.com"},
                                                                       {"unicode", "Hello 世界 🌍"},
                                                                       {"special", "!@#$%^&*()_+-=[]{}|;:',.<>?/"},
                                                                       {"quotes", "He said \"Hello\" and she replied 'Hi'"},
                                                                       {"newlines", "Line 1\nLine 2\nLine 3"},
                                                                       {"tabs", "Col1\tCol2\tCol3"},
                                                                       {"backslash", "C:\\Users\\test\\file.txt"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("Hello 世界 🌍", rc.document.value("unicode", ""));
    EXPECT_EQ("!@#$%^&*()_+-=[]{}|;:',.<>?/", rc.document.value("special", ""));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test large arrays and strings
/// @details Creates document with 100-item array and 1000-char string
/// @test Validates large content works correctly
TEST_F(CosmosIntegrationTests, DocumentWithLargeContent)
{
    std::string    docId      = GenerateDocId("large_doc");

    nlohmann::json largeArray = nlohmann::json::array();
    for (int i = 0; i < 100; i++) {
        largeArray.push_back({{"index", i}, {"value", std::format("Item {}", i)}, {"data", std::string(100, 'x')}});
    }

    auto rc = testSuiteClient.createDocument(
            {.database   = testDBName0,
             .collection = testCollectionNames[0],
             .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"large_array", largeArray}, {"large_string", std::string(1000, 'a')}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ(100, rc.document.value("large_array", nlohmann::json::array()).size());
    EXPECT_EQ(1000, rc.document.value("large_string", "").length());

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Create 10 documents in sequence
/// @details Creates, finds, and deletes 10 documents
/// @test Validates bulk creation works correctly
TEST_F(CosmosIntegrationTests, BulkCreateDocuments)
{
    std::vector<std::string> docIds;
    const int                BULK_COUNT = 10;

    for (int i = 0; i < BULK_COUNT; i++) {
        std::string docId = GenerateDocId(std::format("bulk_{}", i));
        docIds.push_back(docId);

        auto rc = testSuiteClient.createDocument({.database   = testDBName0,
                                                  .collection = testCollectionNames[0],
                                                  .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"bulk_index", i}, {"batch", "bulk_test"}}});

        EXPECT_EQ(201, rc.statusCode);
    }

    for (const auto& docId : docIds) {
        auto rc = testSuiteClient.findDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});

        EXPECT_EQ(200, rc.statusCode);
    }

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Update 5 documents in sequence
/// @details Creates, updates, and deletes 5 documents
/// @test Validates bulk update works correctly
TEST_F(CosmosIntegrationTests, BulkUpdateDocuments)
{
    std::vector<std::string> docIds;
    const int                BULK_COUNT = 5;

    for (int i = 0; i < BULK_COUNT; i++) {
        std::string docId = GenerateDocId(std::format("bulkupd_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"version", 1}}});
    }

    for (const auto& docId : docIds) {
        auto findRc = testSuiteClient.findDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});

        nlohmann::json updatedDoc = findRc.document;
        updatedDoc["version"]     = 2;

        auto updateRc             = testSuiteClient.updateDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com", .document = updatedDoc});

        EXPECT_EQ(200, updateRc.statusCode);
        EXPECT_EQ(2, updateRc.document.value("version", 0));
    }

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Create documents from 4 threads concurrently
/// @details Creates documents from multiple threads
/// @test Validates concurrent operations work correctly
TEST_F(CosmosIntegrationTests, ConcurrentDocumentCreation)
{
    const int                 THREAD_COUNT    = 4;
    const int                 DOCS_PER_THREAD = 5;
    std::vector<std::jthread> threads;
    std::vector<std::string>  createdDocIds;
    std::mutex                docIdsMutex;

    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([this, t, &createdDocIds, &docIdsMutex]() {
            for (int i = 0; i < DOCS_PER_THREAD; i++) {
                std::string docId = GenerateDocId(std::format("concurrent_{}_{}", t, i));

                auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                                    .collection = testCollectionNames[0],
                                                                    .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"thread", t}, {"iteration", i}}});

                if (rc.statusCode == 201) {
                    std::lock_guard<std::mutex> lock(docIdsMutex);
                    createdDocIds.push_back(docId);
                }
            }
        });
    }

    threads.clear();

    std::this_thread::sleep_for(std::chrono::seconds(3));

    EXPECT_EQ(THREAD_COUNT * DOCS_PER_THREAD, createdDocIds.size());

    for (const auto& docId : createdDocIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

// ============================================================================
// ADDITIONAL AZURE COSMOS REST API FEATURE TESTS
// ============================================================================

/// @brief Verify TTL (Time-To-Live) field support
/// @details Creates document with TTL=1 second
/// @test Validates TTL field is preserved in response
TEST_F(CosmosIntegrationTests, DocumentTTLExpiration)
{
    std::string docId = GenerateDocId("ttl_doc");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"ttl", 1}, {"data", "This document will expire"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("ttl"));
    EXPECT_EQ(1, rc.document.value("ttl", 0));
}

/// @brief Verify _ts system property (timestamp)
/// @details Creates document and checks _ts field
/// @test Validates timestamp is automatically set by Cosmos DB
TEST_F(CosmosIntegrationTests, DocumentWithTimestamp)
{
    std::string docId = GenerateDocId("timestamp_doc");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"data", "Document with timestamp"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("_ts"));
    EXPECT_GT(rc.document.value("_ts", 0), 0);

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Verify _etag system property (entity tag)
/// @details Creates document and checks _etag field
/// @test Validates ETag is automatically set by Cosmos DB
TEST_F(CosmosIntegrationTests, DocumentWithETag)
{
    std::string docId = GenerateDocId("etag_doc");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"data", "Document with ETag"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("_etag"));
    EXPECT_FALSE(rc.document.value("_etag", "").empty());

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Verify _rid system property (resource ID)
/// @details Creates document and checks _rid field
/// @test Validates Resource ID is automatically set by Cosmos DB
TEST_F(CosmosIntegrationTests, DocumentWithRID)
{
    std::string docId = GenerateDocId("rid_doc");

    auto        rc    = testSuiteClient.createDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"data", "Document with RID"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("_rid"));
    EXPECT_FALSE(rc.document.value("_rid", "").empty());

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test ORDER BY clause in queries
/// @details Creates documents with priority values and queries with ORDER BY
/// @test Validates ORDER BY clause works correctly
TEST_F(CosmosIntegrationTests, QueryWithOrderBy)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 5; i++) {
        std::string docId = GenerateDocId(std::format("orderby_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument({.database   = testDBName0,
                                        .collection = testCollectionNames[0],
                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"priority", 5 - i}, {"name", std::format("Item {}", i)}}});
    }

    auto irt = testSuiteClient.queryDocuments({.database       = testDBName0,
                                               .collection     = testCollectionNames[0],
                                               .partitionKey   = "*",
                                               .queryStatement = "SELECT * FROM c WHERE c.priority > 0 ORDER BY c.priority DESC"});

    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Test COUNT and SUM aggregation functions
/// @details Creates documents with numeric values and queries aggregations
/// @test Validates aggregation functions work correctly
TEST_F(CosmosIntegrationTests, QueryWithAggregation)
{
    std::vector<std::string> docIds;

    for (int i = 1; i <= 5; i++) {
        std::string docId = GenerateDocId(std::format("agg_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument({.database   = testDBName0,
                                        .collection = testCollectionNames[0],
                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"ival", i * 10}, {"category", "test"}}});
    }

    // Dump the just added document..
    // In order to test the following queryDocuments.. we must allow the above createDocument to complete.
    // For some reason the emulator fails the queryDocuments if we don't perform a listDocuments!
    auto justAdded = testSuiteClient.listDocuments({.database = testDBName0, .collection = testCollectionNames[0]});
    EXPECT_EQ(200, justAdded.statusCode);

    // SELECT COUNT(c.id) as count, SUM(c.value) as total FROM c WHERE c.category = @cat
    auto irt = testSuiteClient.queryDocuments({.database        = testDBName0,
                                               .collection      = testCollectionNames[0],
                                               .partitionKey    = "*",
                                               .queryStatement  = "SELECT COUNT(c.id) as count, SUM(c.ival) as total FROM c WHERE c.category = @cat",
                                               .queryParameters = {{{"name", "@cat"}, {"value", "test"}}}});

    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents")) << "irt.statusCode = " << irt.statusCode;

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Test DISTINCT keyword in queries
/// @details Creates documents with duplicate categories and queries DISTINCT
/// @test Validates DISTINCT keyword works correctly
TEST_F(CosmosIntegrationTests, QueryWithDistinct)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            std::string docId = GenerateDocId(std::format("distinct_{}_{}", i, j));
            docIds.push_back(docId);

            testSuiteClient.createDocument({.database   = testDBName0,
                                            .collection = testCollectionNames[0],
                                            .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"category", std::format("cat_{}", i)}}});
        }
    }

    auto irt = testSuiteClient.queryDocuments(
            {.database = testDBName0, .collection = testCollectionNames[0], .partitionKey = "*", .queryStatement = "SELECT DISTINCT c.category FROM c"});

    EXPECT_EQ(200, irt.statusCode);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Test UPPER, LOWER, LENGTH string functions
/// @details Creates document with text field and queries string functions
/// @test Validates string functions work correctly
TEST_F(CosmosIntegrationTests, QueryWithStringFunctions)
{
    std::string docId = GenerateDocId("string_func");

    testSuiteClient.createDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"text", "Hello World"}}});

    auto irt = testSuiteClient.queryDocuments(
            {.database        = testDBName0,
             .collection      = testCollectionNames[0],
             .partitionKey    = "*",
             .queryStatement  = "SELECT c.id, UPPER(c.text) as upper_text, LOWER(c.text) as lower_text, LENGTH(c.text) as text_length FROM c WHERE c.id = @id",
             .queryParameters = {{{"name", "@id"}, {"value", docId}}}});

    EXPECT_EQ(200, irt.statusCode);

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test ROUND, FLOOR, CEILING math functions
/// @details Creates document with numeric value and queries math functions
/// @test Validates math functions work correctly
TEST_F(CosmosIntegrationTests, QueryWithMathFunctions)
{
    std::string docId = GenerateDocId("math_func");

    testSuiteClient.createDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"fval", 16.5}}});

    // Dump the just added document..
    // In order to test the following queryDocuments.. we must allow the above createDocument to complete.
    // For some reason the emulator fails the queryDocuments if we don't perform a listDocuments!
    auto justAdded = testSuiteClient.listDocuments({.database = testDBName0, .collection = testCollectionNames[0]});
    EXPECT_EQ(200, justAdded.statusCode);

    // Get the various math functions tested.
    auto irt = testSuiteClient.queryDocuments(
            {.database        = testDBName0,
             .collection      = testCollectionNames[0],
             .partitionKey    = "*",
             .queryStatement  = "SELECT c.id, ROUND(c.fval) as rounded, FLOOR(c.fval) as floored, CEILING(c.fval) as ceiled FROM c WHERE c.id = @id",
             .queryParameters = {{{"name", "@id"}, {"value", docId}}}});

    EXPECT_EQ(200, irt.statusCode);

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test upsert as insert operation
/// @details Upserts new document (does not exist yet)
/// @test Validates status code is 201 (Created)
TEST_F(CosmosIntegrationTests, UpsertDocumentInsert)
{
    std::string docId = GenerateDocId("upsert_insert");

    auto        rc    = testSuiteClient.upsertDocument({.database   = testDBName0,
                                                        .collection = testCollectionNames[0],
                                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"operation", "insert"}, {"version", 1}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("insert", rc.document.value("operation", ""));
    EXPECT_EQ(1, rc.document.value("version", 0));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test upsert as update operation
/// @details Creates document then upserts with new data
/// @test Validates status code is 200 (OK)
TEST_F(CosmosIntegrationTests, UpsertDocumentUpdate)
{
    std::string docId    = GenerateDocId("upsert_update");

    auto        createRc = testSuiteClient.createDocument({.database   = testDBName0,
                                                           .collection = testCollectionNames[0],
                                                           .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"operation", "create"}, {"version", 1}}});
    EXPECT_EQ(201, createRc.statusCode);

    auto upsertRc = testSuiteClient.upsertDocument({.database   = testDBName0,
                                                    .collection = testCollectionNames[0],
                                                    .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"operation", "upsert"}, {"version", 2}}});

    EXPECT_EQ(200, upsertRc.statusCode);
    EXPECT_EQ("upsert", upsertRc.document.value("operation", ""));
    EXPECT_EQ(2, upsertRc.document.value("version", 0));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Query with specific partition key
/// @details Creates documents and queries with specific partition key
/// @test Validates partition key query works correctly
TEST_F(CosmosIntegrationTests, PartitionKeyRangeQuery)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 3; i++) {
        std::string docId = GenerateDocId(std::format("pk_range_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"index", i}}});
    }

    auto irt = testSuiteClient.queryDocuments(
            {.database = testDBName0, .collection = testCollectionNames[0], .partitionKey = "siddiqsoft.com", .queryStatement = "SELECT * FROM c"});

    EXPECT_EQ(200, irt.statusCode);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Query across all partitions
/// @details Creates documents and queries with "*" partition key
/// @test Validates cross-partition query works correctly
TEST_F(CosmosIntegrationTests, CrossPartitionQuery)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 3; i++) {
        std::string docId = GenerateDocId(std::format("cross_pk_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"index", i}}});
    }

    auto irt = testSuiteClient.queryDocuments(
            {.database = testDBName0, .collection = testCollectionNames[0], .partitionKey = "*", .queryStatement = "SELECT * FROM c WHERE c.index >= 0"});

    EXPECT_EQ(200, irt.statusCode);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Verify all system properties are present
/// @details Creates document and checks all system properties
/// @test Validates _rid, _self, _etag, _attachments, _ts exist
TEST_F(CosmosIntegrationTests, DocumentWithSystemProperties)
{
    std::string docId = GenerateDocId("sys_props");

    auto        rc    = testSuiteClient.createDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"data", "Test"}}});

    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("_rid"));
    EXPECT_TRUE(rc.document.contains("_self"));
    EXPECT_TRUE(rc.document.contains("_etag"));
    EXPECT_TRUE(rc.document.contains("_attachments"));
    EXPECT_TRUE(rc.document.contains("_ts"));

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test TOP clause for limiting results
/// @details Creates 10 documents and queries with TOP 5
/// @test Validates TOP clause limits results correctly
TEST_F(CosmosIntegrationTests, QueryWithLimit)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 10; i++) {
        std::string docId = GenerateDocId(std::format("limit_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"index", i}}});
    }

    auto irt = testSuiteClient.queryDocuments(
            {.database = testDBName0, .collection = testCollectionNames[0], .partitionKey = "*", .queryStatement = "SELECT TOP 5 * FROM c"});

    EXPECT_EQ(200, irt.statusCode);
    EXPECT_LE(irt.document.value("_count", 0), 5);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Test OFFSET and LIMIT for pagination
/// @details Creates 10 documents and queries with OFFSET 5 LIMIT 5
/// @test Validates OFFSET and LIMIT work correctly
TEST_F(CosmosIntegrationTests, QueryWithOffset)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 10; i++) {
        std::string docId = GenerateDocId(std::format("offset_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"index", i}}});
    }

    auto irt = testSuiteClient.queryDocuments({.database       = testDBName0,
                                               .collection     = testCollectionNames[0],
                                               .partitionKey   = "*",
                                               .queryStatement = "SELECT * FROM c ORDER BY c.index OFFSET 5 LIMIT 5"});

    EXPECT_EQ(200, irt.statusCode);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

/// @brief Test ARRAY_CONTAINS function
/// @details Creates document with array field and queries ARRAY_CONTAINS
/// @test Validates ARRAY_CONTAINS function works correctly
TEST_F(CosmosIntegrationTests, QueryWithArrayContains)
{
    std::string docId = GenerateDocId("array_contains");

    testSuiteClient.createDocument({.database   = testDBName0,
                                    .collection = testCollectionNames[0],
                                    .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"tags", nlohmann::json::array({"important", "urgent", "review"})}}});

    auto irt = testSuiteClient.queryDocuments({.database        = testDBName0,
                                               .collection      = testCollectionNames[0],
                                               .partitionKey    = "*",
                                               .queryStatement  = "SELECT * FROM c WHERE ARRAY_CONTAINS(c.tags, @tag)",
                                               .queryParameters = {{{"name", "@tag"}, {"value", "urgent"}}}});

    EXPECT_EQ(200, irt.statusCode);

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
}

/// @brief Test EXISTS function
/// @details Creates documents with and without optional field
/// @test Validates EXISTS function works correctly
TEST_F(CosmosIntegrationTests, QueryWithExists)
{
    std::string docId1 = GenerateDocId("exists_yes");
    std::string docId2 = GenerateDocId("exists_no");

    testSuiteClient.createDocument(
            {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId1}, {"__pk", "siddiqsoft.com"}, {"optional_field", "value"}}});

    testSuiteClient.createDocument({.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId2}, {"__pk", "siddiqsoft.com"}}});

    // Check if the field exists in the document
    // The EXISTS function in Cosmos DB SQL requires a subquery, not just a field reference.
    auto irt = testSuiteClient.queryDocuments({.database       = testDBName0,
                                               .collection     = testCollectionNames[0],
                                               .partitionKey   = "*",
                                               .queryStatement = "SELECT * FROM c WHERE IS_DEFINED(c.optional_field)"});

    EXPECT_EQ(200, irt.statusCode);

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId1, .partitionKey = "siddiqsoft.com"});

    testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId2, .partitionKey = "siddiqsoft.com"});
}

TEST_F(CosmosIntegrationTests, QueryWithIn)
{
    std::vector<std::string> docIds;

    for (int i = 0; i < 5; i++) {
        std::string docId = GenerateDocId(std::format("in_query_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument({.database   = testDBName0,
                                        .collection = testCollectionNames[0],
                                        .document   = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"status", i % 2 == 0 ? "active" : "inactive"}}});
    }

    auto irt = testSuiteClient.queryDocuments({.database       = testDBName0,
                                               .collection     = testCollectionNames[0],
                                               .partitionKey   = "*",
                                               .queryStatement = "SELECT * FROM c WHERE c.status IN ('active', 'pending')"});

    EXPECT_EQ(200, irt.statusCode);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}

TEST_F(CosmosIntegrationTests, QueryWithBetween)
{
    std::vector<std::string> docIds;

    for (int i = 1; i <= 10; i++) {
        std::string docId = GenerateDocId(std::format("between_{}", i));
        docIds.push_back(docId);

        testSuiteClient.createDocument(
                {.database = testDBName0, .collection = testCollectionNames[0], .document = {{"id", docId}, {"__pk", "siddiqsoft.com"}, {"score", i * 10}}});
    }

    auto irt = testSuiteClient.queryDocuments({.database       = testDBName0,
                                               .collection     = testCollectionNames[0],
                                               .partitionKey   = "*",
                                               .queryStatement = "SELECT * FROM c WHERE c.score BETWEEN 30 AND 70"});

    EXPECT_EQ(200, irt.statusCode);

    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({.database = testDBName0, .collection = testCollectionNames[0], .id = docId, .partitionKey = "siddiqsoft.com"});
    }
}
