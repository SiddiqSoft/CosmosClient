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


/// @brief Test the serializers and cast operators
TEST(CosmosConnection, test1_n)
{
    std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
                     ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {cs};

    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
              ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;",
              std::string(cd.Primary));
    // std::cerr << "Primary........." << std::string(cd.Primary) << std::endl;

    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cd.Primary.BaseUri));
    // std::cerr << "Primary Uri....." << std::string(cd.Primary.BaseUri) << std::endl;

    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cd.Primary.EncodedKey);
    // std::cerr << "Primary Key....." << cd.Primary.EncodedKey << std::endl;

    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cd.Primary.BaseUri}.authority.host);
    // std::cerr << "Primary.host...." << ::siddiqsoft::Uri<char> {cd.Primary.BaseUri}.authority.host << std::endl;

    // std::cerr << "Uri............." << cd.Primary.BaseUri << std::endl;
}

/// @brief Tests the serializers.
/// Compiliation will fail for malformed/missing serializers
TEST(CosmosConnection, test2_n)
{
    std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
                     ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {cs};

    nlohmann::json               info = cd;
    EXPECT_EQ(4, info.size());
    // std::cerr << "json............" << info.dump() << std::endl;
    // std::cerr << "operator<<......" << cd << std::endl;
    // std::cerr << "std:format......" << std::format("{}", cd) << std::endl;
}


/// @brief Test rotate between primary and secondary connection
TEST(CosmosConnection, rotateConnection_1)
{
    std::string pcs = "AccountEndpoint=https://YOURDBNAME-1.documents.azure.com:443/"
                      ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    std::string scs = "AccountEndpoint=https://YOURDBNAME-2.documents.azure.com:443/"
                      ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {pcs, scs};

    // We must start at Primary
    // //std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    // Swap to the secondary..
    cd.rotate();
    // //std::cerr << "1. Should be Secondary." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::SecondaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(scs, cd.current().string());

    // Swap again.. which should end up at Primary
    cd.rotate();
    // //std::cerr << "2. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    // Force set at 2
    cd.rotate(2);
    // //std::cerr << "3. Should be Secondary." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::SecondaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(scs, cd.current().string());

    // Force set at 1
    cd.rotate(1);
    // //std::cerr << "4. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());
}


/// @brief Check the rotate method with single connection
TEST(CosmosConnection, rotateConnection_2)
{
    std::string pcs = "AccountEndpoint=https://YOURDBNAME-1.documents.azure.com:443/"
                      ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    siddiqsoft::CosmosConnection cd {pcs};

    // We must start at Primary
    // //std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    // Swap to the secondary..
    cd.rotate();
    // //std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    // Swap again.. which should end up at Primary
    cd.rotate();
    // //std::cerr << "2. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    // Force set at 2
    cd.rotate(2);
    // //std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());

    // Force set at 1
    cd.rotate(1);
    // //std::cerr << "4. Should be Primary..." << cd.current() << std::endl;
    EXPECT_EQ(siddiqsoft::CosmosConnection::CurrentConnectionIdType::PrimaryConnection, cd.CurrentConnectionId);
    EXPECT_EQ(pcs, cd.current().string());
}


/// @brief Test serializers and cast for the CosmosEndpoint object no extra read/write locations
TEST(CosmosEndpoint, test1_n)
{
    siddiqsoft::CosmosEndpoint cs;

    cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
         ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cs.BaseUri));
    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cs.EncodedKey);
    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
              ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;",
              std::string(cs));
    // std::cerr << "operator<<......" << cs << std::endl;
    // std::cerr << "std::format....." << std::format("{}", cs) << std::endl;
    // std::cerr << "std::format.Uri." << std::format("{}", cs.BaseUri) << std::endl;
    // std::cerr << "string.operator." << std::string(cs.BaseUri) << std::endl;

    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    cs.rotateReadUri();
    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
    cs.rotateWriteUri();
    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

    auto info = nlohmann::json(cs);
    EXPECT_EQ(6, info.size());
    // //std::cerr << "json............" << info.dump(3) << std::endl;
}


