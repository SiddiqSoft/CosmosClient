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
#include "../src/azure-cosmos-restcl.hpp"

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
	std::cerr << "operator<<......" << cs << std::endl;
	std::cerr << "std::format....." << std::format("{}", cs) << std::endl;
	std::cerr << "std::format.Uri." << std::format("{}", cs.BaseUri) << std::endl;
	std::cerr << "string.operator." << std::string(cs.BaseUri) << std::endl;

	EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);
	cs.rotateReadUri();
	EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentReadUri()}.authority.host);

	EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);
	cs.rotateWriteUri();
	EXPECT_EQ("YOURDBNAME.documents.azure.com", ::siddiqsoft::Uri<char> {cs.currentWriteUri()}.authority.host);

	auto info = nlohmann::json(cs);
	EXPECT_EQ(6, info.size());
	std::cerr << "json............" << info.dump(3) << std::endl;
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

	std::cerr << "operator<<......" << cs << std::endl;
	std::cerr << "std::format....." << std::format("{}", cs) << std::endl;
	std::cerr << "std::format.Uri." << std::format("{}", cs.BaseUri) << std::endl;
	std::cerr << "string.operator." << std::string(cs.BaseUri) << std::endl;

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
