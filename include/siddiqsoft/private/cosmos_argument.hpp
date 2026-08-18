/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_ARGUMENT_HPP
#define COSMOS_ARGUMENT_HPP

#include "cosmos_response.hpp"

namespace siddiqsoft
{
    /// @brief Cosmos data extends the nlohmann::json and adds the callback
    struct CosmosArgumentType
    {
        CosmosOperation operation {};
        std::string     database {};
        std::string     collection {};
        std::string     id {};
        std::string     partitionKey {};
        std::string     continuationToken {};
        std::string     queryStatement {};
        nlohmann::json  queryParameters;
        nlohmann::json  document;
        /// @brief The callback for this request. Invoked with the CosmosResponseType ref
        std::function<void(CosmosArgumentType const&, CosmosResponseType const&)> onResponse {};

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(CosmosArgumentType,
                                       operation,
                                       database,
                                       collection,
                                       id,
                                       partitionKey,
                                       continuationToken,
                                       queryStatement,
                                       queryParameters,
                                       document);
    };

    /// @brief Alias to the callback for async operation
    using CosmosAsyncCallbackType = std::function<void(CosmosArgumentType const&, CosmosResponseType const&)>;
} // namespace siddiqsoft

#endif // !COSMOS_ARGUMENT_HPP
