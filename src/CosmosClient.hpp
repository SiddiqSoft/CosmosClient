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
			return std::format(_NORW(CharT, "AccountEndpoint={};AccountKey={};"), Uri, Key);
		}

		/// @brief Create the Cosmos Authorization Token using the Key for this connection
		/// @param date Date in RFC7231 as string
		/// @param verb GET, POST, PUT, DELETE
		/// @param type One of the following: dbs, docs, colls, attachments or empty
		/// @param resourceLink The resource link sub-uri
		/// @return Cosmos Authorization signature as std::string
		//std::string
		//signature(const std::string& date, const std::string& verb, const std::string& type, const std::string& resourceLink)
		//{
		//	auto urlEncode = [](const std::string& s) -> std::string {
		//		std::string retOutput {};
		//		std::for_each(s.begin(), s.end(), [&](auto ch) {
		//			switch (ch) {
		//				case '%': retOutput += "%25"; break;
		//				case ' ': retOutput += "%20"; break;
		//				case '&': retOutput += "%26"; break;
		//				case '<': retOutput += "%3c"; break;
		//				case '>': retOutput += "%3e"; break;
		//				case '{': retOutput += "%7b"; break;
		//				case '}': retOutput += "%7d"; break;
		//				case '\'': retOutput += "%27"; break;
		//				case '\"': retOutput += "%22"; break;
		//				case '/': retOutput += "%2f"; break;
		//				case '\\': retOutput += "%5c"; break;
		//				case '@': retOutput += "%40"; break;
		//				case '~': retOutput += "%7e"; break;
		//				case '|': retOutput += "%7c"; break;
		//				case ',': retOutput += "%2c"; break;
		//				case '+': retOutput += "%2b"; break;
		//				case ':': retOutput += "%3a"; break;
		//				case '`': retOutput += "%60"; break;
		//				case '[': retOutput += "%5b"; break;
		//				case ']': retOutput += "%5d"; break;
		//				case '?': retOutput += "%3f"; break;
		//				case '=': retOutput += "%3d"; break;
		//				case '$': retOutput += "%24"; break;
		//				case '#': retOutput += "%23"; break;
		//				default: retOutput += ch;
		//			};
		//		});
		//		return retOutput;
		//	};
		//
		//	if (verb.empty() || date.empty()) throw std::invalid_argument("Verb and date must not be empty!");
		//
		//	// Assignment is critical to ensure that the destinations are properly sized and terminated!
		//	// This one assignment allows us to play with the values without casting the const out.
		//	std::string myVerb(verb), myType(type), myDate(date);
		//
		//	// Ensure the items are lower-case
		//	std::transform(verb.begin(), verb.end(), myVerb.begin(), std::tolower);
		//	std::transform(type.begin(), type.end(), myType.begin(), std::tolower);
		//	std::transform(date.begin(), date.end(), myDate.begin(), std::tolower);
		//
		//	// The formula is expressed as per
		//	// https://docs.microsoft.com/en-us/rest/api/documentdb/access-control-on-documentdb-resources?redirectedfrom=MSDN
		//	std::string strToHash = std::format("{}\n{}\n{}\n{}\n\n", myVerb, myType, resourceLink, myDate);
		//	// Sign using SHA256 using the master key and base64 encode
		//	std::string hmac = base64encode(createHMAC(strToHash, Key));
		//	// Return the encoded signature as follows:
		//	auto ret = std::format("type%3dmaster%26ver%3d1.0%26sig%3d{}", urlEncode(hmac));
		//
		//	return std::move(ret);
		//}
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
		::siddiqsoft::Uri<CharT>      Uri {};
		CosmosConnectionString<CharT> Primary {};
		CosmosConnectionString<CharT> Secondary {};
		std::basic_string<CharT>      DBName {};

		CosmosDatabase(const std::basic_string<CharT>& p)
		    : Primary(p)
		{
			if (!Primary.Uri.empty()) Uri = Primary.Uri;
			// Extract the DBName from the convention where the first portion of the hostname is the name of the database.
			// The client may override this field at any point after the construction stage.
			if (!Uri.authority.host.empty()) DBName = Uri.authority.host.substr(0, Uri.authority.host.find(_NORW(CharT, ".")));
		}

		CosmosDatabase(const std::basic_string<CharT>& p, const std::basic_string<CharT>& s)
		    : CosmosDatabase(p)
		    , Secondary(s)
		{
			// NOTE: We will only take the Uri from the primary and the secondary connection string is used to store the backup key.
		}
	};


	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const CosmosDatabase<CharT>& src)
	{
		if constexpr (std::is_same_v<CharT, char>) {
			dest["Uri"]       = src.Uri;
			dest["Primary"]   = src.Primary;
			dest["Secondary"] = src.Secondary;
			dest["DBName"]    = src.DBName;
		}
		if constexpr (std::is_same_v<CharT, wchar_t>) {
			auto converter    = std::wstring_convert<std::codecvt_utf8<CharT>> {};

			dest["Uri"]       = src.Uri;
			dest["Primary"]   = src.Primary;
			dest["Secondary"] = src.Secondary;
			dest["DBName"]    = converter.to_bytes(src.DBName);
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
		explicit CosmosClient(const std::basic_string<CharT>& cs)
		    : Database(cs)
		{
			m_config["ConnectionStrings"][0] = cs;
		}

		/// @brief Construct from Cosmos Connection String argument
		/// @param cs Cosmos Connection String
		explicit CosmosClient(const CosmosConnectionString<CharT>& cs)
		    : Database(cs)
		{
			m_config["ConnectionStrings"][0] = cs;
		}

		/// @brief Construct from CosmosDatabase argument
		/// @param db CosmosDatabase argument
		explicit CosmosClient(const CosmosDatabase<CharT>& db)
		    : Database(db)
		{
			m_config["ConnectionStrings"][0] = db.Primary;
			m_config["ConnectionStrings"][1] = db.Secondary;
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
		friend void to_json(nlohmann::json& dest, const CosmosClient<CharT>& src);
		// friend void to_json(nlohmann::json& dest, const CosmosClient<char>& src);
		// friend void to_json(nlohmann::json& dest, const CosmosClient<wchar_t>& src);
	};

	template <typename CharT = char>
		requires std::same_as<CharT, char> || std::same_as<CharT, wchar_t>
	static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<CharT>& src)
	{
		dest["Database"]      = src.Database;
		dest["Configuration"] = src.m_config;
	}


	// static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<char>& src)
	//{
	//	dest["Database"]      = src.Database;
	//	dest["Configuration"] = src.m_config;
	//}

	// static void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient<wchar_t>& src)
	//{
	//	dest["Database"]      = src.Database;
	//	dest["Configuration"] = src.m_config;
	//}

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
