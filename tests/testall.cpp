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
        if (!IsCosmosReachable()) {
            std::print(std::cerr, "SetUpTestCase: Cosmos service is not reachable, skipping setup\n");
            return;
        }

        std::print(std::cerr, "SetUpTestCase: Configuring test suite client...\n");
        testSuiteClient.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetActiveConnectionStrings()}});

        // Clean up any existing test database from previous runs
        std::print(std::cerr, "SetUpTestCase: Cleaning up existing database '{}' if it exists...\n", testDBName0);
        auto deleteRc = TSdeleteDatabase(testDBName0);
        if (deleteRc.statusCode == 204 || deleteRc.statusCode == 404) {
            std::print(std::cerr, "SetUpTestCase: Database cleanup completed (status: {})\n", deleteRc.statusCode);
        }

        // Wait a moment for deletion to propagate
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Create test database
        std::print(std::cerr, "SetUpTestCase: Creating test database '{}'\n", testDBName0);
        auto createDbRc = TScreateDatabase(testDBName0);
        if (createDbRc.statusCode != 201) {
            std::print(std::cerr, "SetUpTestCase: ERROR - Failed to create database '{}' (status: {})\n", testDBName0, createDbRc.statusCode);
            throw std::runtime_error(std::format("Failed to create test database '{}' with status code {}", testDBName0, createDbRc.statusCode));
        }
        std::print(std::cerr, "SetUpTestCase: Database '{}' created successfully\n", testDBName0);

        // Create test collections
        std::print(std::cerr, "SetUpTestCase: Creating {} test collections...\n", testCollectionNames.size());
        for (size_t idx = 0; idx < testCollectionNames.size(); ++idx) {
            auto& collName = testCollectionNames[idx];
            std::print(std::cerr, "SetUpTestCase: Creating collection '{}' in database '{}'\n", collName, testDBName0);
            
            auto createCollRc = TScreateCollection(testDBName0, collName);
            if (createCollRc.statusCode != 201) {
                std::print(std::cerr, "SetUpTestCase: ERROR - Failed to create collection '{}' (status: {})\n", collName, createCollRc.statusCode);
                throw std::runtime_error(std::format("Failed to create test collection '{}' with status code {}", collName, createCollRc.statusCode));
            }
            std::print(std::cerr, "SetUpTestCase: Collection '{}' created successfully\n", collName);

            // Seed with test documents
            std::print(std::cerr, "SetUpTestCase: Seeding collection '{}' with {} documents...\n", collName, SEED_DOCUMENT_COUNT);
            for (auto i = 0; i < SEED_DOCUMENT_COUNT; i++) {
                auto seedRc = testSuiteClient.createDocument({
                    .database   = testDBName0,
                    .collection = collName,
                    .document   = {
                        {"id", std::format("{:0X}.{}", i, (i % 2) == 0 ? "even" : "odd")},
                        {"ttl", 1360},
                        {"__pk", "siddiqsoft.com"},
                        {"func", __func__},
                        {"extra", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" : "odd")},
                        {"source", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" : "odd")}
                    }
                });
                if (seedRc.statusCode != 201) {
                    std::print(std::cerr, "SetUpTestCase: Warning - Failed to seed document {} in collection '{}' (status: {})\n", i, collName, seedRc.statusCode);
                }
            }
            std::print(std::cerr, "SetUpTestCase: Collection '{}' seeding completed\n", collName);
        }
        std::print(std::cerr, "SetUpTestCase: Test suite setup completed successfully\n");
    }

    static void TearDownTestCase()
    {
        std::print(std::cerr, "TearDownTestCase: Cleaning up test database '{}'\n", testDBName0);
        auto deleteRc = TSdeleteDatabase(testDBName0);
        if (deleteRc.statusCode == 204 || deleteRc.statusCode == 404) {
            std::print(std::cerr, "TearDownTestCase: Database cleanup completed (status: {})\n", deleteRc.statusCode);
        }
    }

    // Helper to generate unique document IDs
    static std::string GenerateDocId(const std::string& prefix = "doc")
    {
        return std::format("{}_{}", prefix, std::chrono::system_clock().now().time_since_epoch().count());
    }

    // Helper to create test document
    static nlohmann::json CreateTestDocument(const std::string& id, const std::string& pk = "siddiqsoft.com")
    {
        return {
            {"id", id},
            {"__pk", pk},
            {"name", std::format("Document {}", id)},
            {"description", "Test document for comprehensive testing"},
            {"created", std::chrono::system_clock::now().time_since_epoch().count()},
            {"tags", nlohmann::json::array({"test", "comprehensive"})},
            {"metadata", {{"version", 1}, {"author", "test_suite"}}}
        };
    }
};

