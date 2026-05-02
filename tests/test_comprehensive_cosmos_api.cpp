/*
    CosmosClient - Comprehensive Azure Cosmos API Test Suite
    Extensive testing of the Azure Cosmos REST-API Client implementation

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

#include "gtest/gtest.h"

#include <thread>
#include <chrono>
#include <format>
#include <print>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/cosmoscl.hpp"

#include "test_common.hpp"

// ============================================================================
// Comprehensive Azure Cosmos API Test Suite
// ============================================================================

/// @brief Comprehensive test fixture for Azure Cosmos API testing
class ComprehensiveCosmosAPITests : public ::testing::Test
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
        } else {
            std::print(std::cerr, "SetUpTestCase: Warning - unexpected status code {} when deleting database\n", deleteRc.statusCode);
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

            // Seed with diverse test documents
            std::print(std::cerr, "SetUpTestCase: Seeding collection '{}' with {} documents...\n", collName, SEED_DOCUMENT_COUNT);
            for (auto i = 0; i < SEED_DOCUMENT_COUNT; i++) {
                auto seedRc = testSuiteClient.createDocument({
                    .database   = testDBName0,
                    .collection = collName,
                    .document   = {
                        {"id", std::format("seed_{:02d}", i)},
                        {"__pk", "siddiqsoft.com"},
                        {"index", i},
                        {"parity", (i % 2) == 0 ? "even" : "odd"},
                        {"category", std::format("cat_{}", i % 3)},
                        {"value", i * 10},
                        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                        {"active", i < 5}
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
        } else {
            std::print(std::cerr, "TearDownTestCase: Warning - unexpected status code {} when deleting database\n", deleteRc.statusCode);
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
// SECTION 1: Connection and Configuration Tests
// ============================================================================

TEST(ComprehensiveConnectionTests, InvalidConnectionStringFormat)
{
    std::string cs = "InvalidFormat";
    siddiqsoft::CosmosEndpoint endpoint(cs);
    
    EXPECT_FALSE(static_cast<bool>(endpoint));
}

TEST_F(ComprehensiveCosmosAPITests, ClientDefaultConfiguration)
{
    siddiqsoft::CosmosClient client;
    auto& config = client.configuration();
    
    EXPECT_TRUE(config.contains("apiVersion"));
    EXPECT_EQ("2018-12-31", config.value("apiVersion", ""));
    EXPECT_TRUE(config.contains("libRetryLimit"));
    EXPECT_EQ(7, config.value("libRetryLimit", 0));
    EXPECT_TRUE(config.contains("connectionStrings"));
    EXPECT_TRUE(config.contains("partitionKeyNames"));
}


// ============================================================================
// SECTION 2: Database Operations Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, CreateDatabaseBasic)
{
    std::string dbName = GenerateDocId("testdb");
    
    auto rc = testSuiteClient.createDatabase({.database = dbName});
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("id"));
    EXPECT_EQ(dbName, rc.document.value("id", ""));
    EXPECT_TRUE(rc.document.contains("_rid"));
    EXPECT_TRUE(rc.document.contains("_self"));
    EXPECT_TRUE(rc.document.contains("_etag"));
    EXPECT_TRUE(rc.document.contains("_ts"));
    
    // Cleanup
    testSuiteClient.deleteDatabase({.database = dbName});
}

TEST_F(ComprehensiveCosmosAPITests, CreateDatabaseDuplicate)
{
    std::string dbName = GenerateDocId("dupdb");
    
    // Create first time
    auto rc1 = testSuiteClient.createDatabase({.database = dbName});
    EXPECT_EQ(201, rc1.statusCode);
    
    // Try to create again - should fail with 409 Conflict
    auto rc2 = testSuiteClient.createDatabase({.database = dbName});
    EXPECT_EQ(409, rc2.statusCode);
    
    // Cleanup
    testSuiteClient.deleteDatabase({.database = dbName});
}

TEST_F(ComprehensiveCosmosAPITests, ListDatabasesNotEmpty)
{
    auto rc = testSuiteClient.listDatabases();
    
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("Databases"));
    EXPECT_TRUE(rc.document["Databases"].is_array());
    EXPECT_GE(rc.document["Databases"].size(), 1);
    
    // Verify structure of first database
    auto& firstDb = rc.document["Databases"][0];
    EXPECT_TRUE(firstDb.contains("id"));
    EXPECT_TRUE(firstDb.contains("_rid"));
    EXPECT_TRUE(firstDb.contains("_self"));
    EXPECT_TRUE(firstDb.contains("_etag"));
    EXPECT_TRUE(firstDb.contains("_ts"));
}

TEST_F(ComprehensiveCosmosAPITests, FindDatabaseExists)
{
    auto rc = testSuiteClient.findDatabase({.database = testDBName0});
    
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("id"));
    EXPECT_EQ(testDBName0, rc.document.value("id", ""));
}

TEST_F(ComprehensiveCosmosAPITests, FindDatabaseNotFound)
{
    std::string nonexistentDb = "nonexistent_db_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    auto rc = testSuiteClient.findDatabase({.database = nonexistentDb});
    
    EXPECT_EQ(404, rc.statusCode);
}

TEST_F(ComprehensiveCosmosAPITests, DeleteDatabaseSuccess)
{
    std::string dbName = GenerateDocId("deldb");
    
    // Create database
    auto createRc = testSuiteClient.createDatabase({.database = dbName});
    EXPECT_EQ(201, createRc.statusCode);
    
    // Delete database
    auto deleteRc = testSuiteClient.deleteDatabase({.database = dbName});
    EXPECT_EQ(204, deleteRc.statusCode);
    
    // Verify deletion
    auto findRc = testSuiteClient.findDatabase({.database = dbName});
    EXPECT_EQ(404, findRc.statusCode);
}

TEST_F(ComprehensiveCosmosAPITests, DeleteNonexistentDatabase)
{
    std::string nonexistentDb = "nonexistent_db_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    auto rc = testSuiteClient.deleteDatabase({.database = nonexistentDb});
    
    EXPECT_EQ(404, rc.statusCode);
}

// ============================================================================
// SECTION 3: Collection Operations Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, CreateCollectionBasic)
{
    std::string collName = GenerateDocId("testcoll");
    
    auto rc = testSuiteClient.createCollection({
        .database   = testDBName0,
        .collection = collName
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("id"));
    EXPECT_EQ(collName, rc.document.value("id", ""));
    EXPECT_TRUE(rc.document.contains("partitionKey"));
    EXPECT_TRUE(rc.document["partitionKey"].contains("paths"));
}

TEST_F(ComprehensiveCosmosAPITests, CreateCollectionDuplicate)
{
    std::string collName = GenerateDocId("dupcoll");
    
    // Create first time
    auto rc1 = testSuiteClient.createCollection({
        .database   = testDBName0,
        .collection = collName
    });
    EXPECT_EQ(201, rc1.statusCode);
    
    // Try to create again - should fail with 409 Conflict
    auto rc2 = testSuiteClient.createCollection({
        .database   = testDBName0,
        .collection = collName
    });
    EXPECT_EQ(409, rc2.statusCode);
}

TEST_F(ComprehensiveCosmosAPITests, ListCollectionsNotEmpty)
{
    auto rc = testSuiteClient.listCollections({.database = testDBName0});
    
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("DocumentCollections"));
    EXPECT_TRUE(rc.document["DocumentCollections"].is_array());
    EXPECT_GE(rc.document["DocumentCollections"].size(), 1);
    
    // Verify structure of first collection
    auto& firstColl = rc.document["DocumentCollections"][0];
    EXPECT_TRUE(firstColl.contains("id"));
    EXPECT_TRUE(firstColl.contains("_rid"));
    EXPECT_TRUE(firstColl.contains("partitionKey"));
}

TEST_F(ComprehensiveCosmosAPITests, ListCollectionsNonexistentDatabase)
{
    std::string nonexistentDb = "nonexistent_db_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    auto rc = testSuiteClient.listCollections({.database = nonexistentDb});
    
    EXPECT_EQ(404, rc.statusCode);
}

// ============================================================================
// SECTION 4: Document Creation Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, CreateDocumentMinimal)
{
    std::string docId = GenerateDocId("mindoc");
    
    auto rc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = {
            {"id", docId},
            {"__pk", "siddiqsoft.com"}
        }
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ(docId, rc.document.value("id", ""));
    EXPECT_EQ("siddiqsoft.com", rc.document.value("__pk", ""));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

TEST_F(ComprehensiveCosmosAPITests, CreateDocumentWithComplexStructure)
{
    std::string docId = GenerateDocId("complexdoc");
    
    auto rc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = {
            {"id", docId},
            {"__pk", "siddiqsoft.com"},
            {"name", "Complex Document"},
            {"nested", {
                {"level1", {
                    {"level2", {
                        {"value", "deep"}
                    }}
                }}
            }},
            {"array", nlohmann::json::array({1, 2, 3, "four"})},
            {"boolean", true},
            {"number", 42.5},
            {"null_value", nullptr}
        }
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("Complex Document", rc.document.value("name", ""));
    EXPECT_EQ("deep", rc.document.value("/nested/level1/level2/value"_json_pointer, ""));
    EXPECT_EQ("four", rc.document.value("/array/3"_json_pointer, ""));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

TEST_F(ComprehensiveCosmosAPITests, CreateDocumentMissingId)
{
    EXPECT_THROW(
        testSuiteClient.createDocument({
            .database   = testDBName0,
            .collection = testCollectionNames[0],
            .document   = {
                {"__pk", "siddiqsoft.com"},
                {"name", "No ID"}
            }
        }),
        std::invalid_argument
    );
}

TEST_F(ComprehensiveCosmosAPITests, CreateDocumentMissingPartitionKey)
{
    std::string docId = GenerateDocId("nopkdoc");
    
    EXPECT_THROW(
        testSuiteClient.createDocument({
            .database   = testDBName0,
            .collection = testCollectionNames[0],
            .document   = {
                {"id", docId},
                {"name", "No Partition Key"}
            }
        }),
        std::invalid_argument
    );
}

TEST_F(ComprehensiveCosmosAPITests, CreateDocumentDuplicate)
{
    std::string docId = GenerateDocId("dupdoc");
    
    // Create first time
    auto rc1 = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, rc1.statusCode);
    
    // Try to create again - should fail with 409 Conflict
    auto rc2 = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(409, rc2.statusCode);
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

// ============================================================================
// SECTION 5: Document Read Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, FindDocumentExists)
{
    std::string docId = GenerateDocId("finddoc");
    
    // Create document
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Find document
    auto findRc = testSuiteClient.findDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
    
    EXPECT_EQ(200, findRc.statusCode);
    EXPECT_EQ(docId, findRc.document.value("id", ""));
    EXPECT_EQ("siddiqsoft.com", findRc.document.value("__pk", ""));
    EXPECT_EQ(std::format("Document {}", docId), findRc.document.value("name", ""));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

TEST_F(ComprehensiveCosmosAPITests, FindDocumentNotFound)
{
    auto rc = testSuiteClient.findDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = "nonexistent_doc_12345",
        .partitionKey = "siddiqsoft.com"
    });
    
    EXPECT_EQ(404, rc.statusCode);
}

TEST_F(ComprehensiveCosmosAPITests, FindDocumentWrongPartitionKey)
{
    std::string docId = GenerateDocId("wrongpkdoc");
    
    // Create document
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Try to find with wrong partition key
    auto findRc = testSuiteClient.findDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "wrong.partition.key"
    });
    
    EXPECT_EQ(404, findRc.statusCode);
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

// ============================================================================
// SECTION 6: Document Update Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, UpdateDocumentBasic)
{
    std::string docId = GenerateDocId("updatedoc");
    
    // Create document
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Update document
    nlohmann::json updatedDoc = createRc.document;
    updatedDoc["name"] = "Updated Name";
    updatedDoc["metadata"]["version"] = 2;
    
    auto updateRc = testSuiteClient.updateDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com",
        .document     = updatedDoc
    });
    
    EXPECT_EQ(200, updateRc.statusCode);
    EXPECT_EQ("Updated Name", updateRc.document.value("name", ""));
    EXPECT_EQ(2, updateRc.document.value("/metadata/version"_json_pointer, 0));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

TEST_F(ComprehensiveCosmosAPITests, UpdateDocumentAddFields)
{
    std::string docId = GenerateDocId("addfieldsdoc");
    
    // Create document
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Update with new fields
    nlohmann::json updatedDoc = createRc.document;
    updatedDoc["newField1"] = "new value";
    updatedDoc["newField2"] = 999;
    updatedDoc["newField3"] = nlohmann::json::array({"a", "b", "c"});
    
    auto updateRc = testSuiteClient.updateDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com",
        .document     = updatedDoc
    });
    
    EXPECT_EQ(200, updateRc.statusCode);
    EXPECT_EQ("new value", updateRc.document.value("newField1", ""));
    EXPECT_EQ(999, updateRc.document.value("newField2", 0));
    EXPECT_EQ(3, updateRc.document.value("newField3", nlohmann::json::array()).size());
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

TEST_F(ComprehensiveCosmosAPITests, UpdateDocumentRemoveFields)
{
    std::string docId = GenerateDocId("removefieldsdoc");
    
    // Create document with extra fields
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = {
            {"id", docId},
            {"__pk", "siddiqsoft.com"},
            {"field1", "value1"},
            {"field2", "value2"},
            {"field3", "value3"}
        }
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Update by removing field2
    nlohmann::json updatedDoc = createRc.document;
    updatedDoc.erase("field2");
    
    auto updateRc = testSuiteClient.updateDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com",
        .document     = updatedDoc
    });
    
    EXPECT_EQ(200, updateRc.statusCode);
    EXPECT_FALSE(updateRc.document.contains("field2"));
    EXPECT_TRUE(updateRc.document.contains("field1"));
    EXPECT_TRUE(updateRc.document.contains("field3"));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

TEST_F(ComprehensiveCosmosAPITests, UpdateNonexistentDocument)
{
    std::string docId = GenerateDocId("nonexistentupdate");
    
    auto rc = testSuiteClient.updateDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com",
        .document     = CreateTestDocument(docId)
    });
    
    EXPECT_EQ(404, rc.statusCode);
}

// ============================================================================
// SECTION 7: Document Upsert Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, UpsertDocumentInsert)
{
    std::string docId = GenerateDocId("upsertinsert");
    
    auto rc = testSuiteClient.upsertDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ(docId, rc.document.value("id", ""));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}


// ============================================================================
// SECTION 8: Document Delete Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, RemoveDocumentSuccess)
{
    std::string docId = GenerateDocId("removedoc");
    
    // Create document
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Remove document
    auto removeRc = testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
    
    EXPECT_EQ(204, removeRc);
    
    // Verify deletion
    auto findRc = testSuiteClient.findDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
    
    EXPECT_EQ(404, findRc.statusCode);
}

TEST_F(ComprehensiveCosmosAPITests, RemoveNonexistentDocument)
{
    auto rc = testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = "nonexistent_doc_12345",
        .partitionKey = "siddiqsoft.com"
    });
    
    EXPECT_EQ(404, rc);
}

TEST_F(ComprehensiveCosmosAPITests, RemoveDocumentWrongPartitionKey)
{
    std::string docId = GenerateDocId("wrongpkremove");
    
    // Create document
    auto createRc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = CreateTestDocument(docId)
    });
    EXPECT_EQ(201, createRc.statusCode);
    
    // Try to remove with wrong partition key
    auto removeRc = testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "wrong.partition.key"
    });
    
    EXPECT_EQ(404, removeRc);
    
    // Cleanup with correct partition key
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

// ============================================================================
// SECTION 9: Query Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, QueryDocumentsSimple)
{
    auto irt = testSuiteClient.queryDocuments({
        .database       = testDBName0,
        .collection     = testCollectionNames[0],
        .partitionKey   = "*",
        .queryStatement = "SELECT * FROM c WHERE c.parity = @parity",
        .queryParameters = {{{"name", "@parity"}, {"value", "even"}}}
    });
    
    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));
    EXPECT_GE(irt.document.value("_count", 0), 1);
}

TEST_F(ComprehensiveCosmosAPITests, QueryDocumentsWithPagination)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t totalDocs = 0;
    int iterations = 0;
    
    do {
        irt = testSuiteClient.queryDocuments({
            .database          = testDBName0,
            .collection        = testCollectionNames[0],
            .partitionKey      = "*",
            .continuationToken = irt.continuationToken,
            .queryStatement    = "SELECT * FROM c"
        });
        
        EXPECT_EQ(200, irt.statusCode);
        if (irt.statusCode == 200 && irt.document.contains("Documents")) {
            totalDocs += irt.document.value("_count", 0);
        }
        
        iterations++;
        if (iterations >= 10) break; // Safety limit
    } while (!irt.continuationToken.empty());
    
    EXPECT_GE(totalDocs, SEED_DOCUMENT_COUNT);
}

TEST_F(ComprehensiveCosmosAPITests, QueryDocumentsMultipleConditions)
{
    auto irt = testSuiteClient.queryDocuments({
        .database       = testDBName0,
        .collection     = testCollectionNames[0],
        .partitionKey   = "*",
        .queryStatement = "SELECT * FROM c WHERE c.parity = @parity AND c.index < @maxIndex",
        .queryParameters = {
            {{"name", "@parity"}, {"value", "even"}},
            {{"name", "@maxIndex"}, {"value", 5}}
        }
    });
    
    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));
}


TEST_F(ComprehensiveCosmosAPITests, QueryDocumentsAggregate)
{
    auto irt = testSuiteClient.queryDocuments({
        .database       = testDBName0,
        .collection     = testCollectionNames[0],
        .partitionKey   = "*",
        .queryStatement = "SELECT COUNT(1) as count FROM c"
    });
    
    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));
}

// ============================================================================
// SECTION 10: List Documents Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, ListDocumentsBasic)
{
    auto irt = testSuiteClient.listDocuments({
        .database   = testDBName0,
        .collection = testCollectionNames[0]
    });
    
    EXPECT_EQ(200, irt.statusCode);
    EXPECT_TRUE(irt.document.contains("Documents"));
    EXPECT_GE(irt.document.value("_count", 0), 1);
}

TEST_F(ComprehensiveCosmosAPITests, ListDocumentsWithPagination)
{
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t totalDocs = 0;
    int iterations = 0;
    
    do {
        irt = testSuiteClient.listDocuments({
            .database          = testDBName0,
            .collection        = testCollectionNames[0],
            .continuationToken = irt.continuationToken
        });
        
        EXPECT_EQ(200, irt.statusCode);
        totalDocs += irt.document.value("_count", 0);
        
        iterations++;
        if (iterations >= 10) break; // Safety limit
    } while (!irt.continuationToken.empty());
    
    EXPECT_GE(totalDocs, SEED_DOCUMENT_COUNT);
}

// ============================================================================
// SECTION 11: Region Discovery Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, DiscoverRegionsSuccess)
{
    auto rc = testSuiteClient.discoverRegions();
    
    EXPECT_EQ(200, rc.statusCode);
    EXPECT_TRUE(rc.document.contains("writableLocations"));
    EXPECT_TRUE(rc.document.contains("readableLocations"));
    EXPECT_TRUE(rc.document["writableLocations"].is_array());
    EXPECT_TRUE(rc.document["readableLocations"].is_array());
}

// ============================================================================
// SECTION 12: Response Type Tests
// ============================================================================

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeSuccess)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 200;
    
    EXPECT_TRUE(resp.success());
}

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeCreated)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 201;
    
    EXPECT_TRUE(resp.success());
}

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeNoContent)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 204;
    
    EXPECT_TRUE(resp.success());
}

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeClientError)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 400;
    
    EXPECT_FALSE(resp.success());
}

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeNotFound)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 404;
    
    EXPECT_FALSE(resp.success());
}

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeServerError)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 500;
    
    EXPECT_FALSE(resp.success());
}

TEST(ComprehensiveResponseTypeTests, CosmosResponseTypeJsonSerialization)
{
    siddiqsoft::CosmosResponseType resp;
    resp.statusCode = 201;
    resp.document = {{"id", "test123"}, {"name", "Test"}};
    resp.ttx = std::chrono::microseconds(5000);
    
    nlohmann::json json = resp;
    
    EXPECT_EQ(201, json.value("statusCode", 0));
    EXPECT_TRUE(json.contains("document"));
    EXPECT_TRUE(json.contains("ttx"));
    EXPECT_EQ("test123", json.value("/document/id"_json_pointer, ""));
}

// ============================================================================
// SECTION 13: Endpoint URI Management Tests
// ============================================================================

TEST(ComprehensiveEndpointTests, EndpointReadUriRotation)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";
    endpoint.ReadableUris.push_back("https://read1.documents.azure.com/");
    endpoint.ReadableUris.push_back("https://read2.documents.azure.com/");
    endpoint.ReadableUris.push_back("https://read3.documents.azure.com/");
    
    // First read URI
    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());
    
    // Rotate to second
    endpoint.rotateReadUri();
    EXPECT_EQ("https://read2.documents.azure.com/", endpoint.currentReadUri());
    
    // Rotate to third
    endpoint.rotateReadUri();
    EXPECT_EQ("https://read3.documents.azure.com/", endpoint.currentReadUri());
    
    // Rotate back to first
    endpoint.rotateReadUri();
    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());
}

TEST(ComprehensiveEndpointTests, EndpointWriteUriRotation)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";
    endpoint.WritableUris.push_back("https://write1.documents.azure.com/");
    endpoint.WritableUris.push_back("https://write2.documents.azure.com/");
    
    // First write URI
    EXPECT_EQ("https://write1.documents.azure.com/", endpoint.currentWriteUri());
    
    // Rotate to second
    endpoint.rotateWriteUri();
    EXPECT_EQ("https://write2.documents.azure.com/", endpoint.currentWriteUri());
    
    // Rotate back to first
    endpoint.rotateWriteUri();
    EXPECT_EQ("https://write1.documents.azure.com/", endpoint.currentWriteUri());
}

TEST(ComprehensiveEndpointTests, EndpointFallbackToBaseUri)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";
    
    // No readable URIs - should return base URI
    EXPECT_EQ("https://base.documents.azure.com/", endpoint.currentReadUri());
    
    // No writable URIs - should return base URI
    EXPECT_EQ("https://base.documents.azure.com/", endpoint.currentWriteUri());
}

TEST(ComprehensiveEndpointTests, EndpointSingleReadUri)
{
    siddiqsoft::CosmosEndpoint endpoint;
    endpoint.BaseUri = "https://base.documents.azure.com/";
    endpoint.ReadableUris.push_back("https://read1.documents.azure.com/");
    
    // Should always return the same URI
    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());
    endpoint.rotateReadUri();
    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());
    endpoint.rotateReadUri();
    EXPECT_EQ("https://read1.documents.azure.com/", endpoint.currentReadUri());
}

// ============================================================================
// SECTION 14: Error Handling and Validation Tests
// ============================================================================

TEST(ComprehensiveErrorHandlingTests, InvalidConfigurationMissingConnectionStrings)
{
    siddiqsoft::CosmosClient client;
    
    EXPECT_THROW(
        client.configure({{"partitionKeyNames", {"__pk"}}}),
        std::invalid_argument
    );
}

TEST(ComprehensiveErrorHandlingTests, InvalidConfigurationMissingPartitionKeyNames)
{
    siddiqsoft::CosmosClient client;
    auto [priConnStr, secConnStr] = GetActiveConnectionStrings();
    
    EXPECT_THROW(
        client.configure({{"connectionStrings", {priConnStr, secConnStr}}}),
        std::invalid_argument
    );
}

TEST(ComprehensiveErrorHandlingTests, InvalidConfigurationEmptyConnectionStrings)
{
    siddiqsoft::CosmosClient client;
    
    EXPECT_THROW(
        client.configure({
            {"partitionKeyNames", {"__pk"}},
            {"connectionStrings", nlohmann::json::array()}
        }),
        std::invalid_argument
    );
}

TEST(ComprehensiveErrorHandlingTests, InvalidAsyncOperationMissingDatabase)
{
    siddiqsoft::CosmosClient client;
    
    EXPECT_THROW(
        client.async({
            .operation = siddiqsoft::CosmosOperation::listCollections,
            .onResponse = [](auto const&, auto const&) {}
        }),
        std::invalid_argument
    );
}

TEST(ComprehensiveErrorHandlingTests, InvalidAsyncOperationMissingCallback)
{
    siddiqsoft::CosmosClient client;
    
    EXPECT_THROW(
        client.async({
            .operation = siddiqsoft::CosmosOperation::listDatabases
        }),
        std::invalid_argument
    );
}

// ============================================================================
// SECTION 15: Serialization Tests
// ============================================================================

TEST(ComprehensiveSerializationTests, CosmosArgumentTypeSerialization)
{
    siddiqsoft::CosmosArgumentType arg;
    arg.operation = siddiqsoft::CosmosOperation::create;
    arg.database = "testdb";
    arg.collection = "testcoll";
    arg.id = "doc1";
    arg.partitionKey = "pk1";
    arg.document = {{"id", "doc1"}, {"name", "Test"}};
    
    nlohmann::json json = arg;
    
    EXPECT_EQ("create", json.value("operation", ""));
    EXPECT_EQ("testdb", json.value("database", ""));
    EXPECT_EQ("testcoll", json.value("collection", ""));
    EXPECT_EQ("doc1", json.value("id", ""));
    EXPECT_EQ("pk1", json.value("partitionKey", ""));
}


// ============================================================================
// SECTION 16: Bulk Operations Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, BulkCreateDocuments)
{
    std::vector<std::string> docIds;
    const int BULK_COUNT = 10;
    
    // Create multiple documents
    for (int i = 0; i < BULK_COUNT; i++) {
        std::string docId = GenerateDocId(std::format("bulk_{}", i));
        docIds.push_back(docId);
        
        auto rc = testSuiteClient.createDocument({
            .database   = testDBName0,
            .collection = testCollectionNames[0],
            .document   = {
                {"id", docId},
                {"__pk", "siddiqsoft.com"},
                {"bulk_index", i},
                {"batch", "bulk_test"}
            }
        });
        
        EXPECT_EQ(201, rc.statusCode);
    }
    
    // Verify all documents were created
    for (const auto& docId : docIds) {
        auto rc = testSuiteClient.findDocument({
            .database     = testDBName0,
            .collection   = testCollectionNames[0],
            .id           = docId,
            .partitionKey = "siddiqsoft.com"
        });
        
        EXPECT_EQ(200, rc.statusCode);
    }
    
    // Cleanup
    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({
            .database     = testDBName0,
            .collection   = testCollectionNames[0],
            .id           = docId,
            .partitionKey = "siddiqsoft.com"
        });
    }
}

TEST_F(ComprehensiveCosmosAPITests, BulkUpdateDocuments)
{
    std::vector<std::string> docIds;
    const int BULK_COUNT = 5;
    
    // Create documents
    for (int i = 0; i < BULK_COUNT; i++) {
        std::string docId = GenerateDocId(std::format("bulkupd_{}", i));
        docIds.push_back(docId);
        
        testSuiteClient.createDocument({
            .database   = testDBName0,
            .collection = testCollectionNames[0],
            .document   = {
                {"id", docId},
                {"__pk", "siddiqsoft.com"},
                {"version", 1}
            }
        });
    }
    
    // Update all documents
    for (const auto& docId : docIds) {
        auto findRc = testSuiteClient.findDocument({
            .database     = testDBName0,
            .collection   = testCollectionNames[0],
            .id           = docId,
            .partitionKey = "siddiqsoft.com"
        });
        
        nlohmann::json updatedDoc = findRc.document;
        updatedDoc["version"] = 2;
        
        auto updateRc = testSuiteClient.updateDocument({
            .database     = testDBName0,
            .collection   = testCollectionNames[0],
            .id           = docId,
            .partitionKey = "siddiqsoft.com",
            .document     = updatedDoc
        });
        
        EXPECT_EQ(200, updateRc.statusCode);
        EXPECT_EQ(2, updateRc.document.value("version", 0));
    }
    
    // Cleanup
    for (const auto& docId : docIds) {
        testSuiteClient.removeDocument({
            .database     = testDBName0,
            .collection   = testCollectionNames[0],
            .id           = docId,
            .partitionKey = "siddiqsoft.com"
        });
    }
}

// ============================================================================
// SECTION 17: Data Type Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, DocumentWithVariousDataTypes)
{
    std::string docId = GenerateDocId("datatypes");
    
    auto rc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = {
            {"id", docId},
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
            {"empty_object", nlohmann::json::object()}
        }
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("test string", rc.document.value("string_field", ""));
    EXPECT_EQ(42, rc.document.value("int_field", 0));
    EXPECT_NEAR(3.14159, rc.document.value("float_field", 0.0), 0.00001);
    EXPECT_TRUE(rc.document.value("bool_true", false));
    EXPECT_FALSE(rc.document.value("bool_false", true));
    EXPECT_TRUE(rc.document.value("null_field", nlohmann::json()).is_null());
    EXPECT_EQ(4, rc.document.value("array_field", nlohmann::json::array()).size());
    EXPECT_EQ("nested_value", rc.document.value("/object_field/nested_key"_json_pointer, ""));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

// ============================================================================
// SECTION 18: Special Characters and Encoding Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, DocumentWithSpecialCharacters)
{
    std::string docId = GenerateDocId("special_chars");
    
    auto rc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = {
            {"id", docId},
            {"__pk", "siddiqsoft.com"},
            {"unicode", "Hello 世界 🌍"},
            {"special", "!@#$%^&*()_+-=[]{}|;:',.<>?/"},
            {"quotes", "He said \"Hello\" and she replied 'Hi'"},
            {"newlines", "Line 1\nLine 2\nLine 3"},
            {"tabs", "Col1\tCol2\tCol3"},
            {"backslash", "C:\\Users\\test\\file.txt"}
        }
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ("Hello 世界 🌍", rc.document.value("unicode", ""));
    EXPECT_EQ("!@#$%^&*()_+-=[]{}|;:',.<>?/", rc.document.value("special", ""));
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

// ============================================================================
// SECTION 19: Large Document Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, DocumentWithLargeContent)
{
    std::string docId = GenerateDocId("large_doc");
    
    // Create a large array
    nlohmann::json largeArray = nlohmann::json::array();
    for (int i = 0; i < 100; i++) {
        largeArray.push_back({
            {"index", i},
            {"value", std::format("Item {}", i)},
            {"data", std::string(100, 'x')}
        });
    }
    
    auto rc = testSuiteClient.createDocument({
        .database   = testDBName0,
        .collection = testCollectionNames[0],
        .document   = {
            {"id", docId},
            {"__pk", "siddiqsoft.com"},
            {"large_array", largeArray},
            {"large_string", std::string(1000, 'a')}
        }
    });
    
    EXPECT_EQ(201, rc.statusCode);
    EXPECT_EQ(100, rc.document.value("large_array", nlohmann::json::array()).size());
    EXPECT_EQ(1000, rc.document.value("large_string", "").length());
    
    // Cleanup
    testSuiteClient.removeDocument({
        .database     = testDBName0,
        .collection   = testCollectionNames[0],
        .id           = docId,
        .partitionKey = "siddiqsoft.com"
    });
}

// ============================================================================
// SECTION 20: Concurrent Operations Tests
// ============================================================================

TEST_F(ComprehensiveCosmosAPITests, ConcurrentDocumentCreation)
{
    const int THREAD_COUNT = 4;
    const int DOCS_PER_THREAD = 5;
    std::vector<std::jthread> threads;
    std::vector<std::string> createdDocIds;
    std::mutex docIdsMutex;
    
    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([this, t, &createdDocIds, &docIdsMutex]() {
            for (int i = 0; i < DOCS_PER_THREAD; i++) {
                std::string docId = GenerateDocId(std::format("concurrent_{}_{}", t, i));
                
                auto rc = testSuiteClient.createDocument({
                    .database   = testDBName0,
                    .collection = testCollectionNames[0],
                    .document   = {
                        {"id", docId},
                        {"__pk", "siddiqsoft.com"},
                        {"thread", t},
                        {"iteration", i}
                    }
                });
                
                if (rc.statusCode == 201) {
                    std::lock_guard<std::mutex> lock(docIdsMutex);
                    createdDocIds.push_back(docId);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    threads.clear();
    
    // Verify all documents were created
    EXPECT_EQ(THREAD_COUNT * DOCS_PER_THREAD, createdDocIds.size());
    
    // Cleanup
    for (const auto& docId : createdDocIds) {
        testSuiteClient.removeDocument({
            .database     = testDBName0,
            .collection   = testCollectionNames[0],
            .id           = docId,
            .partitionKey = "siddiqsoft.com"
        });
    }
}
