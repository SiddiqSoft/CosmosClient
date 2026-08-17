# Connection & Endpoint Management

`CosmosClient` parses Azure Cosmos DB connection strings directly, handling endpoint splitting, key decoding, HMAC-SHA256 authorization token calculation, and multi-region failover.

---

## Connection Configuration

The client is configured using `nlohmann::json` parameters:

```cpp
#include "nlohmann/json.hpp"
#include "siddiqsoft/cosmoscl.hpp"

using namespace siddiqsoft;

int main()
{
    CosmosClient client;

    client.configure({
        {"connectionStrings", {
            "AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=dGhpcyBpcyBhIHNhbXBsZSBrZXk=;",
            "AccountEndpoint=https://myaccount-secondary.documents.azure.com:443/;AccountKey=..."
        }},
        {"partitionKeyNames", {"/id", "/partitionKey"}}
    });

    return 0;
}
```

---

## Multi-Region Failover

`CosmosEndpoint` automatically segregates readable and writable URIs:

* **Primary Writable URI**: Used for write operations (create, replace, delete).
* **Readable URIs**: Round-robined for read and query operations.

```cpp
// Inspect active endpoint info
std::cout << "Primary Base URI: " << client.primaryEndpoint().BaseUri << std::endl;
```

---

## Automatic Authentication Token Generation

Azure Cosmos DB REST API requires a master key signature in the `Authorization` header for every request:

$$\text{Signature} = \text{HMAC-SHA256}(\text{Key}, \text{Verb} + \text{ResourceType} + \text{ResourceLink} + \text{Date})$$

`CosmosClient` builds and encodes this authorization header dynamically for each REST call, ensuring seamless access.

---

## HTTP Payload Debug Tracing (`restcl_DEBUG_TRACE`)

To inspect raw HTTP request headers, REST verbs, endpoints, and response payloads during troubleshooting, build your application with the CMake option `restcl_DEBUG_TRACE=ON`:

```bash
cmake -B build -Drestcl_DEBUG_TRACE=ON
```

When enabled, `restcl` outputs detailed diagnostics to `std::cerr` for every outgoing REST request and incoming HTTP response.

---

## Azure Cosmos DB Emulator Configuration

When running against a local Azure Cosmos DB Emulator on `localhost:8081`:

1. **Endpoint Scheme Protocol (`http://` vs `https://`)**:
   - **Azure Cosmos DB vNext Linux Emulator** (`mcr.microsoft.com/cosmosdb/linux/azure-cosmos-emulator:vnext-latest`): Serves plain **HTTP** on port 8081 (`AccountEndpoint=http://localhost:8081/;`). Attempting an `https://` TLS handshake against port 8081 will fail with an SSL protocol error (`SSL connect error`).
   - **Windows / Classic Emulator**: Serves **HTTPS** on port 8081 with a self-signed TLS certificate (`AccountEndpoint=https://localhost:8081/;`).

2. **Default Emulator Connection String**:
   ```cpp
   client.configure({
       {"connectionStrings", {
           "AccountEndpoint=http://localhost:8081/;AccountKey=C2y6yDjf5/R+ob0N8A7Cgv30VRDJIWEHLM+4QDU5DE2nQ9nDuVTqobD4b8mGGyPMbIZnqyMsEcaGQy67XIw/Jw==;"
       }},
       {"partitionKeyNames", {"/id"}}
   });
   ```

3. **SSL Certificate Peer Verification**:
   When connecting to HTTPS emulators with self-signed certificates, ensure peer verification is disabled (`"verifyPeer": 0L`) or import `emulator.pem` into your local CA trust store.

