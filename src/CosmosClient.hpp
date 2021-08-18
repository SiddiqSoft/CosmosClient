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


namespace siddiqsoft
{
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
			this.operator=(s);
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


		operator std::basic_string<CharT>()
		{
			return std::format(_NORW(CharT, "AccountEndpoint={};AccountKey={};"), Uri, Key);
		}
	};


	/// @brief JSON serializer
	/// @tparam CharT We currently only support char (as the underlying json library does not support wchar_t)
	/// @param dest Destination json object
	/// @param src The source cosmos connection string
	template <typename CharT>
		requires std::same_as<CharT, char>
	static void to_json(nlohmann::json& dest, const CosmosConnectionString<CharT>& src)
	{
		dest["uri"] = src.Uri;
		dest["key"] = src.Key;
	}
} // namespace siddiqsoft


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


#endif // !COSMOSCLIENT_HPP
