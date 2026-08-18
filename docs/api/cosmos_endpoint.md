# `CosmosEndpoint` & `CosmosConnection` Reference

Defined in `siddiqsoft/cosmoscl.hpp` inside namespace `siddiqsoft`.

---

## `CosmosEndpoint` Struct

```cpp
struct CosmosEndpoint {
    std::basic_string<char> BaseUri{};
    std::basic_string<char> EncodedKey{};
    std::string Key{};
    std::vector<std::basic_string<char>> ReadableUris{};
    std::atomic<size_t> CurrentReadUriId{0};
    std::vector<std::basic_string<char>> WritableUris{};
    std::atomic<size_t> CurrentWriteUriId{0};

    CosmosEndpoint() = default;
    CosmosEndpoint(const CosmosEndpoint& src);
    CosmosEndpoint& operator=(const CosmosEndpoint& src);
    CosmosEndpoint(CosmosEndpoint&& src) noexcept;
    CosmosEndpoint& operator=(CosmosEndpoint&& src) noexcept;

    explicit operator bool() const;
    explicit operator std::basic_string<char>() const;
    std::basic_string<char> string() const;

    const auto& currentReadUri() const;
    const auto& currentWriteUri() const;
    CosmosEndpoint& rotateReadUri();
    CosmosEndpoint& rotateWriteUri();
};
```

### Fields & Methods

| Member | Type | Description |
| :--- | :--- | :--- |
| `BaseUri` | `std::basic_string<char>` | Base HTTPS URI of the Cosmos DB account. |
| `EncodedKey` | `std::basic_string<char>` | Base64-encoded account master key. |
| `Key` | `std::string` | Binary-decoded account key used for HMAC-SHA256 signature. |
| `ReadableUris` | `std::vector<std::basic_string<char>>` | Read-region URIs for failover / geo-replication. |
| `CurrentReadUriId` | `std::atomic<size_t>` | Atomic index of currently selected read URI. |
| `WritableUris` | `std::vector<std::basic_string<char>>` | Write-region URIs. |
| `CurrentWriteUriId` | `std::atomic<size_t>` | Atomic index of currently selected write URI. |

---

## `CosmosConnection` Struct

```cpp
struct CosmosConnection {
    enum class CurrentConnectionIdType : uint16_t {
        PrimaryConnection   = 1,
        SecondaryConnection = 2
    };

    std::atomic<CurrentConnectionIdType> CurrentConnectionId{CurrentConnectionIdType::PrimaryConnection};
    CosmosEndpoint Primary{};
    CosmosEndpoint Secondary{};

    CosmosConnection() = default;
    CosmosConnection(const CosmosConnection& src);
    CosmosConnection& operator=(const CosmosConnection& src);
    CosmosConnection(CosmosConnection&& src) noexcept;
    CosmosConnection& operator=(CosmosConnection&& src) noexcept;

    CosmosConnection& configure(const nlohmann::json& config);
    const CosmosEndpoint& current() const;
    CosmosConnection& rotate(const uint16_t c = 0);
};
```

### Fields & Methods

| Member | Type | Description |
| :--- | :--- | :--- |
| `CurrentConnectionId` | `std::atomic<CurrentConnectionIdType>` | Atomic active connection selector (Primary vs Secondary). |
| `Primary` | `CosmosEndpoint` | Primary connection endpoint info. |
| `Secondary` | `CosmosEndpoint` | Secondary connection endpoint info. |
| `current()` | `const CosmosEndpoint&` | Thread-safe accessor returning current active endpoint. |
| `rotate(uint16_t)` | `CosmosConnection&` | Atomically rotates connection active state. |

