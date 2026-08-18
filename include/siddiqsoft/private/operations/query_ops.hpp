/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_QUERY_OPS_HPP
#define COSMOS_QUERY_OPS_HPP

#include "../cosmos_connection.hpp"
#include "../cosmos_response.hpp"
#include "../cosmos_argument.hpp"

namespace siddiqsoft
{
    inline CosmosIterableResponseType CosmosClient::queryDocuments(CosmosArgumentType const& ctx)
    {
        timethis       tt {};
        auto           ts = DateUtils::RFC7231();
        nlohmann::json headers {
                {"Authorization",
                 EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", ctx.database, ctx.collection), ts)},
                {"x-ms-date", ts},
                {"x-ms-max-item-count", -1}, // -1: Let Cosmos figure out item count
                {"x-ms-documentdb-isquery", true},
                {"x-ms-version", config["apiVersion"]},
                {"Accept", "application/json"},
                {"Content-Type", "application/query+json"}}; // The content type must be exactly as-is

        if (ctx.queryStatement.empty()) throw std::invalid_argument("Missing queryStatement");

        if (ctx.partitionKey.starts_with("*")) {
            // Special case query with partitioned data set.
            headers["x-ms-documentdb-query-enablecrosspartition"] = true;
            // This is required if the client does not provide partitionkey
            headers["x-ms-query-enable-crosspartition"] = true;
        }
        else if (!ctx.partitionKey.empty()) {
            // Specific partition set by client.
            headers["x-ms-documentdb-partitionkey"] = nlohmann::json {ctx.partitionKey};
        }

        if (!ctx.continuationToken.empty()) {
            headers["x-ms-continuation"] = ctx.continuationToken;
        }

        rest_request<char> req {HttpMethodType::METHOD_POST,
                                std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), ctx.database, ctx.collection),
                                headers,
                                !ctx.queryParameters.is_null() && ctx.queryParameters.is_array()
                                        ? nlohmann::json {{"query", ctx.queryStatement}, {"parameters", ctx.queryParameters}}
                                        : nlohmann::json {{"query", ctx.queryStatement}}};

        return make_CosmosIterableResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", true}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }
} // namespace siddiqsoft

#endif // !COSMOS_QUERY_OPS_HPP
