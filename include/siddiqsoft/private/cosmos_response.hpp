/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_RESPONSE_HPP
#define COSMOS_RESPONSE_HPP

#include "cosmos_types.hpp"

namespace siddiqsoft
{
    /// @brief The CosmosResponseType contains status code and the json content returned by the server.
    /// - `uint32_t` - Status Code from the server
    /// - `nlohmann::json` - Json contents from the server
    struct CosmosResponseType
    {
        /// @brief Status Code from the server
        uint32_t statusCode {};

        /// @brief Document from the server
        nlohmann::json document;

        /// @brief Represents the total time
        std::chrono::microseconds ttx {};

        /// @brief Checks if the response is successful based on the HTTP status code
        /// @return true iff the statusCode is in the range [200, 300)
        inline bool success() const { return statusCode >= 200 && statusCode < 300; }
    };

    /// @brief Serializer for CosmosResponseType
    /// @param dest Destination json object
    /// @param src CosmosResponseType
    inline void to_json(nlohmann::json& dest, CosmosResponseType const& src)
    {
        dest["statusCode"] = src.statusCode;
        dest["document"]   = src.document;
        dest["ttx"]        = src.ttx.count();
    }

    [[nodiscard]] inline auto make_CosmosResponseType(timethis& tt, std::expected<siddiqsoft::rest_response<char>, int>&& ret) -> CosmosResponseType
    {
        CosmosResponseType crt;

        crt.ttx = std::chrono::microseconds(tt.elapsed().count());
        if (ret.has_value() && ret->success()) {
            crt.statusCode = ret->statusCode();
            crt.document   = std::move(ret->getContentBodyJSON());

            gCLog.trace("- CRT (good)  statusCode:{}\n{}", crt.statusCode, crt.document.dump());
        }
        else if (ret.has_value()) {
            gCLog.trace("- Raw response (failed):\n{}", *ret);

            // Has value but not successful, return the code
            std::tie(crt.statusCode, std::ignore) = ret->status();
        }
        else {
            crt.statusCode = ret.error();
        }

        return crt;
    }

    /// @brief The CosmosIterableResponseType inherits from the CosmosResponseType and includes the continuation token
    /// - `std::string` - Continuation Token.
    struct CosmosIterableResponseType : CosmosResponseType
    {
        /// @brief Continuation token from the server
        std::string continuationToken;
    };

    [[nodiscard]] inline auto make_CosmosIterableResponseType(timethis& tt, std::expected<siddiqsoft::rest_response<char>, int>&& ret) -> CosmosIterableResponseType
    {
        CosmosIterableResponseType iterableRespFromCosmos;

        iterableRespFromCosmos.ttx = std::chrono::microseconds(tt.elapsed().count());
        if (ret.has_value() && ret->success()) {
            gCLog.trace("- Raw response (good):\n{}", *ret);

            iterableRespFromCosmos.statusCode = ret->statusCode();
            iterableRespFromCosmos.document   = std::move(ret->getContentBodyJSON());
            try {
                iterableRespFromCosmos.continuationToken = ret->getHeader("x-ms-continuation");
            }
            catch (...) {
            }

            gCLog.trace("- CIRT (good)  statusCode:{}\n{}", iterableRespFromCosmos.statusCode, iterableRespFromCosmos.document.dump(4));
        }
        else if (ret.has_value()) {
            gCLog.trace("- Raw response (failed):\n{}", *ret);
            // Has value but not successful, return the code
            std::tie(iterableRespFromCosmos.statusCode, std::ignore) = ret->status();
        }
        else {
            iterableRespFromCosmos.statusCode = ret.error();
        }

        return iterableRespFromCosmos;
    }

    /// @brief Serializer for CosmosIterableResponseType uses the serializer for CosmosResponseType
    /// @param dest Destination json object
    /// @param src CosmosIterableResponseType
    inline void to_json(nlohmann::json& dest, CosmosIterableResponseType const& src)
    {
        to_json(dest, CosmosResponseType(src));
        dest["continuationToken"] = src.continuationToken;
    }
} // namespace siddiqsoft

#endif // !COSMOS_RESPONSE_HPP
