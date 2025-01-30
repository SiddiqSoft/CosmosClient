/*
    CosmosClient - Tests
    Azure Cosmos REST-API Client for Modern C++

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

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/cosmoscl.hpp"

#include "test_common.hpp"

class CosmosClientAsync : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        // Perform one-time setup for the entire test suite
        testSuiteClient.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetConnectionStrings()}});


        if (auto rc2 = TSfindDatabase(testDBName1); rc2.statusCode == 404) {
            auto rc1 = TScreateDatabase(testDBName1);
        }

        // Next we create the collections..
        try {
            for (auto& collName : testCollectionNames) {
                if (auto rc3 = TScreateCollection(testDBName1, collName); rc3.statusCode == 201) {
                    for (auto i = 0; i < SEED_DOCUMENT_COUNT; i++) {
                        auto rc4 =
                                testSuiteClient.createDocument({.database   = testDBName1,
                                                                .collection = collName,
                                                                .document   = {{"id", std::format("{:0X}.{}", i, (i % 2) == 0 ? "even" : "odd")},
                                                                               {"__pk", (i % 2) == 0 ? "even.siddiqsoft.com" : "odd.siddiqsoft.com"},
                                                                               {"func", __func__},
                                                                               {"extra", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" : "odd")},
                                                                               {"source", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" : "odd")}}});

                        /*testSuiteClient.async(
                                {.operation    = siddiqsoft::CosmosOperation::create,
                                 .database     = testDBName1,
                                 .collection   = collName,
                                 .id           = std::format("{:0X}.{}", i, (i % 2) == 0 ? "even" : "odd"),
                                 .partitionKey = "siddiqsoft.com",
                                 .document     = {{"id", std::format("{:0X}.{}", i, (i % 2) == 0 ? "even" : "odd")},
                                                  {"ttl", 1360},
                                                  {"__pk", "siddiqsoft.com"},
                                                  {"func", __func__},
                                                  {"source", std::format("{:0X}-{}-{}", i, getpid(), (i % 2) == 0 ? "even" :
                           "odd")}}, .onResponse   = [&](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType
                           const& resp) { std::cerr << std::format("Completed create: {}\n", resp);
                                 }});*/
                    }
                }
            }
        }
        catch (...) {
        }
    }

    static void TearDownTestCase()
    {
        // Perform one-time cleanup for the entire test suite
        // Cleanup the db we just created.
        // auto rc9 = TSdeleteDatabase(testDBName1);
    }
};


/// @brief Example code
/// Declare the instance, configure and createDocument a document with only three lines!
/// The code here is based on configuration and dynamic fetching for the database, collection and regions.
TEST_F(CosmosClientAsync, async_example)
{
    std::atomic_bool         passTest = false;

    siddiqsoft::CosmosClient cc;
    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetConnectionStrings()}});

    cc.async({.operation  = siddiqsoft::CosmosOperation::listDatabases,
              .onResponse = [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                  std::cerr << std::format("Completed listDatabases: {}\n", resp);

                  auto dbName = resp.document.value("/Databases/0/id"_json_pointer, "");
                  testSuiteClient.async(
                          {.operation  = siddiqsoft::CosmosOperation::listCollections,
                           .database   = dbName,
                           .onResponse = [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                               std::cerr << std::format("Completed listCollections: {}\n", resp);

                               auto collectionName = resp.document.value("/DocumentCollections/0/id"_json_pointer, "");
                               auto id             = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
                               auto pkId           = "siddiqsoft.com";

                               testSuiteClient.async(
                                       {.operation    = siddiqsoft::CosmosOperation::create,
                                        .database     = ctx.database,
                                        .collection   = collectionName,
                                        .id           = id,
                                        .partitionKey = pkId,
                                        .document     = {{"id", id}, {"ttl", 360}, {"__pk", pkId}, {"func", __func__}, {"source", "basic_tests.exe"}},
                                        .onResponse   = [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                                            std::cerr << std::format("Completed create: {}\n", resp);
                                            // Remove the document
                                            testSuiteClient.async({.operation    = siddiqsoft::CosmosOperation::remove,
                                                                     .database     = ctx.database,
                                                                     .collection   = ctx.collection,
                                                                     .id           = resp.document.value("id", ctx.id),
                                                                     .partitionKey = ctx.partitionKey,
                                                                     .onResponse   = [&cc, &passTest](auto const& ctx, auto const& resp) {
                                                                       std::cerr << std::format("Completed removeDocument: {}\n", resp);
                                                                       // Document should be removed.
                                                                       passTest = true;
                                                                       passTest.notify_all();
                                                                   }});
                                        }});
                           }});
              }});

    // Holds until the test completes
    passTest.wait(false);

    // This sleep is critical to allow the various async operations to complete
    // otherwise the test will abort immediately
    // std::this_thread::sleep_for(std::chrono::seconds(15));
    EXPECT_TRUE(passTest);
}