// ============================================================================
// VALIDATION TESTS (No Emulator Required)
// ============================================================================

TEST(Validation, checkEmulatorInfo)
{
    auto [priConnStr, secConnStr] = GetConnectionStrings();
    ASSERT_FALSE(priConnStr.empty()) << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";
    ASSERT_FALSE(secConnStr.empty()) << "Missing environment variable CCTEST_SECONDARY_CS; Set it to Secondary Connection string from Azure portal.";
}

TEST(Validation, configure_Defaults)
{
    siddiqsoft::CosmosClient cc;
    auto& currentConfig = cc.configuration();

    EXPECT_TRUE(currentConfig.contains("apiVersion"));
    EXPECT_EQ("2018-12-31", currentConfig.value("apiVersion", ""));
    EXPECT_TRUE(currentConfig.contains("connectionStrings"));
    EXPECT_TRUE(currentConfig.contains("partitionKeyNames"));
}

TEST(Validation, configure_check_json)
{
    siddiqsoft::CosmosClient cc;
    nlohmann::json info = cc;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(5, info.size()) << info.dump(3);
}

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
    std::print(std::cerr, "{} - ....rc:-\n{}\n", __func__, rc.document.dump(4));

    EXPECT_EQ(200, rc.statusCode) << rc.document.dump(3);
    EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
    EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
}

TEST(Validation, discoverRegions_BadPrimary)
{
    if (!IsCosmosReachable()) GTEST_SKIP() << "Cosmos service is not reachable";

    auto [_, secConnStr] = GetActiveConnectionStrings();
    auto priConnStr = std::string_view("AccountEndpoint=https://localhost:4043/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;");

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

TEST(CosmosConnection, test1_n)
{
    std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {cs};

    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;", std::string(cd.Primary));
    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cd.Primary.BaseUri));
    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cd.Primary.EncodedKey);
    EXPECT_EQ("yourdbname.documents.azure.com", ::siddiqsoft::Uri<char> {cd.Primary.BaseUri}.authority.host);
}

TEST(CosmosConnection, test2_n)
{
    std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {cs};

    nlohmann::json info = cd;
    EXPECT_EQ(4, info.size());
}

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

TEST_F(CosmosIntegrationTests, CreateDatabase)
{
    auto rc1 = TScreateDatabase(testDBName);
    EXPECT_EQ(201, rc1.statusCode);

    auto rc2 = TSfindDatabase(testDBName);
    EXPECT_EQ(200, rc2.statusCode);

    auto rc3 = TSdeleteDatabase(testDBName);
    EXPECT_EQ(204, rc3.statusCode);
}

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

TEST_F(CosmosIntegrationTests, Example)
{
    if (auto rc = testSuiteClient.listDatabases(); 200 == rc.statusCode) {
        auto dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

        if (auto rc2 = testSuiteClient.listCollections({.database = dbName}); 200 == rc2.statusCode) {
            auto collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");
            auto id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
            auto pkId = "siddiqsoft.com";

            if (auto rc3 = testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"func", __func__}, {"source", "basic_tests.exe"}}}); 201 == rc3.statusCode) {
                auto rc4 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = rc3.document.value("id", id), .partitionKey = pkId});
                EXPECT_EQ(204, rc4);
            }
        }
    }
}

TEST_F(CosmosIntegrationTests, ListDatabases)
{
    EXPECT_NO_THROW({
        auto rc = testSuiteClient.listDatabases();
        EXPECT_TRUE(rc.document.contains("Databases"));
        EXPECT_TRUE(rc.document["Databases"].is_array());
        EXPECT_EQ(200, rc.statusCode);
    });
}

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