/// @brief Test serializers and cast operators for CosmosEndpoint object with additional read/write locations.
TEST(CosmosEndpoint, test2_n)
{
    using namespace siddiqsoft::literals;

    siddiqsoft::CosmosEndpoint cs;

    cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
         ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cs.BaseUri));
    EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cs.EncodedKey);
    EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
              ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;",
              std::string(cs));

    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

    // Feed some readlocations and writelocations
    cs.ReadableUris.push_back("https://YOURDBNAME-r1.documents.azure.com:10/"_Uri);
    cs.ReadableUris.push_back("https://YOURDBNAME-r2.documents.azure.com:11/"_Uri);
    // Note that writable uris have ports 90 and 91 so we can test the parser
    cs.WritableUris.push_back("https://YOURDBNAME-w1.documents.azure.com:90/"_Uri);
    cs.WritableUris.push_back("https://YOURDBNAME-w2.documents.azure.com:91/"_Uri);

    // std::cerr << "operator<<......" << cs << std::endl;
    // std::cerr << "std::format....." << std::format("{}", cs) << std::endl;
    // std::cerr << "std::format.Uri." << std::format("{}", cs.BaseUri) << std::endl;
    // std::cerr << "string.operator." << std::string(cs.BaseUri) << std::endl;

    // Test readable uris..
    EXPECT_EQ("YOURDBNAME-r1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    cs.rotateReadUri();
    EXPECT_EQ("YOURDBNAME-r2.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
    cs.rotateReadUri();
    EXPECT_EQ("YOURDBNAME-r1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    // Test writable uris..
    EXPECT_EQ("YOURDBNAME-w1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
    cs.rotateWriteUri();
    EXPECT_EQ("YOURDBNAME-w2.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
    cs.rotateWriteUri();
    EXPECT_EQ("YOURDBNAME-w1.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

    // If we exhaust the reads..
    cs.ReadableUris.clear();
    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

    cs.WritableUris.clear();
    EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
}


/// @brief Example code
/// Declare the instance, configure and create a document with only three lines!
/// The code here is based on configuration and dynamic fetching for the database, collection and regions.
TEST(CosmosClient, example1)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});


    if (auto [rc, dbDoc] = cc.listDatabases(); 200 == rc) {
        auto dbName = dbDoc.value("/Databases/0/id"_json_pointer, "");

        if (auto [rc2, respCollections] = cc.listCollections(dbName); 200 == rc2) {
            auto collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

            // Now, let us create the document
            auto docId = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
            auto pkId  = "siddiqsoft.com";

            if (auto [rc3, cDoc] =
                        cc.create(dbName,
                                  collectionName,
                                  {{"id", docId}, {"ttl", 360}, {"__pk", pkId}, {"func", __func__}, {"source", "basic_tests.exe"}});
                201 == rc3)
            {
                // ...do something
                // ...useful with cDoc..

                // Remove the just created document..
                auto rc4 = cc.remove(dbName, collectionName, cDoc.value("id", docId), pkId);
                EXPECT_EQ(204, rc4);
            }
        }
    }
}


/// @brief Test the configuration defaults
/// Checks that we match the changes to the configuration defaults in code against the documentation and client tests.
TEST(CosmosClient, configure_Defaults)
{
    siddiqsoft::CosmosClient cc;

    // Check that we have read/write locations detected.
    auto currentConfig = cc.configuration();

    EXPECT_TRUE(currentConfig.contains("apiVersion"));
    // This forces us to make sure we check side-effects whenever the version changes!
    EXPECT_EQ("2018-12-31", currentConfig.value("apiVersion", ""));
    EXPECT_TRUE(currentConfig.contains("connectionStrings"));
    EXPECT_TRUE(currentConfig.contains("partitionKeyNames"));
}


/// @brief Checks the CosmosClient to_json has a consistent output
TEST(CosmosClient, configure_check_json)
{
    siddiqsoft::CosmosClient cc;

    // Check that the to_json has a consistent output
    nlohmann::json info = cc;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(3, info.size());
}


/// @brief Test to check call to configure will pull in the serviceSettings.
/// The `serviceSettings` contains information about the service as reported by call to discoverRegions
/// Here we perform a configure and check the responses obtained from the Azure service.
///
/// NOTE: The `serviceSettings` is protected and this test declares the macro `COSMOSCLIENT_TESTING_MODE`
/// to enable public access during testing stage only.
TEST(CosmosClient, configure_1)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    // Check that we have read/write locations detected.
    auto currentConfig = cc.configuration();

    EXPECT_TRUE(cc.serviceSettings["writableLocations"].is_array());
    EXPECT_TRUE(cc.serviceSettings["readableLocations"].is_array());

    // Atleast one read location
    EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
    // Atleast one write location
    EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
}


TEST(CosmosClient, discoverRegions)
{
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

    nlohmann::json info = cc;
    EXPECT_TRUE(info.contains("serviceSettings"));
    EXPECT_TRUE(info.contains("database"));
    EXPECT_TRUE(info.contains("configuration"));
    EXPECT_EQ(3, info.size());

    // Check that we have read/write locations detected.
    // Atleast one read location
    EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().ReadableUris.size());
    // Atleast one write location
    EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
    EXPECT_LE(1, cc.cnxn.current().WritableUris.size());
}


