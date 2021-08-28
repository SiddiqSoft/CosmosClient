/*
    CosmosClient
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
#ifndef COSMOSCLIENT_HPP
#define COSMOSCLIENT_HPP


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
#pragma region CosmosConnection
	/// @brief Cosmos Connection String as available in the Azure Portal
	/// @tparam CharT Must be char or wchar_t
	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	struct CosmosConnection
	{
		/// @brief The "home" or base Uri points to the home location.
		///	For globally partitioned/availability zones, we use this to build the readable/writable
		::siddiqsoft::Uri<CharT> BaseUri {};

		/// @brief The Base64 encoded key
		std::basic_string<CharT> EncodedKey {};

		/// @brief The "binary" key decoded from the EncodedKey stored as std::string
		std::string Key {};

		/// @brief The DBName
		std::basic_string<CharT> DBName {};

		/// @brief Read Locations for the region
		std::vector<::siddiqsoft::Uri<CharT>> ReadableUris {};

		/// @brief Current Read location within the ReadableUris
		size_t CurrentReadUriId {};

		/// @brief Write Locations for the region
		std::vector<::siddiqsoft::Uri<CharT>> WritableUris {};

		/// @brief Current Write Location within the WritableUris
		size_t CurrentWriteUriId {};


		/// @brief Default constructor
		CosmosConnection() = default;

		/// @brief Construct the connection string object from the Connection String obtained from the Azure Portal
		/// @param s Connection String obtained from the Azure Portal
		CosmosConnection(const std::basic_string<CharT>& s)
		{
			this->operator=(s);
		}

		/// @brief Operator assignment
		/// @param s Source string obtained from the Azure portal
		/// @return Self
		CosmosConnection<CharT>& operator=(const std::basic_string<CharT>& s)
		{
			std::basic_string<CharT> MatchAccountEndpoint = _NORW(CharT, "AccountEndpoint=");
			std::basic_string<CharT> MatchAccountKey      = _NORW(CharT, ";AccountKey=");

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
					if (EncodedKey.ends_with(_NORW(CharT, ";"))) EncodedKey.resize(EncodedKey.length() - 1);
					// Store the decoded key (only for std::string)
					if constexpr (std::is_same_v<CharT, char>)
						Key = Base64Utils::decode(EncodedKey);
					else
						Key = Base64Utils::decode(ConversionUtils::asciiFromWide(EncodedKey));
					// Extract the DBName
					if (!BaseUri.authority.host.empty())
						DBName = BaseUri.authority.host.substr(0, BaseUri.authority.host.find(_NORW(CharT, ".")));
				}
			}

			return *this;
		}

		/// @brief Cast operator for string
		operator std::basic_string<CharT>() const
		{
			return string();
		}

		/// @brief Encodes the contents back into the original connection string from Azure Portal
		/// @return The rebuilt connection string must match that of the original connection string from the Azure Portal
		const std::basic_string<CharT> string() const
		{
			return std::format(_NORW(CharT, "AccountEndpoint={};AccountKey={};"), BaseUri, EncodedKey);
		}


		/// @brief Current read Uri
		/// @return Current read Uri or the base Uri
		const ::siddiqsoft::Uri<CharT>& currentReadUri() const
		{
			if (!ReadableUris.empty() && CurrentReadUriId < ReadableUris.size()) return ReadableUris.at(CurrentReadUriId);
			return BaseUri;
		}

		/// @brief Current write Uri
		/// @return Current write Uri or the base Uri
		const ::siddiqsoft::Uri<CharT>& currentWriteUri() const
		{
			if (!WritableUris.empty() && CurrentWriteUriId < WritableUris.size()) return WritableUris.at(CurrentWriteUriId);
			return BaseUri;
		}

		/// @brief Increment the read Uri to the next one in the list and if we reach the end, go back to start
		/// @return Self
		CosmosConnection<CharT>& rotateReadUri()
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
		CosmosConnection<CharT>& rotateWriteUri()
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
	template <typename CharT>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const CosmosConnection<CharT>& src)
	{
		dest["baseUri"]           = src.BaseUri;
		dest["readUris"]          = src.ReadableUris;
		dest["currentReadUriId"]  = src.CurrentReadUriId;
		dest["writeUris"]         = src.WritableUris;
		dest["currentWriteUriId"] = src.CurrentWriteUriId;
		if constexpr (std::is_same_v<CharT, char>) {
			dest["key"]    = src.EncodedKey;
			dest["dbName"] = src.DBName;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			dest["key"]    = ConversionUtils::asciiFromWide(src.EncodedKey);
			dest["dbName"] = ConversionUtils::asciiFromWide(src.DBName);
		}
	}
#pragma endregion


#pragma region CosmosDatabase
	using CosmosResponseType = std::tuple<uint32_t, nlohmann::json>;

	/// @brief Represents the Cosmos Database
	/// @tparam CharT may be char or wchar_t
	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	struct CosmosDatabase
	{
		/// @brief Current Connection: 0=Not Set 1=Primary 2=Secondary
		uint16_t CurrentConnectionId {0};

		/// @brief The Primary connection string from the Azure Portal
		CosmosConnection<CharT> Primary {};

		/// @brief The Secondary connection string from the Azure Portal
		CosmosConnection<CharT> Secondary {};


		CosmosDatabase() = default;

		/// @brief Constructor with Primary and optional Secondary.
		/// @param p Primary Connection String from Azure portal
		/// @param s Secondary Connection String from Azure portal
		CosmosDatabase(const std::basic_string<CharT>& p, const std::basic_string<CharT>& s = {})
		{
			configure(p, s);
		}

		/// @brief Configure the Primary and Secondary. Also resets the current connection to the "Primary"
		/// @param p Primary Connection String from Azure portal
		/// @param s Secondary Connection String from Azure portal
		void configure(const std::basic_string<CharT>& p, const std::basic_string<CharT>& s = {})
		{
			if (p.empty() && s.empty()) throw std::invalid_argument("Primary or Secondary must be present");

			if (!s.empty()) Secondary = s;
			if (!p.empty()) Primary = p;

			// Reset at the "top"; start with Primary
			rotateConnection(1);
		}

		/// @brief Get the current active connection string
		/// @return Cosmos connection string
		CosmosConnection<CharT>& currentConnection()
		{
			// If the Secondary is selected and non-empty then return Secondary otherwise return Primary.
			if (CurrentConnectionId == 2 && !Secondary.Key.empty()) {
				return std::ref(Secondary);
			}
			else {
				// The secondary is empty; force back to primary!
				return std::ref(Primary);
			}
		}

		/// @brief Swaps the current connection by incrementing the current and if we hit past Secondary, we restart at Primary.
		/// @param c Maybe 0=Swap 1=Use Primary 2=Use Secondary
		/// @return Cosmos connection string
		CosmosConnection<CharT>& rotateConnection(const uint16_t c = 0)
		{
			// If c==0 then we increment
			// otherwise accept the given parameter
			CurrentConnectionId = (c == 0) ? ++CurrentConnectionId : c;
			// If CurrentConnectionId exceeds "2" then we roll back to "1".
			// This will "rotate" between Primary and Secondary.
			CurrentConnectionId = CurrentConnectionId > 2 ? 1 : CurrentConnectionId;
			// If the Secondary is empty, then reset back to primary
			if (Secondary.Key.empty()) CurrentConnectionId = 1;

			return currentConnection();
		}
	};


	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const CosmosDatabase<CharT>& src)
	{
		dest["currentConnectionId"] = src.CurrentConnectionId;
		dest["primary"]             = src.Primary;
		dest["secondary"]           = src.Secondary;
		dest["currentConnection"]   = const_cast<CosmosDatabase<CharT>&>(src).currentConnection();

		if constexpr (std::is_same_v<CharT, char>) {
			dest["dBName"] = const_cast<CosmosDatabase<CharT>&>(src).currentConnection().DBName;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			dest["dBName"] = ::siddiqsoft::ConversionUtils::asciiFromWide(
			        const_cast<CosmosDatabase<CharT>&>(src).currentConnection().DBName);
		}
	}
#pragma endregion


#pragma region CosmosCollection
	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	struct CosmosCollection
	{
		std::basic_string<CharT> Name {};
	};


	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const CosmosCollection<CharT>& src)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			dest["name"] = src.Name;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			dest["name"] = ::siddiqsoft::ConversionUtils::asciiFromWide(src.Name);
		}
	}
#pragma endregion


#pragma region CosmosClient
	static const std::string CosmosClientUserAgentString {"SiddiqSoft.CosmosClient/0.1.0"};


	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	class CosmosClient
	{
	protected:
		nlohmann::json   m_config {{"_typever", CosmosClientUserAgentString},
                                 {"apiVersion", "2018-12-31"},   // The API version for Cosmos REST API
                                 {"customHeaders", nullptr},     // Optional custom headers
                                 {"azureRegion", nullptr},       // The preferred Azure Regions requested
                                 {"connectionStrings", nullptr}, // The Connection String from the Azure portal
                                 {"uniqueKeys", nullptr},        // Array of unique keys (see your Azure Cosmos configuration)
                                 {"documentIdKeyName", "id"},    // This is the default
                                 {"partitionKeyNames", nullptr}, // The partition key names is an array of partition key names
                                 {"timeToLive", 0}};             // Default time to live
		std::atomic_bool m_isConfigured {false};

	public:
		WinHttpRESTClient     RestClient {CosmosClientUserAgentString};
		CosmosDatabase<CharT> Database {};

	public:
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
				Database.configure(m_config.value("/connectionStrings/0"_json_pointer, ""),
				                   m_config.value("/connectionStrings/1"_json_pointer, ""));
				// Mark as updated
				m_isConfigured = true;

				// Discover the regions..
				auto [rc, regionInfo] = discoverRegions();
				if (rc == 200 && !regionInfo.empty()) {
					// Read Locations
					if (nlohmann::json readableLocations = regionInfo["readableLocations"]; readableLocations.is_array()) {
						m_config["readableLocations"] = regionInfo["readableLocations"];
					}

					// Write Locations..
					if (nlohmann::json writableLocations = regionInfo["writableLocations"]; writableLocations.is_array()) {
						m_config["writableLocations"] = regionInfo["writableLocations"];
					}
				}
			}

			return m_config;
		}


		CosmosResponseType discoverRegions()
		{
			CosmosResponseType ret {0xFA17, {}};
			auto               ts         = DateUtils::RFC7231();
			auto               currentKey = Database.currentConnection().Key;
			auto               auth       = EncryptionUtils::CosmosToken<char>(currentKey, "GET", "", "", ts);

			RestClient.send(ReqGet(Database.currentConnection().currentReadUri(),
			                       {{"Authorization", auth}, {"x-ms-date", ts}, {"x-ms-version", m_config["apiVersion"]}}),
			                [&ret](const auto& req, const auto& resp) {
				                ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"]) : nlohmann::json {}};
			                });
			return ret;
		}

		// friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>&, const CosmosClient<CharT>&);
		// friend void to_json(nlohmann::json& dest, const CosmosClient<CharT>& src);
		friend void to_json(nlohmann::json& dest, const CosmosClient<char>& src);
		friend void to_json(nlohmann::json& dest, const CosmosClient<wchar_t>& src);
	};

	// template <typename CharT = char>
	//	requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	// static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<CharT>& src)
	//{
	//	dest["Database"]      = src.Database;
	//	dest["Configuration"] = src.m_config;
	//}


	static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<char>& src)
	{
		dest["database"]      = src.Database;
		dest["configuration"] = src.m_config;
	}

	static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<wchar_t>& src)
	{
		dest["catabase"]      = src.Database;
		dest["configuration"] = src.m_config;
	}
#pragma endregion
} // namespace siddiqsoft


#pragma region Serializer CosmosConnection
/// @brief Serializer for CosmosConnection for the char type
/// @tparam CharT Either char or wchar_t
template <typename CharT>
struct std::formatter<siddiqsoft::CosmosConnection<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosConnection<CharT>& s, FC& ctx)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			auto str = std::format("AccountEndpoint={};AccountKey={};", s.BaseUri, s.EncodedKey);
			return std::formatter<std::basic_string<CharT>, CharT>::format(str, ctx);
		}

		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto str = std::format(L"AccountEndpoint={};AccountKey={};", s.BaseUri, s.EncodedKey);
			return std::formatter<std::basic_string<CharT>, CharT>::format(str, ctx);
		}

		return ctx.out();
	}
};

template <typename CharT>
	requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
static std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const siddiqsoft::CosmosConnection<CharT>& s)
{
	os << std::basic_string<CharT>(s);
	return os;
}

#pragma endregion


#pragma region Serializer CosmosDatabase
/// @brief Serializer for CosmosDatabase for the char type
/// @tparam CharT Either char or wchar_t
template <typename CharT>
struct std::formatter<siddiqsoft::CosmosDatabase<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosDatabase<CharT>& s, FC& ctx)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			return std::formatter<std::basic_string<CharT>, CharT>::format(nlohmann::json(s).dump(), ctx);
		}

		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto str  = nlohmann::json(s).dump();
			auto wstr = ::siddiqsoft::ConversionUtils::wideFromAscii(str);
			return std::formatter<std::basic_string<CharT>, CharT>::format(wstr, ctx);
		}

		return ctx.out();
	}
};

/// @brief Output stream write operator
/// @tparam CharT char or wchar_t
/// @param os The output stream
/// @param s The CosmosDatabase object
/// @return The output stream
template <typename CharT>
	requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
static std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const siddiqsoft::CosmosDatabase<CharT>& s)
{
	using namespace siddiqsoft;
	os << std::format(_NORW(CharT, "{}"), s);
	return os;
}

#pragma endregion


#pragma region Serializer CosmosCollection
/// @brief Serializer for CosmosCollection for the char type
/// @tparam CharT Either char or wchar_t
template <typename CharT>
struct std::formatter<siddiqsoft::CosmosCollection<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosCollection<CharT>& s, FC& ctx)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			return std::formatter<std::basic_string<CharT>, CharT>::format(nlohmann::json(s).dump(), ctx);
		}

		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto str  = nlohmann::json(s).dump();
			auto wstr = std::wstring_convert<std::codecvt_utf8<wchar_t>> {}.from_bytes(str);
			return std::formatter<std::basic_string<CharT>, CharT>::format(wstr, ctx);
		}

		return ctx.out();
	}
};

template <typename CharT>
	requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
static std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const siddiqsoft::CosmosCollection<CharT>& s)
{
	using namespace siddiqsoft;
	os << std::format(_NORW(CharT, "{}"), s);
	return os;
}

#pragma endregion


#pragma region Serializer CosmosClient
/// @brief Serializer for CosmosClient for the char type
/// @tparam CharT Either char or wchar_t
template <typename CharT>
struct std::formatter<siddiqsoft::CosmosClient<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosClient<CharT>& s, FC& ctx)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			return std::formatter<std::basic_string<CharT>, CharT>::format(nlohmann::json(s).dump(), ctx);
		}

		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto str  = nlohmann::json(s).dump();
			auto wstr = std::wstring_convert<std::codecvt_utf8<wchar_t>> {}.from_bytes(str);
			return std::formatter<std::basic_string<CharT>, CharT>::format(wstr, ctx);
		}

		return ctx.out();
	}
};


template <typename CharT = char>
	requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
static std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const siddiqsoft::CosmosClient<CharT>& s)
{
	os << nlohmann::json(s).dump();
	return os;
}
#pragma endregion

#endif // !COSMOSCLIENT_HPP
