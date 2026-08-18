/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_DATABASE_OPS_HPP
#define COSMOS_DATABASE_OPS_HPP

#include "../cosmos_connection.hpp"
#include "../cosmos_response.hpp"
#include "../cosmos_argument.hpp"

namespace siddiqsoft
{
    inline CosmosResponseType CosmosClient::createDatabase(CosmosArgumentType const& ctx)
    {
        timethis tt {};
        auto     ts = DateUtils::RFC7231();

        if (ctx.database.empty()) gCLog.err_throw<std::invalid_argument>("Need `partitionId` for db: {}", nlohmann::json(ctx).dump());

        auto req = rest_request<char> {HttpMethodType::METHOD_POST,
                                       std::format("{}dbs", cnxn.current().currentWriteUri()),
                                       {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "POST", "dbs", "", ts)},
                                        {"x-ms-date", ts},
                                        {"x-ms-version", config["apiVersion"]},
                                        {"x-ms-cosmos-allow-tentative-writes", "true"}},
                                       {{"id", ctx.database}}};
        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", true}, {"verifyPeer", 0L}, {"freshConnect", true}})->send(req));
    }

    inline CosmosResponseType CosmosClient::deleteDatabase(CosmosArgumentType const& ctx)
    {
        timethis tt {};
        auto     ts = DateUtils::RFC7231();

        if (ctx.database.empty()) gCLog.err_throw<std::invalid_argument>("Need `partitionId` for db: {}", nlohmann::json(ctx).dump());

        auto req = rest_request<char> {
                HttpMethodType::METHOD_DELETE,
                std::format("{}dbs/{}", cnxn.current().currentWriteUri(), ctx.database),
                {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "DELETE", "dbs", std::format("dbs/{}", ctx.database), ts)},
                 {"x-ms-date", ts},
                 {"x-ms-version", config["apiVersion"]},
                 {"x-ms-cosmos-allow-tentative-writes", "true"}}};
        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }

    inline CosmosResponseType CosmosClient::findDatabase(CosmosArgumentType const& ctx)
    {
        timethis tt {};
        auto     ts = DateUtils::RFC7231();

        if (ctx.database.empty()) gCLog.err_throw<std::invalid_argument>("Need `database` for db: {}", nlohmann::json(ctx).dump());

        auto req = rest_request<char> {
                HttpMethodType::METHOD_GET,
                std::format("{}dbs/{}", cnxn.current().currentReadUri(), ctx.database),
                {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "dbs", std::format("dbs/{}", ctx.database), ts)},
                 {"x-ms-date", ts},
                 {"x-ms-version", config["apiVersion"]},
                 {"x-ms-cosmos-allow-tentative-writes", "true"}}};
        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }

    inline CosmosResponseType CosmosClient::listDatabases()
    {
        timethis tt {};

        auto ts   = DateUtils::RFC7231();
        auto path = cnxn.current().currentReadUri() + "dbs";
        auto req  = rest_request<char>(HttpMethodType::METHOD_GET,
                                       path,
                                       {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "dbs", "", ts)},
                                        {"x-ms-date", ts},
                                        {"x-ms-version", config["apiVersion"]}});

        gCLog.trace("- Sending request to Cosmos: {}", req);
        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }
} // namespace siddiqsoft

#endif // !COSMOS_DATABASE_OPS_HPP
