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

#include "gtest/gtest.h"

#include <thread>
#include <barrier>
#include <latch>
#include <functional>
#include <chrono>
#include <ranges>
#include <semaphore>

#include "nlohmann/json.hpp"
#include "../src/azure-cosmos-restcl.hpp"

/*
 * Required Environment Variables
 * CCTEST_PRIMARY_CS
 *
 * Optional Environment Variables
 * CCTEST_SECONDARY_CS
 */


/// @brief Example code
/// Declare the instance, configure and createDocument a document with only three lines!
/// The code here is based on configuration and dynamic fetching for the database, collection and regions.
TEST(CosmosClient_async, nested_async)
{
    std::atomic_bool passTest = false;
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    cc.queue({.operation = siddiqsoft::CosmosOperation::listDatabases},
             [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                 std::cerr << std::format("Completed listDatabases: {}\n", resp);

                 auto dbName = resp.document.value("/Databases/0/id"_json_pointer, "");
                 cc.queue({.operation = siddiqsoft::CosmosOperation::listCollections, .database = dbName},
                          [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx, siddiqsoft::CosmosResponseType const& resp) {
                              std::cerr << std::format("Completed listCollections: {}\n", resp);

                              auto collectionName = resp.document.value("/DocumentCollections/0/id"_json_pointer, "");
                              auto id             = std::format("azure-cosmos-restcl.{}",
                                                    std::chrono::system_clock().now().time_since_epoch().count());
                              auto pkId           = "siddiqsoft.com";

                              cc.queue({.operation    = siddiqsoft::CosmosOperation::create,
                                        .database     = ctx.database,
                                        .collection   = collectionName,
                                        .id           = id,
                                        .partitionKey = pkId,
                                        .document     = {{"id", id},
                                                     {"ttl", 360},
                                                     {"__pk", pkId},
                                                     {"func", __func__},
                                                     {"source", "basic_tests.exe"}}},
                                       [&cc, &passTest](siddiqsoft::CosmosArgumentType const& ctx,
                                                        siddiqsoft::CosmosResponseType const& resp) {
                                           std::cerr << std::format("Completed create: {}\n", resp);
                                           // Remove the document
                                           cc.queue({.operation    = siddiqsoft::CosmosOperation::remove,
                                                     .database     = ctx.database,
                                                     .collection   = ctx.collection,
                                                     .id           = resp.document.value("id", ctx.id),
                                                     .partitionKey = ctx.partitionKey},
                                                    [&cc, &passTest](auto const& ctx, auto const& resp) {
                                                        std::cerr << std::format("Completed removeDocument: {}\n", resp);
                                                        // Document should be removed.
                                                        passTest = true;
                                                        passTest.notify_all();
                                                    });
                                       });
                          });
             });

    // Holds until the test completes
    passTest.wait(false);

    // This sleep is critical to allow the various async operations to complete
    // otherwise the test will abort immediately
    // std::this_thread::sleep_for(std::chrono::seconds(15));
    EXPECT_TRUE(passTest);
}


TEST(CosmosClient_async, listDatabases)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    // Fail fast if the primary conection string is not present in the build environment
    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}})
            .queue({.operation  = siddiqsoft::CosmosOperation::listDatabases,
                    .onResponse = [](auto const& req, const siddiqsoft::CosmosResponseType& resp) {
                        // Expect success.
                        EXPECT_EQ(200, resp.statusCode);
                    }});
}


TEST(CosmosClient_async, listCollections)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    // Fail fast if the primary conection string is not present in the build environment
    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}})
            .queue({.operation  = siddiqsoft::CosmosOperation::listDatabases,
                    .onResponse = [&cc](auto const& req, const siddiqsoft::CosmosResponseType& resp) {
                        // Expect success.
                        EXPECT_EQ(200, resp.statusCode);
                        cc.queue({.operation  = siddiqsoft::CosmosOperation::listCollections,
                                  .database   = resp.document.value("/Databases/0/id"_json_pointer, ""),
                                  .onResponse = [](auto const&, auto const& resp) {
                                      EXPECT_EQ(200, resp.statusCode);
                                  }});
                    }});
}

