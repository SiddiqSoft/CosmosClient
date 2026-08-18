# Feature Overview

`CosmosClient` simplifies interacting with Azure Cosmos DB via REST by removing boilerplate code for token generation, URL construction, headers, HTTP serialization, and pagination.

---

## Core Capabilities

### 1. Connection & Endpoint Management
- Parse standard Azure Cosmos DB connection strings (e.g. from Azure Portal).
- Auto-extract Base64 AccountKey, master keys, and BaseUri.
- Support for multiple read and write failover regions (`ReadableUris`, `WritableUris`).

### 2. Document CRUD Operations
- **Create**: Add new documents with specified partition keys.
- **Read**: Fetch documents by ID and PartitionKey.
- **Replace & Upsert**: Update existing documents atomically.
- **Delete**: Remove documents by ID and PartitionKey.

### 3. SQL Querying & Iteration
- Full support for SQL syntax (`SELECT * FROM c WHERE ...`).
- Parameterized queries with JSON array bindings.
- Paged querying with continuation tokens via `CosmosIterableResponseType`.

### 4. Database & Container Administration
- Create, list, inspect, and delete Cosmos databases.
- Create, list, inspect, and delete Cosmos containers (collections).
- Execute Cosmos DB stored procedures.

### 5. Asynchronous Operations
- Non-blocking execution via `sendAsync`.
- Integrated thread-pool executor for concurrency.
- Callback-based completion handling.

---

## Next Steps

- Learn about [**Connection & Endpoint**](connection.md) configuration.
- Discover [**Document Operations**](documents.md).
- Explore [**SQL Querying & Iteration**](queries.md).
- Read about [**Asynchronous Operations**](async.md).
