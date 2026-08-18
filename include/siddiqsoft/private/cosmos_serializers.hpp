/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_SERIALIZERS_HPP
#define COSMOS_SERIALIZERS_HPP

#include "cosmos_connection.hpp"
#include "cosmos_response.hpp"
#include "cosmos_argument.hpp"

namespace siddiqsoft
{
    /// @brief JSON serializer helper for CosmosClient
    inline void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient& src)
    {
        dest["serviceSettings"] = src.serviceSettings;
        dest["database"]        = src.cnxn;
        dest["configuration"]   = src.config;
        dest["workers"]         = src.asyncWorkers;
        dest["userAgentString"] = src.CosmosClientUserAgentString;
    }
} // namespace siddiqsoft

#pragma region Serializer CosmosEndpoint
/// @brief Serializer for CosmosEndpoint for the char type
template <>
struct std::formatter<siddiqsoft::CosmosEndpoint> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosEndpoint& s, FC& ctx) const
    {
        auto str = std::format("AccountEndpoint={};AccountKey={};", s.BaseUri, s.EncodedKey);
        return std::formatter<std::basic_string<char>>::format(str, ctx);
    }
};

/// @brief Ostream implementation for CosmosEndpoint object
inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosEndpoint& s)
{
    os << std::basic_string<char>(s);
    return os;
}
#pragma endregion

#pragma region Serializer CosmosConnection
/// @brief Serializer for CosmosConnection for the char type
template <>
struct std::formatter<siddiqsoft::CosmosConnection> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosConnection& s, FC& ctx) const
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

/// @brief Output stream write operator
inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosConnection& s)
{
    os << std::format("{}", s);
    return os;
}
#pragma endregion

#pragma region Serializer CosmosClient
/// @brief Serializer for CosmosClient for the char type
template <>
struct std::formatter<siddiqsoft::CosmosClient> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosClient& s, FC& ctx) const
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

/// @brief Ostream writer for CosmosClient
inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, const siddiqsoft::CosmosClient& s)
{
    os << nlohmann::json(s).dump();
    return os;
}
#pragma endregion

/// @brief Serializer for the CosmosIterableResponseType
template <>
struct std::formatter<siddiqsoft::CosmosIterableResponseType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::CosmosIterableResponseType& s, auto& ctx) const
    {
        return std::format_to(ctx.out(), "CIRT:- statusCode: {}, document: {}, ttx: {}  ctoken: {}", s.statusCode, s.document.dump(), s.ttx, s.continuationToken);
    }
};

/// @brief Serializer for the CosmosResponseType
template <>
struct std::formatter<siddiqsoft::CosmosResponseType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::CosmosResponseType& s, auto& ctx) const
    {
        return std::format_to(ctx.out(), "CRT:- statusCode: {}, document: {}, ttx: {}", s.statusCode, s.document.dump(), s.ttx);
    }
};

template <>
struct std::formatter<siddiqsoft::CosmosOperation> : std::formatter<std::underlying_type<siddiqsoft::CosmosOperation>>
{
    auto format(const siddiqsoft::CosmosOperation& co, auto& ctx) const { return std::format_to(ctx.out(), "{}", nlohmann::json(co).dump()); }
};

/// @brief Serializer for the CosmosArgumentType
template <>
struct std::formatter<siddiqsoft::CosmosArgumentType> : std::formatter<std::basic_string<char>>
{
    template <class FC>
    auto format(const siddiqsoft::CosmosArgumentType& s, FC& ctx) const
    {
        return std::formatter<std::basic_string<char>>::format(nlohmann::json(s).dump(), ctx);
    }
};

#endif // !COSMOS_SERIALIZERS_HPP