#ifdef WORK_TO_BE_COMPLETED
/// @brief Tests the listDocuments with a limit of 7 iterations
TEST(CosmosClient_async, listDocuments)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    // Fail fast if the primary conection string is not present in the build environment
    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient               cc;
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t                               totalDocs = 0;
    auto                                   iteration = 7; // max 7 times

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    auto rc = cc.listDatabases();
    EXPECT_EQ(200, rc.statusCode);

    auto rc2 = cc.listCollections(rc.document.value("/Databases/0/id"_json_pointer, ""));
    EXPECT_EQ(200, rc2.statusCode);

    do {
        irt = cc.listDocuments(rc.document.value("/Databases/0/id"_json_pointer, ""),
                               rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                               irt.continuationToken);
        EXPECT_EQ(200, irt.statusCode);
        // We check against a collection that has multiple
        totalDocs += irt.document.value<uint32_t>("_count", 0);
        EXPECT_EQ(100, irt.document.value("_count", 0));
        EXPECT_FALSE(irt.continuationToken.empty());

        // If we run out of the iterations the break out of the loop.
        if (--iteration == 0) break;
    } while (!irt.continuationToken.empty());
}


/// @brief Invokes listDocuments without continuation
TEST(CosmosClient_async, listDocuments_top100)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    // Fail fast if the primary conection string is not present in the build environment
    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient               cc;
    siddiqsoft::CosmosIterableResponseType irt {};
    uint32_t                               totalDocs = 0;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    auto rc = cc.listDatabases();
    EXPECT_EQ(200, rc.statusCode);

    auto rc2 = cc.listCollections(rc.document.value("/Databases/0/id"_json_pointer, ""));
    EXPECT_EQ(200, rc2.statusCode);

    irt = cc.listDocuments(rc.document.value("/Databases/0/id"_json_pointer, ""),
                           rc2.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                           irt.continuationToken);
    EXPECT_EQ(200, irt.statusCode);
    // We check against a collection that has multiple
    totalDocs += irt.document.value<uint32_t>("_count", 0);
    EXPECT_EQ(100, irt.document.value("_count", 0));
    EXPECT_FALSE(irt.continuationToken.empty());
}


/// @brief Test createDocument document with missing "id" field in the document
TEST(CosmosClient_async, createDocument_MissingId)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};


    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto rc = cc.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    // Now, let us createDocument the document
    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    EXPECT_THROW(cc.createDocument(
            dbName, collectionName, {{"MissingId", id}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}});
                 , std::invalid_argument);
}

/// @brief Test createDocument document with missing partition key field in the document
TEST(CosmosClient_async, createDocument_MissingPkId)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string id {};
    std::string pkId {};


    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto rc = cc.listDatabases();
    EXPECT_EQ(200, rc.statusCode);
    dbName   = rc.document.value("/Databases/0/id"_json_pointer, "");

    auto rc2 = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2.statusCode);
    collectionName = rc2.document.value("/DocumentCollections/0/id"_json_pointer, "");

    // Now, let us createDocument the document
    id   = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId = "siddiqsoft.com";

    EXPECT_THROW(cc.createDocument(
            dbName, collectionName, {{"id", id}, {"ttl", 360}, {"Missing__pk", pkId}, {"source", "basic_tests.exe"}});
                 , std::invalid_argument);
}
#endif