TEST(CosmosClient, discoverRegions_BadPrimary)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = "AccountEndpoint=https://localhost:4043/"
                             ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
    std::string secConnStr = std::getenv("CCTEST_PRIMARY_CS");

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});


    // First attempt should fail.
    auto rc = cc.discoverRegions();
    // //std::cerr << "1/3....rc:" << rc << " Expect failure." << std::endl;
    EXPECT_NE(200, rc.statusCode) << rc.document.dump(3);

    // Try again.. we should succeed.
    cc.cnxn.rotate();
    rc = cc.discoverRegions();
    // //std::cerr << "2/3....rc:" << rc << " Expect success." << std::endl;
    EXPECT_EQ(200, rc.statusCode) << rc.document.dump(3);

    // Try again.. we should fail again!
    cc.cnxn.rotate();
    rc = cc.discoverRegions();
    // //std::cerr << "3/3....rc:" << rc << " Expect failure." << std::endl;
    EXPECT_NE(200, rc.statusCode) << rc.document.dump(3);
}


TEST(CosmosClient, listDatabases)
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

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    auto [rc, resp] = cc.listDatabases();
    // Expect success.
    EXPECT_EQ(200, rc);
}


TEST(CosmosClient, listCollections)
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

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    auto [rc, resp] = cc.listDatabases();
    // Expect success.
    EXPECT_EQ(200, rc);

    auto [rc2, respCollections] = cc.listCollections(resp.value("/Databases/0/id"_json_pointer, ""));
    // Expect success.
    EXPECT_EQ(200, rc2);
}


/// @brief Tests the listDocuments with a limit of 7 iterations
TEST(CosmosClient, listDocuments)
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

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);

    auto [rc2, respCollections] = cc.listCollections(resp.value("/Databases/0/id"_json_pointer, ""));
    EXPECT_EQ(200, rc2);

    do {
        irt = cc.listDocuments(resp.value("/Databases/0/id"_json_pointer, ""),
                               respCollections.value("/DocumentCollections/0/id"_json_pointer, ""),
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
TEST(CosmosClient, listDocuments_top100)
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

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);

    auto [rc2, respCollections] = cc.listCollections(resp.value("/Databases/0/id"_json_pointer, ""));
    EXPECT_EQ(200, rc2);

    irt = cc.listDocuments(resp.value("/Databases/0/id"_json_pointer, ""),
                           respCollections.value("/DocumentCollections/0/id"_json_pointer, ""),
                           irt.continuationToken);
    EXPECT_EQ(200, irt.statusCode);
    // We check against a collection that has multiple
    totalDocs += irt.document.value<uint32_t>("_count", 0);
    EXPECT_EQ(100, irt.document.value("_count", 0));
    EXPECT_FALSE(irt.continuationToken.empty());
}


