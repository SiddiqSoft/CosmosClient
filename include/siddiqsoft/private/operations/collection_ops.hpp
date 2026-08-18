/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_COLLECTION_OPS_HPP
#define COSMOS_COLLECTION_OPS_HPP

#include "../cosmos_connection.hpp"
#include "../cosmos_response.hpp"
#include "../cosmos_argument.hpp"

namespace siddiqsoft
{
    inline CosmosResponseType CosmosClient::createCollection(CosmosArgumentType const& ctx)
    {
        timethis tt {};

        if (!config.contains("/partitionKeyNames/0"_json_pointer)) throw std::invalid_argument("create - I need the partitionKey for collection");

        auto         ts        = DateUtils::RFC7231();
        std::string& pkKeyName = config.at("/partitionKeyNames/0"_json_pointer).get_ref<std::string&>();

        auto req = rest_request<char> {HttpMethodType::METHOD_POST,
                                       std::format("{}dbs/{}/colls", cnxn.current().currentWriteUri(), ctx.database),
                                       {{"Authorization", EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "POST", "colls", {"dbs/" + ctx.database}, ts)},
                                        {"x-ms-date", ts},
                                        {"x-ms-version", config["apiVersion"]},
                                        {"x-ms-cosmos-allow-tentative-writes", "true"}},
                                       {{"id", ctx.collection}, {"partitionKey", {{"kind", "Hash"}, {"Version", 2}, {"paths", {"/" + pkKeyName}}}}}};

        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }

    inline CosmosResponseType CosmosClient::listCollections(CosmosArgumentType const& ctx)
    {
        timethis tt {};
        auto     ts   = DateUtils::RFC7231();
        auto     path = std::format("{}dbs/{}/colls", cnxn.current().currentReadUri(), ctx.database);
        auto     auth = EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "colls", {"dbs/" + ctx.database}, ts);
        auto req = rest_request<char>(HttpMethodType::METHOD_GET, path, {{"Authorization", auth}, {"x-ms-date", ts}, {"x-ms-version", config["apiVersion"]}});
        auto restClient = GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}});
        return make_CosmosResponseType(tt, restClient->send(req));
    }
} // namespace siddiqsoft

#endif // !COSMOS_COLLECTION_OPS_HPP
