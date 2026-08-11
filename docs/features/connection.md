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
