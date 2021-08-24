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

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <string>
#include <iostream>
#include <functional>
#include <format>
#include <codecvt>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/restcl_winhttp.hpp"
#include "siddiqsoft/RWLEnvelope.hpp"
#include "siddiqsoft/azure-cpp-utils.hpp"

namespace siddiqsoft
{
#pragma region CosmosConnectionString
	/// @brief Cosmos Connection String as available in the Azure Portal
	/// @tparam CharT Must be char or wchar_t
	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	struct CosmosConnectionString
	{
		std::basic_string<CharT> Uri {};
		std::basic_string<CharT> Key {};

		CosmosConnectionString() { }


		CosmosConnectionString(const std::basic_string<CharT>& s)
		{
			this->operator=(s);
		}


		CosmosConnectionString<CharT>& operator=(const std::basic_string<CharT>& s)
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
					Uri = s.substr(MatchAccountEndpoint.length(), posAccountKey - MatchAccountEndpoint.length());
					Key = s.substr(posAccountKey + MatchAccountKey.length(),
					               s.length() - (posAccountKey + MatchAccountKey.length()));
					// Make sure to strip off the trailing ; if present
					if (Key.ends_with(_NORW(CharT, ";"))) Key.resize(Key.length() - 1);
				}
			}

			return *this;
		}


		operator std::basic_string<CharT>() const
		{
			return string();
		}

		std::basic_string<CharT> string() const
		{
			return std::format(_NORW(CharT, "AccountEndpoint={};AccountKey={};"), Uri, Key);
		}
	};


	/// @brief JSON serializer
	/// @tparam CharT We currently only support char (as the underlying json library does not support wchar_t)
	/// @param dest Destination json object
	/// @param src The source cosmos connection string
	template <typename CharT>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const CosmosConnectionString<CharT>& src)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			dest["uri"] = src.Uri;
			dest["key"] = src.Key;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto converter = std::wstring_convert<std::codecvt_utf8<CharT>> {};
			dest["uri"]    = converter.to_bytes(src.Uri);
			dest["key"]    = converter.to_bytes(src.Key);
		}
	}
#pragma endregion


#pragma region CosmosDatabase
	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	struct CosmosDatabase
	{
		/// @brief Current Connection: 0=Not Set 1=Primary 2=Secondary
		uint16_t CurrentConnectionId {0};

		/// @brief The Primary connection string from the Azure Portal
		CosmosConnectionString<CharT> Primary {};

		/// @brief The Secondary connection string from the Azure Portal
		CosmosConnectionString<CharT> Secondary {};

		/// @brief The Uri to the current connection
		::siddiqsoft::Uri<CharT> Uri {};

		/// @brief The DBName is derived from the current connection string
		std::basic_string<CharT> DBName {};


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
		void configure(const std::basic_string<CharT>& p, const std::basic_string<CharT>& s)
		{
			if (p.empty() && s.empty()) throw std::invalid_argument("Primary or Secondary must be present");

			if (!s.empty()) Secondary = s;
			if (!p.empty()) Primary = p;

			// Reset at the "top"; start with Primary
			rotateConnection(1);
		}

		/// @brief Get the current active connection string
		/// @return Cosmos connection string
		const CosmosConnectionString<CharT>& currentConnection()
		{
			// If the Secondary is selected and non-empty then return Secondary otherwise return Primary.
			return (CurrentConnectionId == 2 && !Secondary.Uri.empty() && !Secondary.Key.empty()) ? Secondary : Primary;
		}

		/// @brief Swaps the current connection by incrementing the current and if we hit past Secondary, we restart at Primary.
		/// @param c Maybe 0=Swap 1=Use Primary 2=Use Secondary
		/// @return Cosmos connection string
		const CosmosConnectionString<CharT>& rotateConnection(const uint16_t c = 0)
		{
			// If c==0 then we increment
			// otherwise accept the given parameter
			CurrentConnectionId = (c == 0) ? ++CurrentConnectionId : c;
			// If CurrentConnectionId exceeds "2" then we roll back to "1".
			// This will "rotate" between Primary and Secondary.
			CurrentConnectionId = CurrentConnectionId > 2 ? 1 : CurrentConnectionId;
			// "Swap" or "rotate"
			switch (CurrentConnectionId) {
				case 1: Uri = Primary.Uri; break;
				case 2:
					// One more check.. if we're asked to use Secondary..
					// but the value is empty..well..
					// we should just not do it..
					if (!Secondary.Uri.empty() && !Secondary.Key.empty()) {
						Uri = Secondary.Uri;
					}
					else {
						// Failed to satisfy non-empty Secondary, use Primary
						Uri                 = Primary.Uri;
						CurrentConnectionId = 1;
					}
					break;
			}

			// Extract the DBName
			if (!Uri.authority.host.empty()) DBName = Uri.authority.host.substr(0, Uri.authority.host.find(_NORW(CharT, ".")));

			return currentConnection();
		}
	};


	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const CosmosDatabase<CharT>& src)
	{
		dest["Uri"]                 = src.Uri;
		dest["CurrentConnectionId"] = src.CurrentConnectionId;
		dest["Primary"]             = src.Primary;
		dest["Secondary"]           = src.Secondary;

		if constexpr (std::is_same_v<CharT, char>) {
			dest["DBName"] = src.DBName;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto converter = std::wstring_convert<std::codecvt_utf8<CharT>> {};

			dest["DBName"] = converter.to_bytes(src.DBName);
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
			dest["Name"] = src.Name;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto converter = std::wstring_convert<std::codecvt_utf8<CharT>> {};

			dest["Name"]   = converter.to_bytes(src.Name);
		}
	}
