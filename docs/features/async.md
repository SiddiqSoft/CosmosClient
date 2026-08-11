# Asynchronous Operations

`CosmosClient` features integrated async dispatch powered by `asynchrony` and simple thread pools, allowing database operations to execute without blocking caller threads.

---

## `sendAsync` Interface

The primary method for async dispatch is `sendAsync`:

```cpp
#include <iostream>
#include <atomic>
#include <thread>
#include "siddiqsoft/cosmoscl.hpp"

using namespace siddiqsoft;

int main()
{
    CosmosClient client;
    client.configure({
        {"connectionStrings", {"AccountEndpoint=https://myaccount.documents.azure.com:443/;AccountKey=...;"}},
        {"partitionKeyNames", {"/id"}}
    });

    std::atomic<bool> completed{false};

    // Async Read Document
    client.sendAsync(
        CosmosOperation::ReadDocument,
        {
            .database     = "mydb",
            .collection   = "items",
            .id           = "doc-101",
            .partitionKey = "doc-101"
        },
        [&completed](CosmosResponseType resp) {
            if (resp.statusCode == 200) {
                std::cout << "Async read success! RU: " << resp.requestCharge << std::endl;
            } else {
                std::cerr << "Async read error status: " << resp.statusCode << std::endl;
            }
            completed = true;
        }
    );

    // Wait for completion
    while (!completed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
```

---

## Async Operations Enum

You can pass any `CosmosOperation` to `sendAsync`:

* `CosmosOperation::GetDatabase`
* `CosmosOperation::ListDatabases`
* `CosmosOperation::CreateDatabase`
* `CosmosOperation::DeleteDatabase`
* `CosmosOperation::GetContainer`
* `CosmosOperation::ListContainers`
* `CosmosOperation::CreateContainer`
* `CosmosOperation::DeleteContainer`
* `CosmosOperation::CreateDocument`
* `CosmosOperation::ReadDocument`
* `CosmosOperation::ReplaceDocument`
* `CosmosOperation::DeleteDocument`
* `CosmosOperation::QueryDocuments`
* `CosmosOperation::ExecuteStoredProcedure`
