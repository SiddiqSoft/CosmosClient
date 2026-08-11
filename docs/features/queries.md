# SQL Querying & Iteration

`CosmosClient` allows executing parameterized SQL queries against Azure Cosmos DB containers and iterating over multi-page query results using continuation tokens.

---

## Simple Parameterized Query

```cpp
auto resp = client.queryDocuments({
    .database   = "mydb",
    .collection = "items",
    .query      = "SELECT * FROM c WHERE c.price > @minPrice AND c.category = @cat",
    .parameters = {
        {{"name", "@minPrice"}, {"value", 20.0}},
        {{"name", "@cat"}, {"value", "electronics"}}
    }
});

if (resp.statusCode == 200) {
    for (const auto& item : resp.document["Documents"]) {
        std::cout << "Match: " << item["id"] << " - " << item["name"] << std::endl;
    }
}
```

---

## Paged Continuation Iterator

For large query result sets, use `queryDocumentsIterable` or manual continuation tokens:

```cpp
std::string continuationToken = "";

do {
    auto resp = client.queryDocuments({
        .database          = "mydb",
        .collection        = "items",
        .query             = "SELECT * FROM c",
        .continuationToken = continuationToken
    });

    if (resp.statusCode == 200) {
        for (const auto& doc : resp.document["Documents"]) {
            std::cout << "Item: " << doc["id"] << std::endl;
        }

        // Get continuation token for next page
        continuationToken = resp.continuationToken;
    } else {
        break;
    }
} while (!continuationToken.empty());
```

---

## Helper Iterable Response

`CosmosIterableResponseType` simplifies page traversal:

```cpp
CosmosIterableResponseType iterResp = client.queryDocumentsIterable({
    .database   = "mydb",
    .collection = "items",
    .query      = "SELECT * FROM c WHERE c.active = true"
});

while (iterResp.hasNext()) {
    auto page = iterResp.next();
    for (const auto& doc : page.document["Documents"]) {
        std::cout << "Doc: " << doc["id"] << std::endl;
    }
}
```