TEST_F(CosmosIntegrationTests, ListDocuments)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t totalDocs = 0;
    auto iteration = 7;

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_NE("", rc.document.value("/Databases/0/id"_json_pointer, ""));

    auto rc2 = testSuiteClient.listCollections({.database = rc.document.value("/Databases/0/id"_json_pointer, "")});
    EXPECT_EQ(200, rc2.statusCode);
    EXPECT_NE("", rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""));

    do {
        irt = testSuiteClient.listDocuments({.database = rc.document.value("/Databases/0/id"_json_pointer, ""), .collection = rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""), .continuationToken = irt.continuationToken});
        EXPECT_EQ(200, irt.statusCode);
        totalDocs += irt.document.value<uint32_t>("_count", 0);
        EXPECT_LE(1, irt.document.value("_count", 0));

        if (--iteration == 0) break;
    } while (!irt.continuationToken.empty());
}

TEST_F(CosmosIntegrationTests, ListDocuments_top8)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t totalDocs = 0;

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);

    auto rc2 = testSuiteClient.listCollections({.database = rc.document.value("/Databases/0/id"_json_pointer, "")});
    EXPECT_EQ(200, rc2.statusCode);

    irt = testSuiteClient.listDocuments({.database = rc.document.value("/Databases/0/id"_json_pointer, ""), .collection = rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""), .continuationToken = irt.continuationToken});
    EXPECT_EQ(200, irt.statusCode);
    totalDocs += irt.document.value<uint32_t>("_count", 0);
    EXPECT_GE(10, irt.document.value("_count", 0));
}

TEST_F(CosmosIntegrationTests, CreateDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");
    EXPECT_FALSE(dbName.empty());

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");
    EXPECT_FALSE(collectionName.empty());

    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    auto rc3 = testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc3.statusCode);

    auto rc4 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc4);
}

TEST_F(CosmosIntegrationTests, CreateDocument_MissingId)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    EXPECT_THROW(testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"MissingId", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}}}), std::invalid_argument);
}

TEST_F(CosmosIntegrationTests, CreateDocument_MissingPkId)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    EXPECT_THROW(testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"Missing__pk", pkId}, {"source", "basic_tests.exe"}}}), std::invalid_argument);
}

TEST_F(CosmosIntegrationTests, FindDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    auto rc3 = testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc3.statusCode);

    auto rc4 = testSuiteClient.findDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(200, rc4.statusCode);
    EXPECT_EQ(id, rc4.document.value("id", ""));

    auto rc5 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc5);
}

TEST_F(CosmosIntegrationTests, UpsertDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    auto rc4 = testSuiteClient.upsertDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "insert"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(201, rc4.statusCode);
    EXPECT_EQ("insert", rc4.document.value("upsert", ""));

    auto rc5 = testSuiteClient.upsertDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "update"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(200, rc5.statusCode);
    EXPECT_EQ("update", rc5.document.value("upsert", ""));

    auto rc6 = testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "FAIL"}, {"source", "basic_tests.exe"}}});
    EXPECT_EQ(409, rc6.statusCode);

    auto rc7 = testSuiteClient.removeDocument({.database = dbName, .collection = collectionName, .id = id, .partitionKey = pkId});
    EXPECT_EQ(204, rc7);
}

TEST_F(CosmosIntegrationTests, UpdateDocument)
{
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    auto rc4 = testSuiteClient.createDocument({.database = dbName, .collection = collectionName, .document = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"mode", "create"}, {"source", "basic_tests.exe"}}});
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

