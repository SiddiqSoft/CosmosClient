/*
    Azure Cosmos REST Client
    Azure Cosmos REST-API Client for Modern C++

    Copyright (c) 2021, Siddiq Software LLC. All rights reserved.

    BSD 3-Clause License
*/

#pragma once
#ifndef COSMOS_CONNECTION_HPP
#define COSMOS_CONNECTION_HPP

#include "cosmos_endpoint.hpp"

namespace siddiqsoft
{
#pragma region CosmosConnection
    /// @brief Represents the Cosmos cnxn
    struct CosmosConnection
    {
        /// @brief CurrentConnectionIdType
        enum class CurrentConnectionIdType : uint16_t
        {
            PrimaryConnection   = 1,
            SecondaryConnection = 2
        };

        /// @brief Serializer for the internal CurrentConnectionIdType
        NLOHMANN_JSON_SERIALIZE_ENUM(CurrentConnectionIdType,
                                     {{CurrentConnectionIdType::PrimaryConnection, "PrimaryConnection"},
                                      {CurrentConnectionIdType::SecondaryConnection, "SecondaryConnection"}});

        /// @brief Current Connection: 0=Not Set 1=Primary 2=Secondary
        std::atomic<CurrentConnectionIdType> CurrentConnectionId {CurrentConnectionIdType::PrimaryConnection};

        /// @brief The Primary connection string from the Azure Portal
        CosmosEndpoint Primary {};

        /// @brief The Secondary connection string from the Azure Portal
        CosmosEndpoint Secondary {};

        /// @brief Default constructor
        CosmosConnection() = default;

        /// @brief Copy constructor
        CosmosConnection(const CosmosConnection& src)
            : CurrentConnectionId(src.CurrentConnectionId.load())
            , Primary(src.Primary)
            , Secondary(src.Secondary)
        {
        }

        /// @brief Copy assignment operator
        CosmosConnection& operator=(const CosmosConnection& src)
        {
            if (this != &src) {
                CurrentConnectionId.store(src.CurrentConnectionId.load());
                Primary   = src.Primary;
                Secondary = src.Secondary;
            }
            return *this;
        }

        /// @brief Move constructor
        CosmosConnection(CosmosConnection&& src) noexcept
            : CurrentConnectionId(src.CurrentConnectionId.load())
            , Primary(std::move(src.Primary))
            , Secondary(std::move(src.Secondary))
        {
        }

        /// @brief Move assignment operator
        CosmosConnection& operator=(CosmosConnection&& src) noexcept
        {
            if (this != &src) {
                CurrentConnectionId.store(src.CurrentConnectionId.load());
                Primary   = std::move(src.Primary);
                Secondary = std::move(src.Secondary);
            }
            return *this;
        }

        /// @brief Constructor with Primary and optional Secondary.
        /// @param p Primary Connection String from Azure portal
        /// @param s Secondary Connection String from Azure portal
        CosmosConnection(const std::basic_string<char>& p, const std::basic_string<char>& s = {}) { configure({{"connectionStrings", {p, s}}}); }

        /// @brief Configure the Primary and Secondary. Also resets the current connection to the "Primary"
        /// @param config JSON object with the following elements: `connectionStrings` array and `partitionKeyNames` array
        /// @return Self
        /// @remarks Resets the CurrentConnectionId to the Primary(1).
        CosmosConnection& configure(const nlohmann::json& config)
        {
            if (config.contains("connectionStrings") && config.at("connectionStrings").is_array()) {
                Primary   = config.value("/connectionStrings/0"_json_pointer, "");
                Secondary = config.value("/connectionStrings/1"_json_pointer, "");

                if (!Primary) throw std::invalid_argument("Primary must be present");
            }

            // If we have readLocations then load them up for the current connection
            if (config.contains("readableLocations")) {
                for (auto& item : config.at("readableLocations")) {
                    if (CurrentConnectionId.load() == CurrentConnectionIdType::SecondaryConnection)
                        Secondary.ReadableUris.push_back(item.value("databaseAccountEndpoint", ""));
                    else
                        Primary.ReadableUris.push_back(item.value("databaseAccountEndpoint", ""));
                }
            }

            // If we have writeLocations then load them up for the current connection
            if (config.contains("writableLocations")) {
                for (auto& item : config.at("writableLocations")) {
                    if (CurrentConnectionId.load() == CurrentConnectionIdType::SecondaryConnection)
                        Secondary.WritableUris.push_back(item.value("databaseAccountEndpoint", ""));
                    else
                        Primary.WritableUris.push_back(item.value("databaseAccountEndpoint", ""));
                }
            }

            return *this;
        }

        /// @brief Get the current active connection string
        /// @return Cosmos connection string
        /// @return Reference to the current active Connection Primary/Secondary
        const CosmosEndpoint& current() const { return (CurrentConnectionId.load() == CurrentConnectionIdType::SecondaryConnection) ? Secondary : Primary; }

        /// @brief Swaps the current connection by incrementing the current and if we hit past Secondary, we restart at Primary.
        /// @param c Maybe 0=Swap 1=Use Primary 2=Use Secondary
        /// @return Self
        CosmosConnection& rotate(const uint16_t c = 0)
        {
            if (c == 0) {
                // Swap between Primary and Secondary
                auto curr = CurrentConnectionId.load();
                if (curr == CurrentConnectionIdType::PrimaryConnection)
                    CurrentConnectionId.store(CurrentConnectionIdType::SecondaryConnection);
                else if (curr == CurrentConnectionIdType::SecondaryConnection)
                    CurrentConnectionId.store(CurrentConnectionIdType::PrimaryConnection);
            }
            else if (c == 1) {
                CurrentConnectionId.store(CurrentConnectionIdType::PrimaryConnection);
            }
            else if (c == 2) {
                CurrentConnectionId.store(CurrentConnectionIdType::SecondaryConnection);
            }

            // If Secondary is empty; limit to Primary
            if ((CurrentConnectionId.load() == CurrentConnectionIdType::SecondaryConnection) && Secondary.EncodedKey.empty())
                CurrentConnectionId.store(CurrentConnectionIdType::PrimaryConnection);

            return *this;
        }
    };

    /// @brief JSON serializer for the CosmosConnection object
    /// @param dest Output json object
    /// @param src The source CosmosConnection
    inline void to_json(nlohmann::json& dest, const CosmosConnection& src)
    {
        dest["currentConnectionId"] = src.CurrentConnectionId.load();
        dest["primary"]             = src.Primary;
        dest["secondary"]           = src.Secondary;
        dest["currentConnection"]   = src.current();
    }
#pragma endregion
} // namespace siddiqsoft

#endif // !COSMOS_CONNECTION_HPP