TEST(CosmosClient_async, nestedOps)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string      priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string      secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::atomic_bool passTest   = false;

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    cc.configure(nlohmann::json {{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    // First we get the first database from the connection string..
    cc.queue(
            {.operation  = siddiqsoft::CosmosOperation::listDatabases,
             .onResponse = [&cc, &passTest](const auto& ctx, const auto& resp) {
                 std::cerr << std::format("0..{}\n", ctx);
                 EXPECT_EQ(200, resp.statusCode);
                 // Find out the first collection's name..
                 cc.queue(
                         {.operation  = siddiqsoft::CosmosOperation::listCollections,
                          .database   = resp.document.value("/Databases/0/id"_json_pointer, ""),
                          .onResponse = [&cc, &passTest](const auto& ctx, const auto& resp) {
                              std::cerr << std::format("1..{}\n", ctx);
                              EXPECT_EQ(200, resp.statusCode);
                              // Create a document..
                              cc.queue(
                                      {.operation    = siddiqsoft::CosmosOperation::create,
                                       .database     = ctx.database,
                                       .collection   = resp.document.value("/DocumentCollections/0/id"_json_pointer, ""),
                                       .partitionKey = "siddiqsoft.com",
                                       .document     = {{"id",
                                                     std::format("azure-cosmos-restcl.{}",
                                                                 std::chrono::system_clock().now().time_since_epoch().count())},
                                                    {"ttl", 360},
                                                    {"__pk", "siddiqsoft.com"},
                                                    {"mode", "create"},
                                                    {"source", "basic_tests.exe"}},
                                       .onResponse   = [&cc, &passTest](const auto& ctx, const auto& resp) {
                                           std::cerr << std::format("2..{}\n", ctx);
                                           EXPECT_EQ(201, resp.statusCode);
                                           EXPECT_EQ("create", resp.document.value("mode", ""));

                                           // Now we upsert..
                                           auto newDocument    = resp.document;
                                           newDocument["mode"] = "upsert";
                                           cc.queue(
                                                   {.operation    = siddiqsoft::CosmosOperation::upsert,
                                                    .database     = ctx.database,
                                                    .collection   = ctx.collection,
                                                    .id           = ctx.id,
                                                    .partitionKey = ctx.partitionKey,
                                                    .document     = newDocument,
                                                    .onResponse   = [&cc, &passTest](const auto& ctx, const auto& resp) {
                                                        std::cerr << std::format("3..{}\n", ctx);
                                                        EXPECT_EQ(200, resp.statusCode);
                                                        EXPECT_EQ("upsert", resp.document.value("mode", ""));
                                                        // Now, we update the just upsert'd document..
                                                        auto newDocument    = resp.document;
                                                        newDocument["mode"] = "update";
                                                        cc.queue(
                                                                {.operation    = siddiqsoft::CosmosOperation::update,
                                                                 .database     = ctx.database,
                                                                 .collection   = ctx.collection,
                                                                 .id           = resp.document["id"],
                                                                 .partitionKey = ctx.partitionKey,
                                                                 .document     = newDocument,
                                                                 .onResponse = [&cc, &passTest](const auto& ctx, const auto& resp) {
                                                                     std::cerr << std::format("4..{}\n", ctx);
                                                                     EXPECT_EQ(200, resp.statusCode);
                                                                     EXPECT_EQ("update", resp.document.value("mode", ""));
                                                                     // Now, we should "find" this document
                                                                     cc.queue(
                                                                             {.operation    = siddiqsoft::CosmosOperation::find,
                                                                              .database     = ctx.database,
                                                                              .collection   = ctx.collection,
                                                                              .id           = ctx.id,
                                                                              .partitionKey = ctx.partitionKey,
                                                                              .onResponse   = [&cc, &passTest](const auto& ctx,
                                                                                                             const auto& resp) {
                                                                                  std::cerr << std::format("5..{}\n", ctx);
                                                                                  EXPECT_EQ(200, resp.statusCode);
                                                                                  EXPECT_EQ("update",
                                                                                            resp.document.value("mode", ""));
                                                                                  // Finally, remove this document.
                                                                                  cc.queue(
                                                                                          {.operation = siddiqsoft::
                                                                                                   CosmosOperation::remove,
                                                                                           .database     = ctx.database,
                                                                                           .collection   = ctx.collection,
                                                                                           .id           = ctx.id,
                                                                                           .partitionKey = ctx.partitionKey,
                                                                                           .onResponse   = [&cc, &passTest](
                                                                                                                 const auto& ctx,
                                                                                                                 const auto& resp) {
                                                                                               std::cerr << std::format("6..{}\n",
                                                                                                                        ctx);
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

TEST(CosmosClient_async, async_discoverRegions)
{
    std::atomic_bool passTest = false;

    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    // The configure calls the method discoverRegions
    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    // ASSERT_NO_THROW(cc.queue(nlohmann::json {{"operation", "discoverRegions"}, {"source", __func__}},
    //                         [&](auto& resp) { passTest = true; }));
    cc.queue({.operation  = siddiqsoft::CosmosOperation::discoverRegions,
              .onResponse = [&passTest](auto const& ctx, auto const& resp) {
                  std::cerr << "Invoked from the dispatcher!" << std::endl;
                  passTest = true;
                  passTest.notify_all();
              }});

    passTest.wait(false);
    nlohmann::json info = cc;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(5, info.size()) << info.dump(3);

    // Check that we have read/write locations detected.
    // Atleast one read location
    EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
    // Atleast one write location
    EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
}