#pragma endregion


	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	class CosmosClient
	{
	protected:
		nlohmann::json   m_config {{"_typever", "0.1.0"},
                                 {"ApiVersion", "2018-12-31"}, // The API version for Cosmos REST API
                                 {"CustomHeaders", nullptr},   // Optional custom headers
                                 {"AzureRegion", nullptr},     // The preferred Azure Regions requested
                                 {"ConnectionStrings", {{}}},  // The Connection String includes the endpoing and the key
                                 {"UniqueKeys", {{}}},         // Array of unique keys (see your Azure Cosmos configuration)
                                 {"DocumentIdKeyName", "id"},  // This is the default
                                 {"PartitionKeyNames", {{}}},  // The partition key names is an array of partition key names
                                 {"TimeToLive", 0}};           // Default time to live
		std::atomic_bool m_isConfigured {false};

	public:
		CosmosDatabase<CharT> Database {};

	public:
		CosmosClient() = default;

		/// @brief Construct from Cosmos Connection String argument
		/// @param cs Cosmos Connection String
		explicit CosmosClient(const std::basic_string<CharT>& pcs, const std::basic_string<CharT>& scs = {})
		    : Database(pcs, scs)
		{
			m_config["ConnectionStrings"][0] = Database.Primary;
			m_config["ConnectionStrings"][1] = Database.Secondary;
		}


		/// @brief Construct from CosmosDatabase argument
		/// @param db CosmosDatabase argument
		explicit CosmosClient(const CosmosDatabase<CharT>& db)
		    : Database(db)
		{
			m_config["ConnectionStrings"][0] = Database.Primary;
			m_config["ConnectionStrings"][1] = Database.Secondary;
		}


		/// @brief Re-configure the client. This will invoke the discoverRegions to populate the available regions and sets the
		/// primary write/read locations. Avoid repeated invocations!
		/// @param src A valid json object must comply with the defaults
		/// @return The current configuration
		const nlohmann::json& configure(const nlohmann::json& src = {}) noexcept(false)
		{
			if (!src.empty()) {
				m_config.update(src);
				m_isConfigured = true;
			}
			return m_config;
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
		dest["Database"]      = src.Database;
		dest["Configuration"] = src.m_config;
	}

	static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<wchar_t>& src)
	{
		dest["Database"]      = src.Database;
		dest["Configuration"] = src.m_config;
	}

} // namespace siddiqsoft


#pragma region Serializer CosmosConnectionString
/// @brief Serializer for CosmosConnectionString for the char type
/// @tparam CharT Either char or wchar_t
template <typename CharT>
struct std::formatter<siddiqsoft::CosmosConnectionString<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT>
{
	template <class FC>
	auto format(const siddiqsoft::CosmosConnectionString<CharT>& s, FC& ctx)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			auto str = std::format("AccountEndpoint={};AccountKey={};", s.Uri, s.Key);
			return std::formatter<std::basic_string<CharT>, CharT>::format(str, ctx);
		}

		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto str = std::format(L"AccountEndpoint={};AccountKey={};", s.Uri, s.Key);
			return std::formatter<std::basic_string<CharT>, CharT>::format(str, ctx);
		}

		return ctx.out();
	}
};

template <typename CharT>
	requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
static std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const siddiqsoft::CosmosConnectionString<CharT>& s)
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
			auto wstr = std::wstring_convert<std::codecvt_utf8<wchar_t>> {}.from_bytes(str);
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