/// @brief Test create document API. Removes the document after completion.
/// Requires the environment variables
TEST(CosmosClient, createDocument)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string docId {};
    std::string pkId {};

    // Fail fast if the primary conection string is not present in the build environment
    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    docId          = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId           = "siddiqsoft.com";

    // Now, let us create the document
    auto [rc3, createdDoc] =
            cc.create(dbName, collectionName, {{"id", docId}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}});
    EXPECT_EQ(201, rc3);

    auto rc4 = cc.remove(dbName, collectionName, docId, pkId);
    EXPECT_EQ(204, rc4);
}


/// @brief Test create document with missing "id" field in the document
TEST(CosmosClient, createDocument_MissingId)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string docId {};
    std::string pkId {};


    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // Now, let us create the document
    docId = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId  = "siddiqsoft.com";

    EXPECT_THROW(
            cc.create(dbName, collectionName, {{"MissingId", docId}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}});
            , std::invalid_argument);
}

/// @brief Test create document with missing partition key field in the document
TEST(CosmosClient, createDocument_MissingPkId)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string docId {};
    std::string pkId {};


    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // Now, let us create the document
    docId = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId  = "siddiqsoft.com";

    EXPECT_THROW(
            cc.create(dbName, collectionName, {{"id", docId}, {"ttl", 360}, {"Missing__pk", pkId}, {"source", "basic_tests.exe"}});
            , std::invalid_argument);
}

/// @brief Test find API
TEST(CosmosClient, findDocument)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string docId {};
    std::string pkId {};


    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // Now, let us create the document
    docId = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId  = "siddiqsoft.com";

    auto [rc3, createdDoc] =
            cc.create(dbName, collectionName, {{"id", docId}, {"ttl", 360}, {"__pk", pkId}, {"source", "basic_tests.exe"}});
    EXPECT_EQ(201, rc3);

    // Find the document we just created
    auto [rc4, findDoc] = cc.find(dbName, collectionName, docId, pkId);
    EXPECT_EQ(200, rc4);
    EXPECT_EQ(docId, findDoc.value("id", ""));

    auto rc5 = cc.remove(dbName, collectionName, docId, pkId);
    EXPECT_EQ(204, rc5);
}

/// @brief Test upsert API
TEST(CosmosClient, upsertDocument)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string dbName {};
    std::string collectionName {};
    std::string docId {};
    std::string pkId {};


    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // Now, let us create the document
    docId = std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count());
    pkId  = "siddiqsoft.com";

    // Create the document.. (this should be insert
    auto [rc4, iDoc] =
            cc.upsert(dbName,
                      collectionName,
                      {{"id", docId}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "insert"}, {"source", "basic_tests.exe"}});
    EXPECT_EQ(201, rc4);
    EXPECT_EQ("insert", iDoc.value("upsert", ""));

    // Calling upsert again updates the same document.
    auto [rc5, uDoc] =
            cc.upsert(dbName,
                      collectionName,
                      {{"id", docId}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "update"}, {"source", "basic_tests.exe"}});
    EXPECT_EQ(200, rc5);
    EXPECT_EQ("update", uDoc.value("upsert", ""));

    // And to check if we call create on the same docId it should fail
    auto rc6 = cc.create(dbName,
                         collectionName,
                         {{"id", docId}, {"ttl", 360}, {"__pk", pkId}, {"upsert", "FAIL"}, {"source", "basic_tests.exe"}});
    EXPECT_EQ(409, rc6.statusCode);

    // Remove the document
    auto rc7 = cc.remove(dbName, collectionName, docId, pkId);
    EXPECT_EQ(204, rc7);
}


