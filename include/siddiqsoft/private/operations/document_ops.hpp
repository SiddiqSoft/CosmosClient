/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_DOCUMENT_OPS_HPP
#define COSMOS_DOCUMENT_OPS_HPP

#include "../cosmos_connection.hpp"
#include "../cosmos_response.hpp"
#include "../cosmos_argument.hpp"

namespace siddiqsoft
{
    inline CosmosIterableResponseType CosmosClient::listDocuments(CosmosArgumentType const& ctx)
    {
        timethis       tt {};
        auto           ts   = DateUtils::RFC7231();
        auto           path = std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentReadUri(), ctx.database, ctx.collection);
        nlohmann::json headers {
                {"Authorization",
                 EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "GET", "docs", std::format("dbs/{}/colls/{}", ctx.database, ctx.collection), ts)},
                {"x-ms-date", ts},
                {"x-ms-version", config["apiVersion"]}};

        if (!ctx.continuationToken.empty()) headers["x-ms-continuation"] = ctx.continuationToken;

        auto req = rest_request<char>(HttpMethodType::METHOD_GET, path, headers);
        return make_CosmosIterableResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", true}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }

    inline CosmosResponseType CosmosClient::createDocument(CosmosArgumentType const& ctx)
    {
        timethis     tt {};
        auto         ts        = DateUtils::RFC7231();
        std::string& pkKeyName = config.at("/partitionKeyNames/0"_json_pointer).get_ref<std::string&>();
        auto         pkId      = ctx.document.value(pkKeyName, "");

        if (ctx.document.value("id", "").empty()) throw std::invalid_argument("create - I need the uniqueid of the document");
        if (!ctx.document.contains(config.at("/partitionKeyNames/0"_json_pointer)))
            throw std::invalid_argument("create - I need the partitionId of the document");

        auto req = rest_request<char> {
                HttpMethodType::METHOD_POST,
                std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), ctx.database, ctx.collection),
                {{"Authorization",
                  EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", ctx.database, ctx.collection), ts)},
                 {"x-ms-date", ts},
                 {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                 {"x-ms-version", config["apiVersion"]},
                 {"x-ms-cosmos-allow-tentative-writes", "true"}},
                ctx.document};

        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }

    inline CosmosResponseType CosmosClient::upsertDocument(CosmosArgumentType const& ctx)
    {
        timethis tt {};

        if (ctx.document.value("id", "").empty()) throw std::invalid_argument("upsert - I need the uniqueid of the document");
        if (!ctx.document.contains(config.at("/partitionKeyNames/0"_json_pointer)))
            throw std::invalid_argument("upsert - I need the partitionId of the document");

        auto               ts   = DateUtils::RFC7231();
        auto               pkId = ctx.document.value(config.at("/partitionKeyNames/0"_json_pointer).get_ref<std::string&>(), "");

        rest_request<char> req {
                HttpMethodType::METHOD_POST,
                std::format("{}dbs/{}/colls/{}/docs", cnxn.current().currentWriteUri(), ctx.database, ctx.collection),
                {{"Authorization",
                  EncryptionUtils::CosmosToken<char>(cnxn.current().Key, "POST", "docs", std::format("dbs/{}/colls/{}", ctx.database, ctx.collection), ts)},
                 {"x-ms-date", ts},
                 {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                 {"x-ms-documentdb-is-upsert", "true"},
                 {"x-ms-version", config["apiVersion"]},
                 {"x-ms-cosmos-allow-tentative-writes", "true"}},
                ctx.document};

        auto restClient = GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}});
        return make_CosmosResponseType(tt, restClient->send(req));
    }

    inline CosmosResponseType CosmosClient::updateDocument(CosmosArgumentType const& ctx)
    {
        timethis tt {};
        auto     ts = DateUtils::RFC7231();

        if (ctx.id.empty()) throw std::invalid_argument("update - I need the docId of the document");
        if (ctx.partitionKey.empty()) throw std::invalid_argument("update - I need the pkId of the document");
        if (ctx.document.is_null() || ctx.document.size() == 0) throw std::invalid_argument("update - Need the document");

        rest_request<char> req {HttpMethodType::METHOD_PUT,
                                std::format("{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), ctx.database, ctx.collection, ctx.id),
                                {{"Authorization",
                                  EncryptionUtils::CosmosToken<char>(
                                          cnxn.current().Key, "PUT", "docs", std::format("dbs/{}/colls/{}/docs/{}", ctx.database, ctx.collection, ctx.id), ts)},
                                 {"x-ms-date", ts},
                                 {"x-ms-documentdb-partitionkey", nlohmann::json {ctx.partitionKey}},
                                 {"x-ms-version", config["apiVersion"]},
                                 {"x-ms-cosmos-allow-tentative-writes", "true"}},
                                ctx.document};

        return make_CosmosResponseType(
                tt, GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req));
    }

    inline uint32_t CosmosClient::removeDocument(CosmosArgumentType const& ctx)
    {
        auto ts = DateUtils::RFC7231();

        if (ctx.id.empty()) throw std::invalid_argument("remove - I need the docId of the document");
        if (ctx.partitionKey.empty()) throw std::invalid_argument("remove - I need the pkId of the document");

        rest_request<char> req {
                HttpMethodType::METHOD_DELETE,
                std::format("{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentWriteUri(), ctx.database, ctx.collection, ctx.id),
                {{"Authorization",
                  EncryptionUtils::CosmosToken<char>(
                          cnxn.current().Key, "DELETE", "docs", std::format("dbs/{}/colls/{}/docs/{}", ctx.database, ctx.collection, ctx.id), ts)},
                 {"x-ms-date", ts},
                 {"x-ms-documentdb-partitionkey", nlohmann::json {ctx.partitionKey}},
                 {"x-ms-version", config["apiVersion"]},
                 {"x-ms-cosmos-allow-tentative-writes", "true"}}};

        auto resp = GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}})->send(req);

        return resp.has_value() ? resp->statusCode() : resp.error();
    }

    inline CosmosResponseType CosmosClient::findDocument(CosmosArgumentType const& ctx)
    {
        timethis tt {};
        auto     ts = DateUtils::RFC7231();

        if (ctx.id.empty()) throw std::invalid_argument("find - I need the docId of the document");
        if (ctx.partitionKey.empty()) throw std::invalid_argument("find - I need the pkId of the document");

        siddiqsoft::rest_request<char> req {
                HttpMethodType::METHOD_GET,
                std::format("{}dbs/{}/colls/{}/docs/{}", cnxn.current().currentReadUri(), ctx.database, ctx.collection, ctx.id),
                {{"Authorization",
                  EncryptionUtils::CosmosToken<char>(
                          cnxn.current().Key, "GET", "docs", std::format("dbs/{}/colls/{}/docs/{}", ctx.database, ctx.collection, ctx.id), ts)},
                 {"x-ms-date", ts},
                 {"x-ms-documentdb-partitionkey", nlohmann::json {ctx.partitionKey}},
                 {"x-ms-version", config["apiVersion"]},
                 {"x-ms-cosmos-allow-tentative-writes", "true"}}};

        auto restClient = GetRESTClient({{"userAgent", CosmosClientUserAgentString}, {"trace", false}, {"verifyPeer", 0L}, {"freshConnect", false}});
        return make_CosmosResponseType(tt, restClient->send(req));
    }
} // namespace siddiqsoft

#endif // !COSMOS_DOCUMENT_OPS_HPP