TEST_F(CosmosClientAsync, async_listDatabases)
{
    testSuiteClient.async({.operation = siddiqsoft::CosmosOperation::listDatabases, .onResponse = [](auto const& req, const siddiqsoft::CosmosResponseType& resp) {
                               // Expect success.
                               EXPECT_EQ(200, resp.statusCode);
                           }});
}


TEST_F(CosmosClientAsync, async_listCollections)
{
    testSuiteClient.async({.operation = siddiqsoft::CosmosOperation::listDatabases, .onResponse = [&](auto const& req, const siddiqsoft::CosmosResponseType& resp) {
                               // Expect success.
                               EXPECT_EQ(200, resp.statusCode);
                               testSuiteClient.async({.operation  = siddiqsoft::CosmosOperation::listCollections,
                                                      .database   = resp.document.value("/Databases/0/id"_json_pointer, ""),
                                                      .onResponse = [](auto const&, auto const& resp) {
                                                          EXPECT_EQ(200, resp.statusCode);
                                                      }});
                           }});
}

/// @brief Tests the listDocuments with a limit of 7 iterations
TEST_F(CosmosClientAsync, async_listDocuments)
{
    auto rc = testSuiteClient.listDatabases();
    EXPECT_EQ(200, rc.statusCode);

    auto rc2 = testSuiteClient.listCollections({.database = rc.document.value("/Databases/0/id"_json_pointer, "")});
    EXPECT_EQ(200, rc2.statusCode);

    uint32_t totalDocs = 0;
    uint32_t iteration = 7; // max 7 times


    // Start the first listDocuments request.. and we will build our completion in the callback
    // The callback model for the listDocument is such that it will automatically issue additional
    // async operation until the continuationtoken is empty and the status is valid.
    // The client may not invoke any additional requests.
    testSuiteClient.async({.operation  = siddiqsoft::CosmosOperation::listDocuments,
                           .database   = rc.document.value("/Databases/0/id"_json_pointer, ""),
                           .collection = rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                           .onResponse = [&](auto const& ctx, auto const& resp) {
                               totalDocs += resp.document.value("_count", 0);
                               --iteration;
                               std::cerr << std::format("....{:02} {}/{}...status:{}..current totalDocs: {:04}...ttx:{}\n",
                                                        iteration,
                                                        ctx.database,
                                                        ctx.collection,
                                                        resp.statusCode,
                                                        totalDocs,
                                                        std::chrono::duration_cast<std::chrono::milliseconds>(resp.ttx));
                           }});

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cerr << std::format("Total Docs: {}\n", totalDocs);
    std::cerr << std::format("Info: {}\n", testSuiteClient);
}


/// @brief Test createDocument document with missing "id" field in the document
TEST_F(CosmosClientAsync, async_createDocument_MissingId)
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

    // Now, let us createDocument the document
    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    // Missing "id" parameter
    EXPECT_THROW(testSuiteClient.async({.operation  = siddiqsoft::CosmosOperation::create,
                                        .database   = dbName,
                                        .collection = collectionName,
                                        .document   = {{"__pk", pkId}, {"ttl", 360}, {"source", "basic_tests.exe"}}});
                 , std::invalid_argument);
}


