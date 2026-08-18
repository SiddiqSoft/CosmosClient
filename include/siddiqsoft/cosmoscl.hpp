/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOSCL_HPP
#define COSMOSCL_HPP

#include "private/cosmos_types.hpp"
#include "private/cosmos_endpoint.hpp"
#include "private/cosmos_connection.hpp"
#include "private/cosmos_response.hpp"
#include "private/cosmos_argument.hpp"

namespace siddiqsoft
{
    /// @brief Cosmos Client
    /// Implements a stateful Cosmos Client using Cosmos SQL-API via REST API
    class CosmosClient
    {
#if defined(cosmoscl_TESTING_MODE)
    public:
#else
    protected:
#endif
        /// @brief configuration object updated/merged with the client object
        nlohmann::json config {
                {"_typever", CosmosClientUserAgentString},
                {"libRetryLimit", 7},
                {"apiVersion", "2018-12-31"},   // The API version for Cosmos REST API
                {"connectionStrings", nullptr}, // The Connection String from the Azure portal
                {"partitionKeyNames", nullptr}  // The partition key names is an array of partition key names
        };

        /// @brief Service Settings saved from discoverRegion
        nlohmann::json serviceSettings;

        /// @brief Used to signal first-time configuration
        std::atomic_bool isConfigured {false};

        /// @brief The connection object stores Primary & Secondary connection strings
        CosmosConnection cnxn {};

        /// @brief The async worker pool
        simple_pool<CosmosArgumentType> asyncWorkers {std::bind_front(&CosmosClient::asyncDispatcher, this)};

        /// @brief The async dispatcher/driver
        void asyncDispatcher(CosmosArgumentType&& req);

    public:
        /// @brief This is the string used in the User-Agent header
        inline static const std::string CosmosClientUserAgentString {"SiddiqSoft.CosmosClient/0.10.0"};

        /// @brief Default constructor
        CosmosClient() = default;

        /// @brief Move constructor
        CosmosClient(CosmosClient&& src) noexcept
            : config(std::move(src.config))
            , serviceSettings(std::move(src.serviceSettings))
            , isConfigured(src.isConfigured.load())
            , cnxn(std::move(src.cnxn))
        {
        }

        /// @brief Move assignment operator
        CosmosClient& operator=(CosmosClient&& src) noexcept
        {
            if (this != &src) {
                config          = std::move(src.config);
                serviceSettings = std::move(src.serviceSettings);
                isConfigured    = src.isConfigured.load();
                cnxn            = std::move(src.cnxn);
            }
            return *this;
        }

        CosmosClient(const CosmosClient&)    = delete;
        auto& operator=(const CosmosClient&) = delete;

        /// @brief Gets the current configuration object
        const nlohmann::json& configuration() const { return config; }

        /// @brief Re-configure the client.
        CosmosClient& configure(const nlohmann::json& src = {}) noexcept(false);

        /// @brief Invokes the requested operation from threadpool
        void async(CosmosArgumentType&& op) noexcept(false);

        // Region operations
        CosmosResponseType discoverRegions();

        // Database operations
        CosmosResponseType createDatabase(CosmosArgumentType const& ctx);
        CosmosResponseType deleteDatabase(CosmosArgumentType const& ctx);
        CosmosResponseType findDatabase(CosmosArgumentType const& ctx);
        CosmosResponseType listDatabases();

        // Collection operations
        CosmosResponseType createCollection(CosmosArgumentType const& ctx);
        CosmosResponseType listCollections(CosmosArgumentType const& ctx);

        // Document operations
        CosmosIterableResponseType listDocuments(CosmosArgumentType const& ctx);
        CosmosResponseType createDocument(CosmosArgumentType const& ctx);
        CosmosResponseType upsertDocument(CosmosArgumentType const& ctx);
        CosmosResponseType updateDocument(CosmosArgumentType const& ctx);
        uint32_t removeDocument(CosmosArgumentType const& ctx);
        CosmosResponseType findDocument(CosmosArgumentType const& ctx);

        // Query operations
        CosmosIterableResponseType queryDocuments(CosmosArgumentType const& ctx);

        /// @brief JSON serializer helper for CosmosClient
        friend void to_json(nlohmann::json& dest, const siddiqsoft::CosmosClient& src);
    };

    inline CosmosClient& CosmosClient::configure(const nlohmann::json& src) noexcept(false)
    {
        if (!src.empty()) {
            if (src.is_array()) gCLog.err_throw<std::invalid_argument>("src is array instead of object");
            if (!src.contains("connectionStrings")) gCLog.err_throw<std::invalid_argument>("connectionStrings missing");
            if (!src.contains("partitionKeyNames")) gCLog.err_throw<std::invalid_argument>("partitionKeyNames missing");

            gCLog.trace("- Contents of existing configuraion\n{}\nIncoming Configuration:\n{}", config.dump(), src.dump());

            config["connectionStrings"] = src["connectionStrings"];
            config["partitionKeyNames"] = src["partitionKeyNames"];

            gCLog.trace("- Contents of Updated configuraion\n{}", config.dump());

            if (!config["connectionStrings"].is_array()) gCLog.err_throw<std::invalid_argument>("connectionStrings must be array");
            if (config["connectionStrings"].size() < 1) gCLog.err_throw<std::invalid_argument>("connectionStrings array must contain atleast primary element");

            gCLog.trace("- Contents of connection configuraion\n{}", nlohmann::json(cnxn).dump());
            cnxn.configure(config);

            gCLog.trace("- Contents of Updated connection configuraion\n{}", nlohmann::json(cnxn).dump());

            if (auto resp = discoverRegions(); resp.statusCode == 200 && !resp.document.empty()) {
                serviceSettings = resp.document;
                cnxn.configure(serviceSettings);
                isConfigured = true;
            }
        }

        return *this;
    }

