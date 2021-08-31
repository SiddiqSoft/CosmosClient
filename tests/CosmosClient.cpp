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
#include <functional>
#include <chrono>

#include "nlohmann/json.hpp"
#include "../src/azure-cosmos-restcl.hpp"

/*
 * Required Environment Variables
 * CCTEST_PRIMARY_CS
 *
 * Optional Environment Variables
 * CCTEST_SECONDARY_CS
 */


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
				auto [rc4, delDoc] = cc.remove(dbName, collectionName, cDoc.value("id", docId), pkId);
				EXPECT_EQ(204, rc4);
			}
		}
	}
}


TEST(CosmosClient, configure_Defaults)
{
	siddiqsoft::CosmosClient cc;

	// Check that we have read/write locations detected.
	auto currentConfig = cc.configuration();

	EXPECT_TRUE(currentConfig.contains("apiVersion"));
	// This forces us to make sure we check side-effects whenever the version changes!
	EXPECT_EQ("2018-12-31", currentConfig.value("apiVersion", ""));
	EXPECT_TRUE(currentConfig.contains("connectionStrings"));
	EXPECT_TRUE(currentConfig.contains("uniqueKeys"));
	EXPECT_TRUE(currentConfig.contains("documentIdKeyName"));
	EXPECT_EQ("id", currentConfig.value("documentIdKeyName", ""));
	EXPECT_TRUE(currentConfig.contains("partitionKeyNames"));
}

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

	nlohmann::json info = cc;
	EXPECT_TRUE(info.contains("serviceSettings"));
	EXPECT_TRUE(info.contains("database"));
	EXPECT_TRUE(info.contains("configuration"));
	EXPECT_EQ(3, info.size());

	// Check that we have read/write locations detected.
	auto currentConfig = cc.configuration();

	EXPECT_TRUE(cc.serviceSettings["writableLocations"].is_array());
	EXPECT_TRUE(cc.serviceSettings["readableLocations"].is_array());

	// Atleast one read location
	EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
	EXPECT_LE(1, cc.Cnxn.current().ReadableUris.size());
	// Atleast one write location
	EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
	EXPECT_LE(1, cc.Cnxn.current().WritableUris.size());
}


TEST(CosmosClient, discoverRegion)
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

	nlohmann::json info = cc;
	EXPECT_TRUE(info.contains("serviceSettings"));
	EXPECT_TRUE(info.contains("database"));
	EXPECT_TRUE(info.contains("configuration"));
	EXPECT_EQ(3, info.size());

	// Check that we have read/write locations detected.
	// Atleast one read location
	EXPECT_LE(1, cc.serviceSettings["readableLocations"].size());
	EXPECT_LE(1, cc.Cnxn.current().ReadableUris.size());
	// Atleast one write location
	EXPECT_LE(1, cc.serviceSettings["writableLocations"].size());
	EXPECT_LE(1, cc.Cnxn.current().WritableUris.size());
}


TEST(CosmosClient, discoverRegion_BadPrimary)
{
	// These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
	// WARNING!
	// DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
	std::string priConnStr = "AccountEndpoint=https://localhost:4043/"
	                         ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

	ASSERT_FALSE(priConnStr.empty())
	        << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

	siddiqsoft::CosmosClient cc;

	EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

	nlohmann::json info = cc;
	EXPECT_TRUE(info.contains("serviceSettings"));
	EXPECT_TRUE(info.contains("database"));
	EXPECT_TRUE(info.contains("configuration"));
	EXPECT_EQ(3, info.size());

	auto [rc, resp] = cc.discoverRegions();
	// Expect failure.
	EXPECT_NE(200, rc);

	cc.Cnxn.rotate();
	std::tie(rc, resp) = cc.discoverRegions();
	// Expect success.
	EXPECT_EQ(200, rc);

	// Rotate..
	cc.Cnxn.rotate();
	// Try again.. we should fail again!
	std::tie(rc, resp) = cc.discoverRegions();
	EXPECT_NE(200, rc);
}


