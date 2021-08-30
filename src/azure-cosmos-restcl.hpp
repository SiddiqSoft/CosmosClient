/*
    Azure Cosmos REST Client
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

#pragma once
#ifndef AZURE_COSMOS_RESTCL_HPP
#define AZURE_COSMOS_RESTCL_HPP


#include <string>
#include <iostream>
#include <functional>
#include <format>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/restcl_winhttp.hpp"
#include "siddiqsoft/RWLEnvelope.hpp"
#include "siddiqsoft/azure-cpp-utils.hpp"


namespace siddiqsoft
{
#pragma region CosmosEndpoint
	/// @brief Cosmos Connection String as available in the Azure Portal
	struct CosmosEndpoint
	{
		/// @brief The "home" or base Uri points to the home location.
		///	For globally partitioned/availability zones, we use this to build the readable/writable
		::siddiqsoft::Uri<char> BaseUri {};

		/// @brief The Base64 encoded key
		std::basic_string<char> EncodedKey {};

		/// @brief The "binary" key decoded from the EncodedKey stored as std::string
		std::string Key {};

		/// @brief The Name
		std::basic_string<char> Name {};

		/// @brief Read Locations for the region
		std::vector<::siddiqsoft::Uri<char>> ReadableUris {};

		/// @brief Current Read location within the ReadableUris
		size_t CurrentReadUriId {};

		/// @brief Write Locations for the region
		std::vector<::siddiqsoft::Uri<char>> WritableUris {};

		/// @brief Current Write Location within the WritableUris
		size_t CurrentWriteUriId {};


		/// @brief Default constructor
		CosmosEndpoint() = default;

		/// @brief Construct the connection string object from the Connection String obtained from the Azure Portal
		/// @param s Connection String obtained from the Azure Portal
		CosmosEndpoint(const std::basic_string<char>& s)
		{
			this->operator=(s);
		}

		/// @brief Operator assignment
		/// @param s Source string obtained from the Azure portal
		/// @return Self
		CosmosEndpoint& operator=(const std::basic_string<char>& s)
		{
			std::basic_string<char> MatchAccountEndpoint = "AccountEndpoint=";
			std::basic_string<char> MatchAccountKey      = ";AccountKey=";

			// The Azure Cosmos Connection string has the following format
			// AccountEndpoint=Uri;AccountKey=Key
			// The Uri is fully qualified name with port
			// The Key is the base64 encoded string representing the Cosmos key
			if (s.starts_with(MatchAccountEndpoint)) {
				if (auto posAccountKey = s.find(MatchAccountKey); posAccountKey != std::string::npos) {
					// We have enough to extract both the uri and the key
					BaseUri    = s.substr(MatchAccountEndpoint.length(), posAccountKey - MatchAccountEndpoint.length());
					EncodedKey = s.substr(posAccountKey + MatchAccountKey.length(),
					                      s.length() - (posAccountKey + MatchAccountKey.length()));
					// Make sure to strip off the trailing ; if present
					if (EncodedKey.ends_with(";")) EncodedKey.resize(EncodedKey.length() - 1);
					// Store the decoded key (only for std::string)
					Key = Base64Utils::decode(EncodedKey);
					// Extract the DBName
					if (!BaseUri.authority.host.empty()) Name = BaseUri.authority.host.substr(0, BaseUri.authority.host.find("."));
				}
			}

			return *this;
		}


		/// @brief Checks if the BaseUri and the EncodedKey is non-empty
		operator bool()
		{
			return !BaseUri.authority.host.empty() && !EncodedKey.empty();
		}


		/// @brief Cast operator for string
		operator std::basic_string<char>() const
		{
			return string();
		}


		/// @brief Encodes the contents back into the original connection string from Azure Portal
		/// @return The rebuilt connection string must match that of the original connection string from the Azure Portal
		const std::basic_string<char> string() const
		{
			return std::format("AccountEndpoint={};AccountKey={};", BaseUri, EncodedKey);
		}


		/// @brief Current read Uri
		/// @return Current read Uri or the base Uri
		const ::siddiqsoft::Uri<char>& currentReadUri() const
		{
			if (!ReadableUris.empty() && CurrentReadUriId < ReadableUris.size()) return ReadableUris.at(CurrentReadUriId);
			return BaseUri;
		}

		/// @brief Current write Uri
		/// @return Current write Uri or the base Uri
		const ::siddiqsoft::Uri<char>& currentWriteUri() const
		{
			if (!WritableUris.empty() && CurrentWriteUriId < WritableUris.size()) return WritableUris.at(CurrentWriteUriId);
			return BaseUri;
		}

		/// @brief Increment the read Uri to the next one in the list and if we reach the end, go back to start
		/// @return Self
		CosmosEndpoint& rotateReadUri()
		{
			if (ReadableUris.empty()) // If empty; use the baseUri
				CurrentReadUriId = 0;
			else if (ReadableUris.size() == 1) // Do not increment if there is only one item
				CurrentReadUriId = 0;
			else if (ReadableUris.size() >= 2) {
				// We have atleast two items
				// Increment..and..Go back to the top if we reach past the last element.
				if (++CurrentReadUriId >= ReadableUris.size()) CurrentReadUriId = 0;
			}

			return *this;
		}

		/// @brief Increment the write Uri to the next one in the list and if we reach the end, go back to start
		/// @return Self
		CosmosEndpoint& rotateWriteUri()
		{
			if (WritableUris.empty()) // If empty, use the baseUri
				CurrentWriteUriId = 0;
			else if (WritableUris.size() == 1) // Do not increment if there is only one item
				CurrentWriteUriId = 0;
			else if (WritableUris.size() >= 2) {
				// We have atleast two items
				// Increment..and..Go back to the top if we reach past the last element.
				if (++CurrentWriteUriId >= WritableUris.size()) CurrentWriteUriId = 0;
			}

			return *this;
		}
	};


	/// @brief JSON serializer
	/// @tparam CharT We currently only support char (as the underlying json library does not support wchar_t)
	/// @param dest Destination json object
	/// @param src The source cosmos connection string
	static void to_json(nlohmann::json& dest, const CosmosEndpoint& src)
	{
		dest["baseUri"]           = src.BaseUri;
		dest["readUris"]          = src.ReadableUris;
		dest["currentReadUriId"]  = src.CurrentReadUriId;
		dest["writeUris"]         = src.WritableUris;
		dest["currentWriteUriId"] = src.CurrentWriteUriId;
		dest["key"]               = src.EncodedKey;
		dest["name"]              = src.Name;
	}
#pragma endregion


#pragma region CosmosConnection
	using CosmosResponseType = std::tuple<uint32_t, nlohmann::json>;

	/// @brief Represents the Cosmos Cnxn
	struct CosmosConnection
	{
		/// @brief Current Connection: 0=Not Set 1=Primary 2=Secondary
		uint16_t CurrentConnectionId {0};

		/// @brief The Primary connection string from the Azure Portal
		CosmosEndpoint Primary {};

		/// @brief The Secondary connection string from the Azure Portal
		CosmosEndpoint Secondary {};

		/// @brief Default constructor
		CosmosConnection() = default;

		/// @brief Constructor with Primary and optional Secondary.
		/// @param p Primary Connection String from Azure portal
		/// @param s Secondary Connection String from Azure portal
		CosmosConnection(const std::basic_string<char>& p, const std::basic_string<char>& s = {})
		{
			configure({{"connectionStrings", {p, s}}});
		}

		/// @brief Configure the Primary and Secondary. Also resets the current connection to the "Primary"
		/// @param p Primary Connection String from Azure portal
		/// @param s Secondary Connection String from Azure portal
		/// @return Self
		CosmosConnection& configure(const nlohmann::json& config)
		{
			Primary   = config.value("/connectionStrings/0"_json_pointer, "");
			Secondary = config.value("/connectionStrings/1"_json_pointer, "");

			if (!Primary) throw std::invalid_argument("Primary must be present");

			// If we have readLocations then load them up for the current connection
			// If we "switch" we will repopulate the Primary or Secondary as current
			if (current()) {
				if (config.contains("/serviceSettings/readableLocations"_json_pointer)) {
					for (auto& item : config.at("/serviceSettings/readableLocations"_json_pointer)) {
						current().ReadableUris.push_back(item.value("databaseAccountEndpoint", ""));
					}
				}
				if (config.contains("/serviceSettings/writableLocations"_json_pointer)) {
					for (auto& item : config.at("/serviceSettings/writableLocations"_json_pointer)) {
						current().WritableUris.push_back(item.value("databaseAccountEndpoint", ""));
					}
				}
			}

			// Reset at the "top"; start with Primary
			rotate(1);
			return *this;
		}

		/// @brief Get the current active connection string
		/// @return Cosmos connection string
		/// @return Reference to the current active Connection Primary/Secondary
		CosmosEndpoint& current()
		{
			// If the Secondary is selected and non-empty then return Secondary otherwise return Primary.
			if (CurrentConnectionId == 2 && !Secondary.Key.empty()) {
				return std::ref(Secondary);
			}

			// The secondary is empty; force back to primary!
			return std::ref(Primary);
		}

		/// @brief Swaps the current connection by incrementing the current and if we hit past Secondary, we restart at Primary.
		/// @param c Maybe 0=Swap 1=Use Primary 2=Use Secondary
		/// @return Self
		CosmosConnection& rotate(const uint16_t c = 0)
		{
			// If c==0 then we increment
			// otherwise accept the given parameter
			CurrentConnectionId = (c == 0) ? ++CurrentConnectionId : c;
			// If CurrentConnectionId exceeds "2" then we roll back to "1".
			// This will "rotate" between Primary and Secondary.
			CurrentConnectionId = CurrentConnectionId > 2 ? 1 : CurrentConnectionId;
			// If the Secondary is empty, then reset back to primary
			if (Secondary.Key.empty()) CurrentConnectionId = 1;

			return *this;
		}
	};


	static void to_json(nlohmann::json& dest, const CosmosConnection& src)
	{
		dest["currentConnectionId"] = src.CurrentConnectionId;
		dest["primary"]             = src.Primary;
		dest["secondary"]           = src.Secondary;
		dest["currentConnection"]   = const_cast<CosmosConnection&>(src).current();
		dest["name"]                = const_cast<CosmosConnection&>(src).current().Name;
	}
#pragma endregion


#pragma region CosmosClient
	static const std::string CosmosClientUserAgentString {"SiddiqSoft.CosmosClient/0.1.0"};

	/// @brief Cosmos Client
	class CosmosClient
	{
	protected:
		nlohmann::json   m_config {{"_typever", CosmosClientUserAgentString},
                                 {"apiVersion", "2018-12-31"},   // The API version for Cosmos REST API
                                 {"customHeaders", nullptr},     // Optional custom headers
                                 {"azureRegion", nullptr},       // The preferred Azure Regions requested
                                 {"serviceSettings", nullptr},   // Populated with response from call to REST API discover regions
                                 {"connectionStrings", nullptr}, // The Connection String from the Azure portal
                                 {"uniqueKeys", nullptr},        // Array of unique keys (see your Azure Cosmos configuration)
                                 {"documentIdKeyName", "id"},    // This is the default
                                 {"partitionKeyNames", nullptr}, // The partition key names is an array of partition key names
                                 {"timeToLive", 0}};             // Default time to live
		std::atomic_bool m_isConfigured {false};

	public:
		WinHttpRESTClient RestClient {CosmosClientUserAgentString};
		CosmosConnection  Cnxn {};

		friend void       to_json(nlohmann::json& dest, const CosmosClient& src);

	public:
		/// @brief Default constructor
		CosmosClient() = default;


		/// @brief Re-configure the client. This will invoke the discoverRegions to populate the available regions and sets the
		/// primary write/read locations. If the source is empty (no arguments) then the current configuration is returned.
		/// Avoid repeated invocations!
		/// @param src A valid json object must comply with the defaults. If empty, the current configuration is returned without
		/// any changes.
		/// @return The current configuration
		const nlohmann::json& configure(const nlohmann::json& src = {}) noexcept(false)
		{
			if (!src.empty()) {
				// The minimum is that the ConnectionStrings exist with at least one element; a string from the Azure portal with
				// the Primary Connection String.
				if (!src.contains("connectionStrings")) throw std::invalid_argument("connectionStrings missing");

				// Update our local configuration
				m_config.update(src);

				// We continue configuring..
				// After the update we must ensure that the requirements are still met: ConnectionStrings array
				if (!m_config["connectionStrings"].is_array()) throw std::invalid_argument("connectionStrings must be array");
				if (m_config["connectionStrings"].size() < 1)
					throw std::invalid_argument("connectionStrings array must contain atleast primary element");

				// Update the database configuration
				Cnxn.configure(m_config);
				// Mark as updated
				m_isConfigured = true;

				// Discover the regions..
				auto [rc, regionInfo] = discoverRegions();
				if (rc == 200 && !regionInfo.empty()) {
#ifdef _DEBUG
					std::cerr << "Response from discoverRegions()\n" << regionInfo.dump(3) << std::endl;
#endif //  _DEBUG
					m_config["serviceSettings"] = regionInfo;
					// Reconfigure/update the information such as the read location
					Cnxn.configure(m_config);
				}
			}

			return m_config;
		}


		/// @brief Discover the Regions for the current base Uri
		/// @return Tuple of the status code and the json response (or empty)
		CosmosResponseType discoverRegions()
		{
			CosmosResponseType ret {0xFA17, {}};
			auto               ts   = DateUtils::RFC7231();
			auto               auth = EncryptionUtils::CosmosToken<char>(Cnxn.current().Key, "GET", "", "", ts);

			RestClient.send(ReqGet(Cnxn.current().currentReadUri(),
			                       {{"Authorization", auth}, {"x-ms-date", ts}, {"x-ms-version", m_config["apiVersion"]}}),
			                [&ret](const auto& req, const auto& resp) {
				                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			                });
			return std::move(ret);
		}


		//
		// listDatabases
		// Refer to https://docs.microsoft.com/en-us/rest/api/documentdb/documentdb-resource-uri-syntax-for-rest
		//

		/// @brief
		/// @return
		CosmosResponseType listDatabases()
		{
			CosmosResponseType ret {0xFA17, {}};

			// We need to add the same value to the header in the field x-ms-date as well as the Authorization field
			auto ts   = DateUtils::RFC7231();
			auto auth = EncryptionUtils::CosmosToken<char>(Cnxn.current().Key, "GET", "dbs", "", ts);
			auto path = Cnxn.current().currentReadUri().string() + "dbs";

			RestClient.send(ReqGet(path,                                        // the url
			                       {{"Authorization", auth},                    // headers and for this op, no content
			                        {"x-ms-date", ts},                          // timestamp
			                        {"x-ms-version", m_config["apiVersion"]}}), // api version
			                [&ret](const auto& req, const auto& resp) {
				                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			                });

			return std::move(ret);
		}

		/// @brief
		/// @param dbName
		/// @return
		CosmosResponseType listCollections(const std::string& dbName)
		{
			CosmosResponseType ret {0xFA17, {}};

			// We need to add the same value to the header in the field x-ms-date as well as the Authorization field
			auto ts   = DateUtils::RFC7231();
			auto path = std::format("{}dbs/{}/colls", Cnxn.current().currentReadUri().string(), dbName);
			auto auth = EncryptionUtils::CosmosToken<char>(Cnxn.current().Key, "GET", "colls", {"dbs/" + dbName}, ts);

			RestClient.send(ReqGet(path,                                        // the url
			                       {{"Authorization", auth},                    // headers and for this op, no content
			                        {"x-ms-date", ts},                          // timestamp
			                        {"x-ms-version", m_config["apiVersion"]}}), // api version
			                [&ret](const auto& req, const auto& resp) {
				                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			                });

			return std::move(ret);
		}

		/// @brief
		/// @param dbName
		/// @param collName
		/// @return
		CosmosResponseType listDocuments(const std::string& dbName, const std::string& collName)
		{
			CosmosResponseType ret {0xFA17, {}};
			auto               ts = DateUtils::RFC7231();
			auto path = std::format("{}dbs/{}/colls/{}/docs", Cnxn.current().currentReadUri().string(), dbName, collName);
			auto auth = EncryptionUtils::CosmosToken<char>(
			        Cnxn.current().Key, "GET", "docs", std::format("dbs/{}/colls/{}", dbName, collName), ts);

			RestClient.send(ReqGet(path,                                        // the url
			                       {{"Authorization", auth},                    // headers and for this op, no content
			                        {"x-ms-date", ts},                          // timestamp
			                        {"x-ms-version", m_config["apiVersion"]}}), // api version
			                [&ret](const auto& req, const auto& resp) {
				                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			                });

			return std::move(ret);
		}


		/// @brief
		/// opCreate an entity in documentdb using the doc nlohmann::json object as the payload.
		/// See example
		/// https://docs.microsoft.com/en-us/rest/api/documentdb/create-a-document
		/// @param dbName
		/// @param collName
		/// @param doc
		/// @return
		CosmosResponseType create(const std::string& dbName, const std::string& collName, const nlohmann::json& doc)
		{
			if (doc.value("id", "").empty()) throw std::invalid_argument("remove - I need the uniqueid of the document");
			if (doc.value(m_config.at("/partitionKeyNames/0"_json_pointer), "").empty())
				throw std::invalid_argument("remove - I need the partitionId of the document");

			CosmosResponseType ret {0xFA17, {}};
			auto               ts   = DateUtils::RFC7231();
			auto               pkId = doc.value(m_config.at("/partitionKeyNames/0"_json_pointer), "");

			RestClient.send(
			        siddiqsoft::ReqPost {
			                std::format("{}dbs/{}/colls/{}/docs", Cnxn.current().currentWriteUri().string(), dbName, collName),
			                {{"Authorization",
			                  EncryptionUtils::CosmosToken<char>(
			                          Cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", dbName, collName), ts)},
			                 {"x-ms-date", ts},
			                 {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
			                 {"x-ms-version", m_config["apiVersion"]},
			                 {"x-ms-cosmos-allow-tentative-writes", "true"}},
			                doc},
			        [&ret](const auto& req, const auto& resp) {
				        ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			        });

			return std::move(ret);
		}


		/// @brief Removes the document given the docId and pkId
		/// https://docs.microsoft.com/en-us/rest/api/documentdb/delete-a-document
		/// @param dbName Cosmos Database name
		/// @param collName Cosmos Collection Name
		/// @param docId Unique document Id
		/// @param pkId The partition Id
		/// @return Status code
		CosmosResponseType
		remove(const std::string& dbName, const std::string& collName, const std::string& docId, const std::string& pkId)
		{
			using namespace siddiqsoft;

			CosmosResponseType ret {0xFA17, {}};
			auto               ts = DateUtils::RFC7231();

			if (docId.empty()) throw std::invalid_argument("remove - I need the docId of the document");
			if (pkId.empty()) throw std::invalid_argument("remove - I need the pkId of the document");


			// {"x-ms-documentdb-partitionkey", std::format("[\"{}\"]", partitionId)},

			RestClient.send(siddiqsoft::ReqDelete {std::format("{}dbs/{}/colls/{}/docs/{}",
			                                                   Cnxn.current().currentWriteUri().string(),
			                                                   dbName,
			                                                   collName,
			                                                   docId),
			                                       {{"Authorization",
			                                         EncryptionUtils::CosmosToken<char>(
			                                                 Cnxn.current().Key,
			                                                 "DELETE",
			                                                 "docs",
			                                                 std::format("dbs/{}/colls/{}/docs/{}", dbName, collName, docId),
			                                                 ts)},
			                                        {"x-ms-date", ts},
			                                        {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
			                                        {"x-ms-version", m_config["apiVersion"]},
			                                        {"x-ms-cosmos-allow-tentative-writes", "true"}}},
			                [&ret](const auto& req, const auto& resp) {
				                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			                });

			return std::move(ret);
		}
	};

	/// @brief
	/// @param dest
	/// @param src
	static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient& src)
	{
		dest["database"]      = src.Cnxn;
		dest["configuration"] = src.m_config;
	}
#pragma endregion
} // namespace siddiqsoft


#pragma region Serializer CosmosEndpoint
/// @brief Serializer for CosmosEndpoint for the char type
/// @tparam CharT Either char or wchar_t
template <>
struct std::formatter<siddiqsoft::CosmosEndpoint> : std::formatter<std::basic_string<char>>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosEndpoint& s, FC& ctx)
	{
		auto str = std::format("AccountEndpoint={};AccountKey={};", s.BaseUri, s.EncodedKey);
		return std::formatter<std::basic_string<char>>::format(str, ctx);
	}
};

static std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosEndpoint& s)
{
	os << std::basic_string<char>(s);
	return os;
}

#pragma endregion


#pragma region Serializer CosmosConnection
/// @brief Serializer for CosmosConnection for the char type
/// @tparam CharT Either char or wchar_t
template <>
struct std::formatter<siddiqsoft::CosmosConnection> : std::formatter<std::basic_string<char>>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosConnection& s, FC& ctx)
	{
		return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
	}
};

/// @brief Output stream write operator
/// @tparam CharT char or wchar_t
/// @param os The output stream
/// @param s The CosmosConnection object
/// @return The output stream
static std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosConnection& s)
{
	os << std::format("{}", s);
	return os;
}

#pragma endregion


#pragma region Serializer CosmosClient
/// @brief Serializer for CosmosClient for the char type
/// @tparam CharT Either char or wchar_t
template <>
struct std::formatter<siddiqsoft::CosmosClient> : std::formatter<std::basic_string<char>>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosClient& s, FC& ctx)
	{
		return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
	}
};


static std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosClient& s)
{
	os << nlohmann::json(s).dump();
	return os;
}
#pragma endregion

#endif // !AZURE_COSMOS_RESTCL_HPP