/// @brief Test createDocument document with missing partition key field in the document
TEST_F(CosmosClientAsync, async_createDocument_MissingPkId)
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

    // Now, let us createDocument the document
    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    EXPECT_THROW(testSuiteClient.async({.operation  = siddiqsoft::CosmosOperation::create,
                                        .database   = dbName,
                                        .collection = collectionName,
                                        .document   = {{"id", id}, {"ttl", 360}, {"Missing__pk", pkId}, {"source", "basic_tests.exe"}}});
                 , std::invalid_argument);
}


TEST_F(CosmosClientAsync, async_nestedOps)
{
    std::atomic_bool         passTest = false;

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", GetConnectionStrings()}}));

    // First we get the first database from the connection string..
    auto thisUniqueDocId = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    cc.async({.operation = siddiqsoft::CosmosOperation::listDatabases, .onResponse = [&cc, &passTest, &thisUniqueDocId](const auto& ctx, const auto& resp) {
                  std::cerr << std::format("....0..id:{}..{}\n", thisUniqueDocId, ctx);
                  EXPECT_EQ(200, resp.statusCode);
                  // Find out the first collection's name..
                  cc.async({.operation  = siddiqsoft::CosmosOperation::listCollections,
                            .database   = resp.document.value("/Databases/0/id"_json_pointer, ""),
                            .onResponse = [&cc, &passTest, &thisUniqueDocId](const auto& ctx, const auto& resp) {
                                std::cerr << std::format("....1..id:{}..{}\n", thisUniqueDocId, ctx);
                                EXPECT_EQ(200, resp.statusCode);
                                // Create a document..
                                cc.async({.operation    = siddiqsoft::CosmosOperation::create,
                                          .database     = ctx.database,
                                          .collection   = resp.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                                          .partitionKey = "siddiqsoft.com",
                                          .document     = {{"id", thisUniqueDocId},
                                                           {"ttl", 1360},
                                                           {"__pk", "siddiqsoft.com"},
                                                           {"mode", "create-new"},
                                                           {"source", "basic_tests.exe"}},
                                          .onResponse   = [&cc, &passTest, &thisUniqueDocId](const auto& ctx, const auto& resp) {
                                              std::cerr << std::format("....2..id:{}..{}\n", resp.document.value("id", ""), ctx);
                                              EXPECT_EQ(201, resp.statusCode);
                                              EXPECT_EQ(thisUniqueDocId, resp.document.value("id", ""));
                                              EXPECT_EQ("create-new", resp.document.value("mode", ""));

                                              // Now we upsert..
                                              auto newDocument        = resp.document;
                                              newDocument["mode"]     = "upsert";
                                              newDocument["upsert-d"] = thisUniqueDocId;
                                              cc.async({.operation    = siddiqsoft::CosmosOperation::upsert,
                                                          .database     = ctx.database,
                                                          .collection   = ctx.collection,
                                                          .id           = resp.document.value("id", ""),
                                                          .partitionKey = ctx.partitionKey,
                                                          .document     = newDocument,
                                                          .onResponse   = [&cc, &passTest, &thisUniqueDocId](const auto& ctx, const auto& resp) {
                                                            std::cerr << std::format("....3..id:{}..{}\n", resp.document.value("id", ""), ctx);
                                                            ASSERT_EQ(201, resp.statusCode);
                                                            EXPECT_EQ(thisUniqueDocId, resp.document.value("id", ""));
                                                            EXPECT_EQ("upsert", resp.document.value("mode", ""));
                                                            EXPECT_EQ(thisUniqueDocId, resp.document.value("upsert-d", ""));
                                                            // Now, we update the just upsert'd document..
                                                            auto newDocument    = resp.document;
                                                            newDocument["mode"] = "update";
                                                            cc.async({.operation    = siddiqsoft::CosmosOperation::update,
                                                                          .database     = ctx.database,
                                                                          .collection   = ctx.collection,
                                                                          .id           = resp.document["id"],
                                                                          .partitionKey = ctx.partitionKey,
                                                                          .document     = newDocument,
                                                                          .onResponse   = [&cc, &passTest, &thisUniqueDocId](const auto& ctx, const auto& resp) {
                                                                          std::cerr << std::format("....4..id:{}..{}\n", resp.document.value("id", ""), ctx);
                                                                          EXPECT_EQ(200, resp.statusCode);
                                                                          EXPECT_EQ("update", resp.document.value("mode", ""));
                                                                          // Now, we should "find" this document
                                                                          cc.async({.operation    = siddiqsoft::CosmosOperation::find,
                                                                                          .database     = ctx.database,
                                                                                          .collection   = ctx.collection,
                                                                                          .id           = ctx.id,
                                                                                          .partitionKey = ctx.partitionKey,
                                                                                          .onResponse   = [&cc, &passTest, &thisUniqueDocId](const auto& ctx,
                                                                                                                                     const auto& resp) {
                                                                                        std::cerr << std::format("..5..id:{}..{}\n", thisUniqueDocId, ctx);
                                                                                        ASSERT_EQ(200, resp.statusCode);
                                                                                        EXPECT_EQ(thisUniqueDocId, resp.document.value("id", ""));
                                                                                        ASSERT_EQ("update", resp.document.value("mode", ""));
                                                                                        // Finally, remove this document.
                                                                                        cc.async({.operation    = siddiqsoft::CosmosOperation::remove,
                                                                                                          .database     = ctx.database,
                                                                                                          .collection   = ctx.collection,
                                                                                                          .id           = ctx.id,
                                                                                                          .partitionKey = ctx.partitionKey,
                                                                                                          .onResponse   = [&cc, &passTest, &thisUniqueDocId](
                                                                                                                        const auto& ctx, const auto& resp) {
                                                                                                      std::cerr << std::format(
                                                                                                              "....6..id:{}..{}\n", thisUniqueDocId, ctx);
                                                                                                      EXPECT_EQ(204, resp.statusCode);
                                                                                                      // Finished!
                                                                                                      passTest = true;
                                                                                                      passTest.notify_all();
                                                                                                  }});
                                                                                    }});
                                                                      }});
                                                        }});
                                          }});
                            }});
              }});
    // Caution: this will hang forever if there is any error
    passTest.wait(false);
}

