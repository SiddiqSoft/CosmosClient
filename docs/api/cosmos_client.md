# `CosmosClient` Class Reference

Defined in `siddiqsoft/cosmoscl.hpp` inside namespace `siddiqsoft`.

```cpp
class CosmosClient;
```

---

## Member Functions

### Configuration & Connection

#### `configure`
```cpp
CosmosConnection configure(const nlohmann::json& config);
```
Configures the client with connection strings and partition key names.

#### `primaryEndpoint`
```cpp
const CosmosEndpoint& primaryEndpoint() const;
```
Returns the configured `CosmosEndpoint`.

---

## Document Operations

#### `createDocument`
```cpp
CosmosResponseType createDocument(const CosmosArgumentType& arg);
```
Creates a new document in the specified database and container.

#### `readDocument`
```cpp
CosmosResponseType readDocument(const CosmosArgumentType& arg);
```
Reads a document by `id` and `partitionKey`.

#### `replaceDocument`
```cpp
CosmosResponseType replaceDocument(const CosmosArgumentType& arg);
```
Replaces an existing document.

#### `deleteDocument`
```cpp
CosmosResponseType deleteDocument(const CosmosArgumentType& arg);
```
Deletes a document by `id` and `partitionKey`.

---

## Query Operations

#### `queryDocuments`
```cpp
CosmosResponseType queryDocuments(const CosmosArgumentType& arg);
```
Executes a SQL query against the container.

#### `queryDocumentsIterable`
```cpp
CosmosIterableResponseType queryDocumentsIterable(const CosmosArgumentType& arg);
```
Returns an iterable response structure for navigating multi-page query results.

---

## Database Operations

#### `createDatabase` / `getDatabase` / `listDatabases` / `deleteDatabase`
```cpp
CosmosResponseType createDatabase(const CosmosArgumentType& arg);
CosmosResponseType getDatabase(const CosmosArgumentType& arg);
CosmosResponseType listDatabases(const CosmosArgumentType& arg);
CosmosResponseType deleteDatabase(const CosmosArgumentType& arg);
```

---

## Container Operations

#### `createContainer` / `getContainer` / `listContainers` / `deleteContainer`
```cpp
CosmosResponseType createContainer(const CosmosArgumentType& arg);
CosmosResponseType getContainer(const CosmosArgumentType& arg);
CosmosResponseType listContainers(const CosmosArgumentType& arg);
CosmosResponseType deleteContainer(const CosmosArgumentType& arg);
```

---

## Asynchronous Execution

#### `sendAsync`
```cpp
void sendAsync(
    CosmosOperation op,
    CosmosArgumentType arg,
    std::function<void(CosmosResponseType)> callback
);
```
Dispatches the operation to a background worker thread pool and invokes `callback` upon completion.
