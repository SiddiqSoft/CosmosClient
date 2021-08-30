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


TEST(CosmosConnection, test1_n)
{
	std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
	                 ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	siddiqsoft::CosmosConnection cd {cs};

	EXPECT_EQ("AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
	          ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;",
	          std::string(cd.Primary));
	std::cerr << "Primary........." << std::string(cd.Primary) << std::endl;

	EXPECT_EQ("https://YOURDBNAME.documents.azure.com:443/", std::string(cd.Primary.BaseUri));
	std::cerr << "Primary Uri....." << std::string(cd.Primary.BaseUri) << std::endl;

	EXPECT_EQ("U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=", cd.Primary.EncodedKey);
	std::cerr << "Primary Key....." << cd.Primary.EncodedKey << std::endl;

	EXPECT_EQ("YOURDBNAME.documents.azure.com", cd.Primary.BaseUri.authority.host);
	std::cerr << "Primary.host...." << cd.Primary.BaseUri.authority.host << std::endl;

	EXPECT_EQ("YOURDBNAME", cd.current().Name);
	std::cerr << "CosmosName.........." << cd.current().Name << std::endl;

	std::cerr << "Uri............." << std::string(cd.Primary.BaseUri) << std::endl;
}

TEST(CosmosConnection, test2_n)
{
	std::string cs = "AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/"
	                 ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	siddiqsoft::CosmosConnection cd {cs};

	nlohmann::json               info = cd;
	EXPECT_EQ(5, info.size());
	std::cerr << "json............" << info.dump(3) << std::endl;
	std::cerr << "operator<<......" << cd << std::endl;
	std::cerr << "std:format......" << std::format("{}", cd) << std::endl;
}

// TEST(CosmosConnection, test2_w)
//{
//	std::wstring cs =
//	        L"AccountEndpoint=https://YOURDBNAME.documents.azure.com:443/;AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
//	siddiqsoft::CosmosConnection cd {cs};
//
//	// Conversion from wchar_t object to narrow json object
//	nlohmann::json info = cd;
//	EXPECT_EQ(5, info.size());
//	std::cerr << "json............" << info.dump(3) << std::endl;
//	std::wcerr << L"operator<<......" << cd << std::endl;
//	std::wcerr << L"std:format......" << std::format(L"{}", cd) << std::endl;
//}

TEST(CosmosConnection, rotateConnection_1)
{
	std::string pcs = "AccountEndpoint=https://YOURDBNAME-1.documents.azure.com:443/"
	                  ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	std::string scs = "AccountEndpoint=https://YOURDBNAME-2.documents.azure.com:443/"
	                  ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	siddiqsoft::CosmosConnection cd {pcs, scs};

	// We must start at Primary
	std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());

	// Swap to the secondary..
	cd.rotate();
	std::cerr << "1. Should be Secondary." << cd.current() << std::endl;
	EXPECT_EQ(2, cd.CurrentConnectionId);
	EXPECT_EQ(scs, cd.current().string());

	// Swap again.. which should end up at Primary
	cd.rotate();
	std::cerr << "2. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());

	// Force set at 2
	cd.rotate(2);
	std::cerr << "3. Should be Secondary." << cd.current() << std::endl;
	EXPECT_EQ(2, cd.CurrentConnectionId);
	EXPECT_EQ(scs, cd.current().string());

	// Force set at 1
	cd.rotate(1);
	std::cerr << "4. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());
}

TEST(CosmosConnection, rotateConnection_2)
{
	std::string pcs = "AccountEndpoint=https://YOURDBNAME-1.documents.azure.com:443/"
	                  ";AccountKey=U09NRUJBU0U2NEVOQ09ERURLRVlUSEFURU5EU1dJVEhTRU1JQ09MT04=;";
	siddiqsoft::CosmosConnection cd {pcs};

	// We must start at Primary
	std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());

	// Swap to the secondary..
	cd.rotate();
	std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());

	// Swap again.. which should end up at Primary
	cd.rotate();
	std::cerr << "2. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());

	// Force set at 2
	cd.rotate(2);
	std::cerr << "0. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());

	// Force set at 1
	cd.rotate(1);
	std::cerr << "4. Should be Primary..." << cd.current() << std::endl;
	EXPECT_EQ(1, cd.CurrentConnectionId);
	EXPECT_EQ(pcs, cd.current().string());
}