/// @brief Test query API
TEST(CosmosClient, queryDocument)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string              priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string              secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string              dbName {};
    std::string              collectionName {};
    std::vector<std::string> docIds {};
    std::string              pkId {"siddiqsoft.com"};
    std::string              sourceId = std::format("{}-{}", getpid(), siddiqsoft::CosmosClient::CosmosClientUserAgentString);
    constexpr auto           DOCS {5};

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    siddiqsoft::CosmosClient cc;

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // We're going to create DOCS documents
    for (auto i = 0; i < DOCS; i++) {
        docIds.push_back(std::format("azure-cosmos-restcl.{}", std::chrono::system_clock().now().time_since_epoch().count()));
    }

    // The field "odd" will be used in our query statement
    for (auto i = 0; i < docIds.size(); i++) {
        auto [rc, _] = cc.create(dbName,
                                 collectionName,
                                 {{"id", docIds[i]},
                                  {"ttl", 360},
                                  {"__pk", (i % 2 == 0) ? "even.siddiqsoft.com" : "odd.siddiqsoft.com"},
                                  {"i", i},
                                  {"odd", !(i % 2 == 0)},
                                  {"source", sourceId}});
        EXPECT_EQ(201, rc);
    }

    EXPECT_EQ(DOCS, docIds.size()); // total

    // Wait a little bit..
    std::this_thread::sleep_for(std::chrono::seconds(1));

    siddiqsoft::CosmosIterableResponseType irt {};
    nlohmann::json                         allDocs = nlohmann::json::array();
    uint32_t                               allDocsCount {};

    // First, we query for all items that match our criteria (source=__func__)
    do {
        irt = cc.query(dbName,
                       collectionName,
                       "*",
                       "SELECT * FROM c WHERE contains(c.source, @v1)",
                       {{{"name", "@v1"}, {"value", std::format("{}-", getpid())}}},
                       irt.continuationToken); // the params is an array of name-value items
        EXPECT_EQ(200, irt.statusCode);
        if (200 == irt.statusCode && irt.document.contains("Documents") && !irt.document.at("Documents").is_null()) {
            // Append to the current container
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
            allDocsCount += irt.document.value("_count", 0);
        }
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(DOCS, allDocsCount); // total

#ifdef _DEBUG
    // std::cerr << allDocs.dump(4) << std::endl;
#endif

    auto matchCount = 0;
    for (auto& doc : allDocs) {
        if (!doc.is_null()) {
            auto& docId = doc.at("id");
            std::for_each(docIds.begin(), docIds.end(), [&matchCount, &docId](auto& i) {
                if (i == docId) matchCount++;
            });
        }
    }
    EXPECT_EQ(DOCS, matchCount);

    // Clear stuff..
    irt.continuationToken.clear();
    allDocs      = nlohmann::json::array();
    allDocsCount = 0;
    // Query with partition key "odd"; out of five, 2 should be odd: 1, 3
    do {
        irt = cc.query(dbName,
                       collectionName,
                       "odd.siddiqsoft.com", // __pk
                       "SELECT * FROM c WHERE c.source=@v1",
                       {{{"name", "@v1"}, {"value", sourceId}}});
        EXPECT_EQ(200, irt.statusCode);
        if (irt.document.contains("Documents") && !irt.document.at("Documents").is_null())
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
        allDocsCount += irt.document.value("_count", 0);
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(2, allDocsCount); // odd

    // Query with partition key "odd"; out of five, 3 should be even: 0, 2
    // Clear stuff..
    irt.continuationToken.clear();
    allDocs      = nlohmann::json::array();
    allDocsCount = 0;
    // Query with partition key "odd"; out of five, 2 should be odd: 1, 3
    do {
        irt = cc.query(dbName,
                       collectionName,
                       "even.siddiqsoft.com", // __pk
                       "SELECT * FROM c WHERE c.source=@v1",
                       {{{"name", "@v1"}, {"value", sourceId}}});
        EXPECT_EQ(200, irt.statusCode);
        if (irt.document.contains("Documents") && !irt.document.at("Documents").is_null())
            allDocs.insert(allDocs.end(), irt.document["Documents"].begin(), irt.document["Documents"].end());
        allDocsCount += irt.document.value("_count", 0);
    } while (!irt.continuationToken.empty());
    EXPECT_EQ(3, allDocsCount); // even

    // Wait a little bit..
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Remove the documents
    for (auto i = 0; i < docIds.size(); i++) {
        auto rc = cc.remove(dbName, collectionName, docIds[i], (i % 2 == 0) ? "even.siddiqsoft.com" : "odd.siddiqsoft.com");
        EXPECT_EQ(204, rc);
    }
}


TEST(CosmosClient, createDocument_threads)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    auto                 ttx        = std::chrono::system_clock::now();
    std::string          priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string          secConnStr = std::getenv("CCTEST_SECONDARY_CS");
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
// This code is run when all of the creators have completed
#ifdef _DEBUG
        std::cerr << std::format("!! Barrier hit. DOCS:{} x threadCount:{} -> addDocsCount:{} removeDocsCount:{} ttx:{}!!\n",
                                 DOCS,
                                 threadCount,
                                 addDocsCount.load(),
                                 removeDocsCount.load(),
                                 std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
#endif
    });
    siddiqsoft::CosmosClient cc;

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // Threadpool with workers sync on the barrier; this one creates and searches for "odd" items
    ttx = std::chrono::system_clock::now();
    std::vector<std::jthread> creators;
    for (auto t = 0; t < threadCount; t++) {
        creators.emplace_back(std::jthread(
                [&](auto tid) {
                    // siddiqsoft::CosmosClient ccl;
                    std::vector<std::string> docIds {};
                    // ccl.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}});

                    // std::cerr << std::format(
                    //        "{}..{} starting..flip the latch..ttx:{}\n",
                    //        tid,
                    //        GetCurrentThreadId(),
                    //        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
                    // Wait until we're signalled to start..
                    startLatch.count_down();
                    // std::cerr << std::format(
                    //       "{}..{} waiting..ttx:{}\n",
                    //       tid,
                    //       GetCurrentThreadId(),
                    //       std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
                    startLatch.wait();
                    // std::cerr << std::format(
                    //"{} GO! continue  ttx:{}\n",
                    // tid,
                    // std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
                    // Create the odd documents..
                    for (auto i = 0; i < DOCS; i++) {
                        try {
                            // //std::cerr << std::format("{} Calling create:{:02}...\n", tid, i);
                            auto rc = cc.create(
                                    dbName,
                                    collectionName,
                                    {{"id",
                                      std::format("{}.{}.{}", tid, i, std::chrono::system_clock::now().time_since_epoch().count())},
                                     {"ttl", 360},
                                     {"__pk", "siddiqsoft.com"},
                                     {"i", i},
                                     {"tid", tid},
                                     {"source", sourceId}});
                            addDocsCount += rc.statusCode == 201 ? 1 : 0;
                            docIds.push_back(rc.document.value("id", ""));
                            // //std::cerr << std::format("{} create:{:02}: rc:{} `id`:{}\n",
                            //                         tid,
                            //                         i,
                            //                         rc.statusCode,
                            //                         !rc.document.is_null() ? rc.document.value("id", "") : "-empty-");
                        }
                        catch (const std::exception& e) {
                            // std::cerr << std::format("{:02}: create() exception:{}\n", i, e.what());
                        }
                    }

                    // Wait for until the items are all created..
                    // std::cerr << std::format(
                    //        "{} now waiting on barrier for threads to finish creating..ttx:{}\n",
                    //        tid,
                    //        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ttx));
                    creatorsBarrier.arrive_and_wait();

                    // std::cerr << std::format("{}  Finally..remove {} documents..\n", tid, docIds.size());
                    for (auto i = 0; i < docIds.size(); i++) {
                        auto rc = cc.remove(dbName, collectionName, docIds[i], "siddiqsoft.com");
                        removeDocsCount += rc == 204;
                        // //std::cerr << std::format("{}  Finally..removed {} rc:{}\n", tid, docIds[i], rc);
                    }
                    endLatch.count_down();
                },
                t));
        // std::cerr << std::format("#{} Created  ODD thread {} of {}\n", t, creators.size(), threadCount);
    }


    // We should wait until the barrier completion routine signals that we're done
    endLatch.wait();

    // This code will wait until the barrier has been released by all of the threads!
    // std::cerr << std::format("{} - Continue after threads completed\n", __func__);
    EXPECT_EQ(removeDocsCount.load(), addDocsCount.load());
    EXPECT_EQ((DOCS * threadCount), addDocsCount.load());
}


