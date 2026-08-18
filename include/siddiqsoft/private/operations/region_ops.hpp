/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_REGION_OPS_HPP
#define COSMOS_REGION_OPS_HPP

#include "../cosmos_connection.hpp"
#include "../cosmos_response.hpp"
#include "../cosmos_argument.hpp"

namespace siddiqsoft
{
    /// @brief Discover the Regions for the current base Uri
    /// This method is invoked by the `configure` method.
    /// @return Tuple of the status code and the json response (or empty)
    inline CosmosResponseType CosmosClient::discoverRegions()
    {
        timethis           tt {};
        auto               ts = DateUtils::RFC7231();

        rest_request<char> req {HttpMethodType::METHOD_GET,
                                cnxn.current().currentReadUri(),
                                {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "", "", ts)},
                                 {"x-ms-date", ts},
                                 {"x-ms-version", config["apiVersion"]}}};

        gCLog.trace("....DiscoverRegions - request this is our request: {}", req);

        return make_CosmosResponseType(
                tt,
                GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", true}, {"verifyPeer", 0L}, {"useTLSv1_2", true}, {"freshConnect", false}})
                        ->send(req));
    }
} // namespace siddiqsoft

#endif // !COSMOS_REGION_OPS_HPP