TEST_F(CosmosClientAsync, async_discoverRegions)
{
    std::atomic_bool passTest = false;

    testSuiteClient.async({.operation = siddiqsoft::CosmosOperation::discoverRegions, .onResponse = [&passTest](auto const& ctx, auto const& resp) {
                               std::cerr << "Invoked from the dispatcher!" << std::endl;
                               passTest = true;
                               passTest.notify_all();
                           }});

    passTest.wait(false);
    nlohmann::json info = testSuiteClient;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(5, info.size()) << info.dump(3);

    // Check that we have read/write locations detected.
    // Atleast one read location
    EXPECT_LE(1, testSuiteClient.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, testSuiteClient.cnxn.current().ReadableUris.size());
    // Atleast one write location
    EXPECT_LE(1, testSuiteClient.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, testSuiteClient.cnxn.current().WritableUris.size());
}


TEST_F(CosmosClientAsync, async_queryDocument)
{
    std::string              dbName         = testDBName1;
    std::string              collectionName = testCollectionNames[2];
    std::vector<std::string> docIds {};
    std::string              pkId {"siddiqsoft.com"};
    std::string              sourceId = std::format("{}-{}", getpid(), siddiqsoft::CosmosClient::CosmosClientUserAgentString);
    constexpr auto           DOCS {10};
    nlohmann::json           allDocs = nlohmann::json::array();
    uint32_t                 allDocsCount {};

    // Wait for the async operations during setup to complete.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // First, we queryDocuments for all items that match our criteria (source=__func__)
    testSuiteClient.async({.operation       = siddiqsoft::CosmosOperation::query,
                           .database        = dbName,
                           .collection      = collectionName,
                           .partitionKey    = "*",
                           .queryStatement  = "SELECT * FROM c WHERE contains(c.source, @v1)",
                           .queryParameters = {{{"name", "@v1"}, {"value", std::format("{}-", getpid())}}},
                           .onResponse      = [&](auto const& ctx, auto const& resp) {
                               std::cerr << std::format("....1..{}\n", resp);
                               EXPECT_EQ(200, resp.statusCode);
                               // Invoked each time we have data block until empty continuationToken
                               // We do not need to perform re-query as the lib will perform these for us and invoke this callback!
                               if (200 == resp.statusCode && resp.document.contains("Documents") && !resp.document.at("Documents").is_null()) {
                                   // Append to the current container
                                   allDocs.insert(allDocs.end(), resp.document["Documents"].begin(), resp.document["Documents"].end());
                                   allDocsCount += resp.document.value("_count", 0);
                                   std::cerr << "Items: " << resp.document.value("_count", 0)
                                             << "  Result ttx:" << std::chrono::duration_cast<std::chrono::milliseconds>(resp.ttx) << std::endl;
                               }
                           }});

    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_EQ(DOCS, allDocsCount); // total


    // Clear stuff..
    allDocs      = nlohmann::json::array();
    allDocsCount = 0;
    // Query with partition key "odd"; out of five, 2 should be odd: 1, 3
    testSuiteClient.async({.operation       = siddiqsoft::CosmosOperation::query,
                           .database        = dbName,
                           .collection      = collectionName,
                           .partitionKey    = "*",
                           .queryStatement  = "SELECT * FROM c WHERE contains(c.extra, @v1)",
                           .queryParameters = {{{"name", "@v1"}, {"value", "odd"}}},
                           .onResponse      = [&](auto const& ctx, auto const& resp) {
                               std::cerr << std::format("....2..{}\n", resp);
                               EXPECT_EQ(200, resp.statusCode);
                               // Invoked each time we have data block until empty continuationToken
                               // We do not need to perform re-query as the lib will perform these for us and invoke this callback!
                               if (200 == resp.statusCode && resp.document.contains("Documents") && !resp.document.at("Documents").is_null()) {
                                   // Append to the current container
                                   allDocs.insert(allDocs.end(), resp.document["Documents"].begin(), resp.document["Documents"].end());
                                   allDocsCount += resp.document.value("_count", 0);
                                   std::cerr << "ODD Items: " << resp.document.value("_count", 0)
                                             << "  Result ttx:" << std::chrono::duration_cast<std::chrono::milliseconds>(resp.ttx) << std::endl;
                               }
                           }});
    std::this_thread::sleep_for(std::chrono::seconds(5));
    EXPECT_EQ(5, allDocsCount); // odd

    // Query with partition key "odd"; out of five, 3 should be even: 0, 2
    // Clear stuff..
    allDocs      = nlohmann::json::array();
    allDocsCount = 0;
    // Query with partition key "odd"; out of five, 2 should be odd: 1, 3
    testSuiteClient.async({.operation       = siddiqsoft::CosmosOperation::query,
                           .database        = dbName,
                           .collection      = collectionName,
                           .partitionKey    = "*",
                           .queryStatement  = "SELECT * FROM c WHERE contains(c.source, @v1)",
                           .queryParameters = {{{"name", "@v1"}, {"value", "even"}}},
                           .onResponse      = [&](auto const& ctx, auto const& resp) {
                               std::cerr << std::format("....3..{}\n", resp);
                               EXPECT_EQ(200, resp.statusCode);
                               // Invoked each time we have data block until empty continuationToken
                               // We do not need to perform re-query as the lib will perform these for us and invoke this callback!
                               if (200 == resp.statusCode && resp.document.contains("Documents") && !resp.document.at("Documents").is_null()) {
                                   // Append to the current container
                                   allDocs.insert(allDocs.end(), resp.document["Documents"].begin(), resp.document["Documents"].end());
                                   allDocsCount += resp.document.value("_count", 0);
                                   std::cerr << "EVEN Items: " << resp.document.value("_count", 0)
                                             << "  Result ttx:" << std::chrono::duration_cast<std::chrono::milliseconds>(resp.ttx) << std::endl;
                               }
                           }});
    std::this_thread::sleep_for(std::chrono::seconds(5));
    EXPECT_EQ(5, allDocsCount); // even
}
