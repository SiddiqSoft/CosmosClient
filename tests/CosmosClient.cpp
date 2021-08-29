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

#include "nlohmann/json.hpp"
#include "../src/CosmosClient.hpp"

/*
 * Required Environment Variables
 * CCTEST_PRIMARY_CS
 *
 * Optional Environment Variables
 * CCTEST_SECONDARY_CS
 */

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
	EXPECT_EQ(2, info.size());

	// Check that we have read/write locations detected.
	auto currentConfig = cc.configure();

	EXPECT_TRUE(currentConfig["serviceSettings"]["writableLocations"].is_array());
	EXPECT_TRUE(currentConfig["serviceSettings"]["readableLocations"].is_array());

	// Atleast one read location
	EXPECT_LE(1, currentConfig["serviceSettings"]["readableLocations"].size());
	EXPECT_LE(1, cc.Database.currentConnection().ReadableUris.size());
	// Atleast one write location
	EXPECT_LE(1, currentConfig["serviceSettings"]["writableLocations"].size());
	EXPECT_LE(1, cc.Database.currentConnection().WritableUris.size());
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
	EXPECT_EQ(2, info.size());

	// Check that we have read/write locations detected.
	// Atleast one read location
	EXPECT_LE(1, cc.configure()["serviceSettings"]["readableLocations"].size());
	EXPECT_LE(1, cc.Database.currentConnection().ReadableUris.size());
	// Atleast one write location
	EXPECT_LE(1, cc.configure()["serviceSettings"]["writableLocations"].size());
	EXPECT_LE(1, cc.Database.currentConnection().WritableUris.size());
}


TEST(CosmosClient, discoverRegion_BadPrimary)
{
	// These are pulled from Azure Pipelines mapped as secret variables into the following environment variables.
	// WARNING!
	// DO NOT DISPLAY the contents as they will expose the secrets in the Azure pipeline logs!
	std::string priConnStr = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
	                         ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	std::string secConnStr = std::getenv("CCTEST_SECONDARY_CS");

	ASSERT_FALSE(priConnStr.empty())
	        << "Missing environment variable CCTEST_PRIMARY_CS; Set it to Primary Connection string from Azure portal.";

	siddiqsoft::CosmosClient cc;

	EXPECT_NO_THROW(cc.configure({{"partitionKeyNames", {"__pk"}}, {"connectionStrings", {priConnStr, secConnStr}}}));

	nlohmann::json info = cc;
	EXPECT_EQ(2, info.size());

	auto [rc, resp] = cc.discoverRegions();
	// Expect failure.
	EXPECT_EQ(12007, rc);

	cc.Database.rotateConnection();
	std::tie(rc, resp) = cc.discoverRegions();
	// Expect success.
	EXPECT_EQ(200, rc);

	// Rotate..
	cc.Database.rotateConnection();
	// Try again.. we should fail again!
	std::tie(rc, resp) = cc.discoverRegions();
	EXPECT_EQ(12007, rc);
}
