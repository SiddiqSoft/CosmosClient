/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_TYPES_HPP
#define COSMOS_TYPES_HPP

#include <type_traits>
#include <string>
#include <string_view>
#include <iostream>
#include <functional>
#include <format>
#include <atomic>
#include <expected>
#include <chrono>
#include <vector>

#include "nlohmann/json.hpp"

#include "siddiqsoft/conversion-utils.hpp"
#include "siddiqsoft/encryption-utils.hpp"
#include "siddiqsoft/ScopeTrace.hpp"
#include "siddiqsoft/restcl.hpp"
#include "siddiqsoft/simple_pool.hpp"
#include "siddiqsoft/timethis.hpp"

namespace siddiqsoft
{
/// @brief The global logging instance for the cosmoscl library.
#if defined(DEBUG_TRACE) || defined(cosmoscl_DEBUG_TRACE)
    inline auto& gCLog = siddiqsoft::ScopeTrace::GetInstance("cosmoscl", siddiqsoft::LogLevel::trace);
#else
    inline auto& gCLog = siddiqsoft::ScopeTrace::GetInstance("cosmoscl", siddiqsoft::LogLevel::error);
#endif

    /// @brief Azure Cosmos Operations
    enum class CosmosOperation
    {
        notset = 0,
        createDatabase,
        createCollection,
        discoverRegions,
        listDatabases,
        listCollections,
        listDocuments,
        create,
        upsert,
        update,
        remove,
        find,
        query
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(CosmosOperation,
                                 {{CosmosOperation::createDatabase, "createDatabase"},
                                  {CosmosOperation::createCollection, "createCollection"},
                                  {CosmosOperation::discoverRegions, "discoverRegions"},
                                  {CosmosOperation::listDatabases, "listDatabases"},
                                  {CosmosOperation::listCollections, "listCollections"},
                                  {CosmosOperation::listDocuments, "listDocuments"},
                                  {CosmosOperation::create, "create"},
                                  {CosmosOperation::upsert, "upsert"},
                                  {CosmosOperation::update, "update"},
                                  {CosmosOperation::remove, "remove"},
                                  {CosmosOperation::find, "find"},
                                  {CosmosOperation::query, "query"},
                                  {CosmosOperation::notset, nullptr}});
} // namespace siddiqsoft

#endif // !COSMOS_TYPES_HPP