    inline void CosmosClient::async(CosmosArgumentType&& op) noexcept(false)
    {
        switch (op.operation) {
            case CosmosOperation::discoverRegions:
            case CosmosOperation::listDatabases: break;

            case CosmosOperation::createDatabase: {
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
            } break;

            case CosmosOperation::createCollection: {
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
                if (op.collection.empty()) gCLog.err_throw<std::invalid_argument>("op.collection required");
            } break;

            case CosmosOperation::listDocuments:
                if (op.collection.empty()) gCLog.err_throw<std::invalid_argument>("op.collection required");
                [[fallthrough]];
            case CosmosOperation::listCollections:
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
                break;
            case CosmosOperation::create:
            case CosmosOperation::upsert:
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
                if (op.collection.empty()) gCLog.err_throw<std::invalid_argument>("op.collection required");
                if (op.document.empty()) gCLog.err_throw<std::invalid_argument>("op.document required");
                if (op.document.value("id", "").empty()) gCLog.err_throw<std::invalid_argument>("op.document[id] required");
                if (!op.document.contains(config.at("/partitionKeyNames/0"_json_pointer)))
                    gCLog.err_throw<std::invalid_argument>("op.document[] must contain partition key");
                break;
            case CosmosOperation::update:
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
                if (op.collection.empty()) gCLog.err_throw<std::invalid_argument>("op.collection required");
                if (op.id.empty()) gCLog.err_throw<std::invalid_argument>("op.id required");
                if (op.partitionKey.empty()) gCLog.err_throw<std::invalid_argument>("op.partitionKey required");
                if (op.document.empty()) gCLog.err_throw<std::invalid_argument>("op.document required");
                break;
            case CosmosOperation::query:
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
                if (op.collection.empty()) gCLog.err_throw<std::invalid_argument>("op.collection required");
                if (op.partitionKey.empty()) gCLog.err_throw<std::invalid_argument>("op.partitionKey required");
                if (op.queryStatement.empty()) gCLog.err_throw<std::invalid_argument>("op.queryStatement required");
                break;
            case CosmosOperation::remove:
            case CosmosOperation::find:
                if (op.database.empty()) gCLog.err_throw<std::invalid_argument>("op.database required");
                if (op.collection.empty()) gCLog.err_throw<std::invalid_argument>("op.collection required");
                if (op.id.empty()) gCLog.err_throw<std::invalid_argument>("op.id required");
                if (op.partitionKey.empty()) gCLog.err_throw<std::invalid_argument>("op.partitionKey required");
                break;
            case CosmosOperation::notset:
            default: gCLog.err_throw<std::invalid_argument>("requires op.operation be valid: {}", nlohmann::json(op.operation).dump());
        }

        if (!op.onResponse) gCLog.err_throw<std::invalid_argument>("async requires op.onResponse be valid callback");

        asyncWorkers.queue(std::move(op));
    }

    inline void CosmosClient::asyncDispatcher(CosmosArgumentType&& req)
    {
        try {
            switch (req.operation) {
                case CosmosOperation::createDatabase: {
                    auto resp = createDatabase(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::createCollection: {
                    auto resp = createCollection(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::discoverRegions: {
                    auto resp = discoverRegions();
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::listDatabases: {
                    auto resp = listDatabases();
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::listCollections: {
                    auto resp = listCollections(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::listDocuments: {
                    CosmosIterableResponseType resp = listDocuments(req);
                    if (req.onResponse) req.onResponse(req, resp);
                    if (resp.success() && !resp.continuationToken.empty()) {
                        req.continuationToken = resp.continuationToken;
                        gCLog.trace("....Status:{}  continueToken:{}  count:{}  ttx:{} requeue",
                                    resp.statusCode,
                                    resp.continuationToken,
                                    resp.document.value("_count", 0),
                                    resp.ttx);
                        asyncWorkers.queue(std::move(req));
                    }
                } break;

                case CosmosOperation::create: {
                    auto resp = createDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::upsert: {
                    auto resp = upsertDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::update: {
                    auto resp = updateDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::find: {
                    auto resp = findDocument(req);
                    if (req.onResponse) req.onResponse(req, resp);
                } break;

                case CosmosOperation::remove: {
                    auto rc = removeDocument(req);
                    if (req.onResponse) req.onResponse(req, {rc, nullptr});
                } break;

                case CosmosOperation::query: {
                    CosmosIterableResponseType resp = queryDocuments(req);
                    if (req.onResponse) req.onResponse(req, resp);
                    if (resp.success() && !resp.continuationToken.empty()) {
                        req.continuationToken = resp.continuationToken;
                        gCLog.trace("....Status:{}  continueToken:{}  count:{}  ttx:{} requeue",
                                    resp.statusCode,
                                    resp.continuationToken,
                                    resp.document.value("_count", 0),
                                    resp.ttx);
                        asyncWorkers.queue(std::move(req));
                    }
                } break;
                default: {
                    gCLog.warn("....Operation `{}` NOT SUPPORTED", std::to_underlying(req.operation));
                }
            }
        }
        catch (const std::exception& ex) {
            gCLog.exp(ex);
        }
        catch (...) {
            gCLog.err("....Unknown Exception in asyncDispatcher");
        }
    }
} // namespace siddiqsoft

// Include operational implementations
#include "private/operations/region_ops.hpp"
#include "private/operations/database_ops.hpp"
#include "private/operations/collection_ops.hpp"
#include "private/operations/document_ops.hpp"
#include "private/operations/query_ops.hpp"

// Include serializers & formatters
#include "private/cosmos_serializers.hpp"

#endif // !COSMOSCL_HPP
