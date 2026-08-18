# Document Operations

`CosmosClient` provides full CRUD operations for Azure Cosmos DB documents using clean C++ aggregate initialization (`CosmosArgumentType`) and standard `nlohmann::json`.

---

## Create Document

```cpp
auto resp = client.createDocument({
    .database     = "mydb",
    .collection   = "items",
    .partitionKey = "category-1",
    .document     = {
        {"id", "doc-1001"},
        {"category-1", "electronics"},
        {"name", "Smart Sensor"},
        {"price", 29.99}
    }
});

if (resp.statusCode == 201) {
    std::cout << "Document created! RUs consumed: " << resp.requestCharge << std::endl;
}
```

---

## Read Document

```cpp
auto resp = client.readDocument({
    .database     = "mydb",
    .collection   = "items",
    .id           = "doc-1001",
    .partitionKey = "category-1"
});

if (resp.statusCode == 200) {
    std::cout << "Document name: " << resp.document["name"] << std::endl;
}
```

---

## Replace Document

```cpp
auto resp = client.replaceDocument({
    .database     = "mydb",
    .collection   = "items",
    .id           = "doc-1001",
    .partitionKey = "category-1",
    .document     = {
        {"id", "doc-1001"},
        {"category-1", "electronics"},
        {"name", "Smart Sensor v2"},
        {"price", 34.99}
    }
});

if (resp.statusCode == 200) {
    std::cout << "Document updated!" << std::endl;
}
```

---

## Delete Document

```cpp
auto resp = client.deleteDocument({
    .database     = "mydb",
    .collection   = "items",
    .id           = "doc-1001",
    .partitionKey = "category-1"
});

if (resp.statusCode == 204) {
    std::cout << "Document deleted successfully." << std::endl;
}
```