TEST(CosmosClient, queryDocument_threads)
{
    // These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
    // WARNING!
    // DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
    std::string              priConnStr = std::getenv("CCTEST_PRIMARY_CS");
    std::string              secConnStr = std::getenv("CCTEST_SECONDARY_CS");
    std::string              dbName {};
    std::string              collectionName {};
    std::string              pkId {"siddiqsoft.com"};
    std::string              sourceId = std::format("{}-{}", getpid(), siddiqsoft::CosmosClient::CosmosClientUserAgentString);
    constexpr auto           DOCS {15};
    static auto              threadCount = std::thread::hardware_concurrency();
    std::atomic_uint32_t     createdDocCount {0};
    std::latch               startLatch {threadCount * 2}; // we have two thread classes: even and odd
    std::atomic_uint32_t     evenAddDocsCount {0}, oddAddDocsCount {0};
    std::atomic_uint32_t     evenQueryDocsCount {0}, oddQueryDocsCount {0};
    std::atomic_uint32_t     evenRemoveDocsCount {0}, oddRemoveDocsCount {0};
    std::latch               endLatch {threadCount * 2};
    siddiqsoft::CosmosClient cc; // single instance for all threads!
    std::barrier             creatorsBarrier(threadCount, [&]() noexcept -> void {
// This code is run when all of the creators have completed
#ifdef _DEBUG
        std::cerr << std::format("*** Barrier hit. DOCS:{}   threadCount:{} even:{}-{}-{}  odd:{}-{}-{} ***\n",
                                 DOCS,
                                 threadCount,
                                 evenAddDocsCount.load(),
                                 evenQueryDocsCount.load(),
                                 evenRemoveDocsCount.load(),
                                 oddAddDocsCount.load(),
                                 oddQueryDocsCount.load(),
                                 oddRemoveDocsCount.load());
#endif
    });

    ASSERT_FALSE(priConnStr.empty())
            << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

    EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

    auto [rc, resp] = cc.listDatabases();
    EXPECT_EQ(200, rc);
    dbName                      = resp.value("/Databases/0/id"_json_pointer, "");

    auto [rc2, respCollections] = cc.listCollections(dbName);
    EXPECT_EQ(200, rc2);
    collectionName = respCollections.value("/DocumentCollections/0/id"_json_pointer, "");

    // Threadpool with workers sync on the barrier; this one creates and searches for "odd" items
    std::vector<std::jthread> creatorsOdd;
    for (auto t = 0; t < threadCount; t++) {
        creatorsOdd.emplace_back(std::jthread(
                [&](auto tid) {
                    std::vector<std::string> docIds {};

                    // //std::cerr << std::format("ODD: started; waiting\n");
                    // Wait until we're signalled to start..
                    startLatch.count_down();
                    startLatch.wait();
                    // //std::cerr << std::format("ODD: started; continue\n");
                    // Create the odd documents..
                    for (auto i = 0; i < DOCS; i++) {
                        try {
                            if (!(i % 2 == 0)) {
                                auto rc = cc.create(
                                        dbName,
                                        collectionName,
                                        {{"id",
                                          std::format(
                                                  "{}.{}.{}", t, i, std::chrono::system_clock::now().time_since_epoch().count())},
                                         {"ttl", 360},
                                         {"__pk", "odd.siddiqsoft.com"},
                                         {"i", i},
                                         {"tid", tid},
                                         {"source", sourceId}});
                                if (rc.statusCode == 201) {
                                    createdDocCount++;
                                    docIds.push_back(rc.document.value("id", ""));
                                }
                                else {
                                    std::cerr << std::format("ODD:{:02}: create() failed:{}\n", i, rc);
                                }
                            }
                        }
                        catch (const std::exception& e) {
                            std::cerr << std::format("ODD:{:02}: create() exception:{}\n", i, e.what());
                        }
                    }
                    oddAddDocsCount += docIds.size();

                    // Wait for until the items are all created..
                    creatorsBarrier.arrive_and_wait();

                    // Next we start queries
                    siddiqsoft::CosmosIterableResponseType irt {};
                    do {
                        irt = cc.query(dbName,
                                       collectionName,
                                       "odd.siddiqsoft.com", // __pk
                                       "SELECT * FROM c WHERE c.source=@v1 and c.tid=@v2",
                                       {{{"name", "@v2"}, {"value", tid}}, {{"name", "@v1"}, {"value", sourceId}}});
                        oddQueryDocsCount += irt.document.value("_count", 0);
                    } while (!irt.continuationToken.empty());

                    // Wait until we're completed.
                    creatorsBarrier.arrive_and_wait();

                    for (auto i = 0; i < docIds.size(); i++) {
                        auto rc = cc.remove(dbName, collectionName, docIds[i], "odd.siddiqsoft.com");
                        oddRemoveDocsCount += rc == 204;
                    }

                    // The main thread waits until all of the threads have finished before checking for results.
                    endLatch.count_down();
                },
                t));
        // std::cerr << std::format("#{} Created  ODD thread {} of {}\n", t, creatorsOdd.size(), threadCount);
    }

    // Threadpool with workers sync on the barrier; this one creates and searches for "even" items
    std::vector<std::jthread> creatorsEven;
    for (auto t = 0; t < threadCount; t++) {
        creatorsEven.emplace_back(std::jthread(
                [&](auto tid) {
                    std::vector<std::string> docIds {};

                    // Each thread in the pool signals its ready
                    startLatch.count_down();
                    // Then waits until all peers have arrived at the same stage.
                    startLatch.wait();
                    // Create even documents.. this will "flood"
                    for (auto i = 0; i < DOCS; i++) {
                        if ((i % 2 == 0)) {
                            try {
                                auto rc = cc.create(
                                        dbName,
                                        collectionName,
                                        {{"id",
                                          std::format(
                                                  "{}.{}.{}", t, i, std::chrono::system_clock::now().time_since_epoch().count())},
                                         {"ttl", 360},
                                         {"__pk", "even.siddiqsoft.com"},
                                         {"i", i},
                                         {"tid", tid},
                                         {"source", sourceId}});
                                if (rc.statusCode == 201) {
                                    createdDocCount++;
                                    docIds.push_back(rc.document.value("id", ""));
                                }
                                else {
                                    std::cerr << std::format("EVEN:{:02}: create() failed:{}\n", i, rc);
                                }
                            }
                            catch (const std::exception& e) {
                                std::cerr << std::format("EVEN:{:02}: create() exception:{}\n", i, e.what());
                            }
                        }
                    }
                    evenAddDocsCount += docIds.size();
                    // Wait for until the items are all created..
                    creatorsBarrier.arrive_and_wait();

                    // Next we start queries
                    siddiqsoft::CosmosIterableResponseType irt {};
                    do {
                        irt = cc.query(dbName,
                                       collectionName,
                                       "even.siddiqsoft.com", // __pk
                                       "SELECT * FROM c WHERE c.source=@v1 and c.tid=@v2",
                                       {{{"name", "@v2"}, {"value", tid}}, {{"name", "@v1"}, {"value", sourceId}}});
                        evenQueryDocsCount += irt.document.value("_count", 0);
                    } while (!irt.continuationToken.empty());

                    creatorsBarrier.arrive_and_wait();

                    for (auto i = 0; i < docIds.size(); i++) {
                        auto rc = cc.remove(dbName, collectionName, docIds[i], "even.siddiqsoft.com");
                        evenRemoveDocsCount += (204 == rc);
                    }

                    // Each thread hits this latch (decrementing the counter)
                    endLatch.count_down();
                },
                t));
    }

    // Wait until the latch is set to 0 from threadCount indicating that all threads have successfully completed
    // processing.
    endLatch.wait();

    // This code will wait until the barrier has been released by all of the threads!
    EXPECT_EQ(56, oddAddDocsCount.load());  // total odd
    EXPECT_EQ(64, evenAddDocsCount.load()); // total even
    EXPECT_EQ(DOCS * threadCount, oddAddDocsCount.load() + evenAddDocsCount.load());
    EXPECT_EQ(DOCS * threadCount, oddQueryDocsCount.load() + evenQueryDocsCount.load());
    EXPECT_EQ(DOCS * threadCount, oddRemoveDocsCount.load() + evenRemoveDocsCount.load());
}