TEST_F(CosmosIntegrationTests, QueryDocument_odd)
{
    std::string dbName {};
    std::string collectionName {};
    std::vector<std::string> docIds {};
    std::string pkId {"siddiqsoft.com"};
    siddiqsoft::CosmosIterableResponseType irt {};
    nlohmann::json allDocs = nlohmann::json::array();
    uint32_t allDocsCount {};

    std::println(std::cerr, "{} -- Query documents in {}:{}...", __func__, testDBName0, testCollectionNames[1]);

    do {
        irt = testSuiteClient.queryDocuments({.database = testDBName0, .collection = testCollectionNames[1], .partitionKey = "*", .continuationToken = irt.continuationToken, .queryStatement = "SELECT * FROM c WHERE contains(c.source, @v1)", .queryParameters = {{{"name", "@v1"}, {"value", "odd"}}}});
        ASSERT_EQ(200, irt.statusCode);
        if (200 == irt.statusCode && irt.document.contains("Documents") && !irt.document.at("Documents").is_null()) {
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
            allDocsCount += irt.document.value("_count", 0);
        }
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(5, allDocsCount);
}

TEST_F(CosmosIntegrationTests, QueryDocument_even)
{
    std::string dbName {};
    std::string collectionName {};
    std::vector<std::string> docIds {};
    std::string pkId {"siddiqsoft.com"};
    siddiqsoft::CosmosIterableResponseType irt {};
    nlohmann::json allDocs = nlohmann::json::array();
    uint32_t allDocsCount {};

    std::println(std::cerr, "{} -- Query documents in {}:{}...", __func__, testDBName0, testCollectionNames[1]);

    do {
        irt = testSuiteClient.queryDocuments({.database = testDBName0, .collection = testCollectionNames[1], .partitionKey = "*", .continuationToken = irt.continuationToken, .queryStatement = "SELECT * FROM c WHERE contains(c.source, @v1)", .queryParameters = {{{"name", "@v1"}, {"value", "even"}}}});
        ASSERT_EQ(200, irt.statusCode);
        if (200 == irt.statusCode && irt.document.contains("Documents") && !irt.document.at("Documents").is_null()) {
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
            allDocsCount += irt.document.value("_count", 0);
        }
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(5, allDocsCount);
}

TEST_F(CosmosIntegrationTests, MoveConstruct)
{
    std::vector<siddiqsoft::CosmosClient> clients;

    clients.push_back(siddiqsoft::CosmosClient {});
    clients.push_back(siddiqsoft::CosmosClient {});

    EXPECT_EQ(2, clients.size());
}

TEST_F(CosmosIntegrationTests, ConfigureMulti)
{
    std::vector<siddiqsoft::CosmosClient> clients;

    std::cerr << "Setting up the clients..\n";
    for (auto i = 0; i < 4; i++) {
        clients.emplace_back(siddiqsoft::CosmosClient {})
                .configure(nlohmann::json {{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetActiveConnectionStrings()}});
    }

    EXPECT_EQ(4, clients.size());

    std::atomic_uint passTest {0};

    std::cerr << "Setting up the clients..configuring..\n";
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

    std::cerr << "Completed.\n";

    EXPECT_EQ(4, passTest.load());
}

TEST_F(CosmosIntegrationTests, CreateDocumentThreaded)
{
    auto ttx = std::chrono::system_clock::now();
    std::string dbName {};
    std::string collectionName {};
    std::string pkId {"siddiqsoft.com"};
    std::string sourceId = std::format("{}-{}", getpid(), siddiqsoft::CosmosClient::CosmosClientUserAgentString);
    constexpr auto DOCS {15};
    static auto threadCount = std::thread::hardware_concurrency();
    std::latch startLatch {threadCount};
    std::atomic_uint32_t removeDocsCount {0}, addDocsCount {0};
    std::latch endLatch {threadCount};
    std::barrier creatorsBarrier(threadCount, [&]() noexcept -> void {
#if defined(DEBUG)
        std::cerr << std::format("!! Barrier hit. DOCS:{} x threadCount:{} -> addDocsCount:{} removeDocsCount:{} ttx:{}!!\n",
                                 DOCS,
                                 threadCount,
                                 addDocsCount.load(),
                                 removeDocsCount.load(),
                                 std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
#endif
    });

    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = testSuiteClient.listCollections({.database = dbName});
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    ttx = std::chrono::system_clock::now();
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
                                    {.database = dbName,
                                     .collection = collectionName,
                                     .document = {{"id", std::format("{}.{}.{}", tid, i, std::chrono::system_clock::now().time_since_epoch().count())},
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