TEST(CosmosClient, listDatabases)
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

	ASSERT_FALSE(priConnStr.empty())
	        << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

	siddiqsoft::CosmosClient cc;

	EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

	auto [rc, resp] = cc.listDatabases();
	// Expect success.
	EXPECT_EQ(200, rc);

	auto [rc2, respCollections] = cc.listCollections(resp.value("/Databases/0/id"_json_pointer, ""));
	// Expect success.
	EXPECT_EQ(200, rc2);
}


TEST(CosmosClient, listDocuments)
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

	auto [rc, resp] = cc.listDatabases();
	EXPECT_EQ(200, rc);

	auto [rc2, respCollections] = cc.listCollections(resp.value("/Databases/0/id"_json_pointer, ""));
	EXPECT_EQ(200, rc2);

	auto [rc3, respDocuments] = cc.listDocuments(resp.value("/Databases/0/id"_json_pointer, ""),
	                                             respCollections.value("/DocumentCollections/0/id"_json_pointer, ""));
	EXPECT_EQ(200, rc3);
}


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

	auto [rc4, delDoc] = cc.remove(dbName, collectionName, docId, pkId);
	EXPECT_EQ(204, rc4);
}


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

	auto [rc5, delDoc] = cc.remove(dbName, collectionName, docId, pkId);
	EXPECT_EQ(204, rc5);
}


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
	EXPECT_EQ(409, std::get<0>(rc6));

	// Remove the document
	auto rc7 = cc.remove(dbName, collectionName, docId, pkId);
	EXPECT_EQ(204, std::get<0>(rc7));
}


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
		std::cerr << "**** Created: " << docIds[i] << "--" << sourceId << std::endl;
	}

	EXPECT_EQ(DOCS, docIds.size()); // total

	// Wait a little bit..
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// First, we query for all items that match our criteria (source=__func__)
	auto [rq1, qDoc1] =
	        cc.query(dbName,
	                 collectionName,
	                 "*",
	                 "SELECT * FROM c WHERE contains(c.source, @v1)",
	                 {{{"name", "@v1"}, {"value", std::format("{}-", getpid())}}}); // the params is an array of name-value items
	// auto [rq1, qDoc1] = cc.query(dbName,
	//                             collectionName,
	//                             "*", // __pk = '*'
	//                             std::format("SELECT * FROM c WHERE contains(c.source, '{}-')", getpid()));
	EXPECT_EQ(200, rq1);
	EXPECT_EQ(DOCS, qDoc1.value("_count", 0)); // total
	std::cerr << qDoc1.dump(3) << std::endl;

	auto matchCount = 0;
	for (auto& doc : qDoc1["Documents"]) {
		auto& docId = doc.at("id");
		std::cerr << "Matched " << 1 + matchCount << " from query docId:" << doc.value("id", "") << std::endl;
		std::for_each(docIds.begin(), docIds.end(), [&matchCount, &docId](auto& i) {
			if (i == docId) matchCount++;
		});
	}
	EXPECT_EQ(DOCS, matchCount);

	// Query with partition key "odd"; out of five, 2 should be odd: 1, 3
	auto [rq2, qDoc2] = cc.query(dbName,
	                             collectionName,
	                             "odd.siddiqsoft.com", // __pk
	                             "SELECT * FROM c WHERE c.source=@v1",
	                             {{{"name", "@v1"}, {"value", sourceId}}});
	EXPECT_EQ(200, rq2);
	EXPECT_EQ(2, qDoc2.value("_count", 0)); // odd

	// Query with partition key "odd"; out of five, 3 should be even: 0, 2
	auto [rq3, qDoc3] = cc.query(dbName,
	                             collectionName,
	                             "even.siddiqsoft.com", // __pk
	                             "SELECT * FROM c WHERE c.source=@v1",
	                             {{{"name", "@v1"}, {"value", sourceId}}});
	EXPECT_EQ(200, rq3);
	EXPECT_EQ(3, qDoc3.value("_count", 0)); // even

	// Wait a little bit..
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Remove the documents
	for (auto i = 0; i < docIds.size(); i++) {
		auto [rc, _] = cc.remove(dbName, collectionName, docIds[i], (i % 2 == 0) ? "even.siddiqsoft.com" : "odd.siddiqsoft.com");
		EXPECT_EQ(204, rc);
	}
}
