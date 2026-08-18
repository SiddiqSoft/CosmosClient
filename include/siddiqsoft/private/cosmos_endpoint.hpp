/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_ENDPOINT_HPP
#define COSMOS_ENDPOINT_HPP

#include "cosmos_types.hpp"

namespace siddiqsoft
{
#pragma region CosmosEndpoint
    /// @brief Cosmos Connection String as available in the Azure Portal
    struct CosmosEndpoint
    {
        /// @brief The "home" or base Uri points to the home location.
        ///	For globally partitioned/availability zones, we use this to build the readable/writable
        std::basic_string<char> BaseUri {};

        /// @brief The Base64 encoded key
        std::basic_string<char> EncodedKey {};

        /// @brief The "binary" key decoded from the EncodedKey stored as std::string
        std::string Key {};

        /// @brief Read Locations for the region
        std::vector<std::basic_string<char>> ReadableUris {};

        /// @brief Current Read location within the ReadableUris
        std::atomic<size_t> CurrentReadUriId {0};

        /// @brief Write Locations for the region
        std::vector<std::basic_string<char>> WritableUris {};

        /// @brief Current Write Location within the WritableUris
        std::atomic<size_t> CurrentWriteUriId {0};

        /// @brief Default constructor
        CosmosEndpoint() = default;

        /// @brief Copy constructor
        CosmosEndpoint(const CosmosEndpoint& src)
            : BaseUri(src.BaseUri)
            , EncodedKey(src.EncodedKey)
            , Key(src.Key)
            , ReadableUris(src.ReadableUris)
            , CurrentReadUriId(src.CurrentReadUriId.load())
            , WritableUris(src.WritableUris)
            , CurrentWriteUriId(src.CurrentWriteUriId.load())
        {
        }

        /// @brief Copy assignment operator
        CosmosEndpoint& operator=(const CosmosEndpoint& src)
        {
            if (this != &src) {
                BaseUri          = src.BaseUri;
                EncodedKey       = src.EncodedKey;
                Key              = src.Key;
                ReadableUris     = src.ReadableUris;
                CurrentReadUriId.store(src.CurrentReadUriId.load());
                WritableUris     = src.WritableUris;
                CurrentWriteUriId.store(src.CurrentWriteUriId.load());
            }
            return *this;
        }

        /// @brief Move constructor
        CosmosEndpoint(CosmosEndpoint&& src) noexcept
            : BaseUri(std::move(src.BaseUri))
            , EncodedKey(std::move(src.EncodedKey))
            , Key(std::move(src.Key))
            , ReadableUris(std::move(src.ReadableUris))
            , CurrentReadUriId(src.CurrentReadUriId.load())
            , WritableUris(std::move(src.WritableUris))
            , CurrentWriteUriId(src.CurrentWriteUriId.load())
        {
        }

        /// @brief Move assignment operator
        CosmosEndpoint& operator=(CosmosEndpoint&& src) noexcept
        {
            if (this != &src) {
                BaseUri          = std::move(src.BaseUri);
                EncodedKey       = std::move(src.EncodedKey);
                Key              = std::move(src.Key);
                ReadableUris     = std::move(src.ReadableUris);
                CurrentReadUriId.store(src.CurrentReadUriId.load());
                WritableUris     = std::move(src.WritableUris);
                CurrentWriteUriId.store(src.CurrentWriteUriId.load());
            }
            return *this;
        }

        /// @brief Construct the connection string object from the Connection String obtained from the Azure Portal
        /// @param s Connection String obtained from the Azure Portal
        CosmosEndpoint(const std::basic_string<char>& s) { this->operator=(s); }

        /// @brief Operator assignment
        /// @param s Source string obtained from the Azure portal
        /// @return Self
        CosmosEndpoint& operator=(const std::basic_string<char>& s)
        {
            constexpr std::string_view MatchAccountEndpoint = "AccountEndpoint=";
            constexpr std::string_view MatchAccountKey      = ";AccountKey=";

            // The Azure Cosmos Connection string has the following format
            // AccountEndpoint=BaseUri;AccountKey=Key
            // The BaseUri is fully qualified name with port
            // The Key is the base64 encoded string representing the Cosmos key
            if (s.starts_with(MatchAccountEndpoint)) {
                if (auto posAccountKey = s.find(MatchAccountKey); posAccountKey != std::string::npos) {
                    // We have enough to extract both the uri and the key
                    BaseUri    = s.substr(MatchAccountEndpoint.length(), posAccountKey - MatchAccountEndpoint.length());
                    EncodedKey = s.substr(posAccountKey + MatchAccountKey.length(), s.length() - (posAccountKey + MatchAccountKey.length()));
                    // Make sure to strip off the trailing ; if present
                    if (EncodedKey.ends_with(";")) EncodedKey.resize(EncodedKey.length() - 1);
                    // Store the decoded key (only for std::string)
                    Key = Base64Utils::decode(EncodedKey);
                }
            }

            return *this;
        }

        /// @brief Checks if the BaseUri and the EncodedKey is non-empty
        explicit operator bool() const { return !BaseUri.empty() && !EncodedKey.empty(); }

        /// @brief Cast operator for string
        explicit operator std::basic_string<char>() const { return string(); }

        /// @brief Encodes the contents back into the original connection string from Azure Portal
        /// @return The rebuilt connection string must match that of the original connection string from the Azure Portal
        std::basic_string<char> string() const { return std::format("AccountEndpoint={};AccountKey={};", BaseUri, EncodedKey); }

        /// @brief Current read endpoint
        /// @return Current read endpoint or the base Uri
        const auto& currentReadUri() const
        {
            size_t id = CurrentReadUriId.load();
            if (!ReadableUris.empty() && id < ReadableUris.size()) return ReadableUris.at(id);
            return BaseUri;
        }

        /// @brief Current write endpoint
        /// @return Current write endpoint or the base Uri
        const auto& currentWriteUri() const
        {
            size_t id = CurrentWriteUriId.load();
            if (!WritableUris.empty() && id < WritableUris.size()) return WritableUris.at(id);
            return BaseUri;
        }

        /// @brief Increment the read endpoint to the next one in the list and if we reach the end, go back to start
        /// @return Self
        CosmosEndpoint& rotateReadUri()
        {
            if (!ReadableUris.empty()) {
                size_t id = CurrentReadUriId.load();
                CurrentReadUriId.store((id + 1) % ReadableUris.size());
            }
            else {
                CurrentReadUriId.store(0);
            }
            return *this;
        }

        /// @brief Increment the write endpoint to the next one in the list and if we reach the end, go back to start
        /// @return Self
        CosmosEndpoint& rotateWriteUri()
        {
            if (!WritableUris.empty()) {
                size_t id = CurrentWriteUriId.load();
                CurrentWriteUriId.store((id + 1) % WritableUris.size());
            }
            else {
                CurrentWriteUriId.store(0);
            }
            return *this;
        }
    };

    /// @brief JSON serializer
    /// @param dest Destination json object
    /// @param src The source cosmos connection string
    inline void to_json(nlohmann::json& dest, const CosmosEndpoint& src)
    {
        dest["baseUri"]           = src.BaseUri;
        dest["readUris"]          = src.ReadableUris;
        dest["currentReadUriId"]  = src.CurrentReadUriId.load();
        dest["writeUris"]         = src.WritableUris;
        dest["currentWriteUriId"] = src.CurrentWriteUriId.load();
        dest["key"]               = src.EncodedKey;
    }
#pragma endregion
} // namespace siddiqsoft

#endif // !COSMOS_ENDPOINT_HPP
