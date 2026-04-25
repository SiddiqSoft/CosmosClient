#!/usr/bin/env python3
"""
Mock Azure Cosmos DB REST API server for offline testing.

Simulates the Cosmos DB REST API endpoints with in-memory storage.
Listens on http://127.0.0.1:<port> (default 18081).

Usage:
    python3 mock_cosmos_server.py [port]

The server writes "READY" to stdout once it's listening, then runs until killed.
"""

import json
import sys
import time
import uuid
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, unquote


class CosmosState:
    """In-memory Cosmos DB state."""

    def __init__(self):
        self.lock = threading.Lock()
        # databases: { dbName: { collections: { collName: { documents: { docId: doc } } } } }
        self.databases = {}

    def list_databases(self):
        with self.lock:
            return [{"id": name, "_rid": f"rid-{name}", "_self": f"dbs/{name}/"}
                    for name in self.databases]

    def create_database(self, db_id):
        with self.lock:
            if db_id in self.databases:
                return None, 409  # Conflict
            self.databases[db_id] = {"collections": {}}
            return {"id": db_id, "_rid": f"rid-{db_id}", "_self": f"dbs/{db_id}/"}, 201

    def find_database(self, db_id):
        with self.lock:
            if db_id in self.databases:
                return {"id": db_id, "_rid": f"rid-{db_id}", "_self": f"dbs/{db_id}/"}, 200
            return None, 404

    def delete_database(self, db_id):
        with self.lock:
            if db_id in self.databases:
                del self.databases[db_id]
                return None, 204
            return None, 404

    def list_collections(self, db_id):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            colls = [{"id": name, "_rid": f"rid-{name}", "_self": f"dbs/{db_id}/colls/{name}/"}
                     for name in db["collections"]]
            return colls, 200

    def create_collection(self, db_id, coll_data):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll_id = coll_data.get("id", "")
            if coll_id in db["collections"]:
                return None, 409
            db["collections"][coll_id] = {"documents": {}}
            return {"id": coll_id, "_rid": f"rid-{coll_id}",
                    "_self": f"dbs/{db_id}/colls/{coll_id}/",
                    "partitionKey": coll_data.get("partitionKey", {})}, 201

    def list_documents(self, db_id, coll_id):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll = db["collections"].get(coll_id)
            if coll is None:
                return None, 404
            docs = list(coll["documents"].values())
            return docs, 200

    def create_document(self, db_id, coll_id, doc, upsert=False):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll = db["collections"].get(coll_id)
            if coll is None:
                return None, 404
            doc_id = doc.get("id", "")
            if not doc_id:
                return {"message": "Missing id"}, 400
            if doc_id in coll["documents"] and not upsert:
                return {"message": "Conflict"}, 409
            # Add Cosmos metadata
            enriched = dict(doc)
            ts = int(time.time())
            enriched.setdefault("_rid", f"rid-{doc_id[:8]}")
            enriched.setdefault("_self", f"dbs/{db_id}/colls/{coll_id}/docs/{doc_id}/")
            enriched.setdefault("_etag", f'"{uuid.uuid4()}"')
            enriched.setdefault("_ts", ts)
            enriched.setdefault("_attachments", "attachments/")
            coll["documents"][doc_id] = enriched
            return enriched, 201

    def find_document(self, db_id, coll_id, doc_id):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll = db["collections"].get(coll_id)
            if coll is None:
                return None, 404
            doc = coll["documents"].get(doc_id)
            if doc is None:
                return None, 404
            return doc, 200

    def update_document(self, db_id, coll_id, doc_id, doc):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll = db["collections"].get(coll_id)
            if coll is None:
                return None, 404
            if doc_id not in coll["documents"]:
                return None, 404
            enriched = dict(doc)
            ts = int(time.time())
            enriched["_rid"] = f"rid-{doc_id[:8]}"
            enriched["_self"] = f"dbs/{db_id}/colls/{coll_id}/docs/{doc_id}/"
            enriched["_etag"] = f'"{uuid.uuid4()}"'
            enriched["_ts"] = ts
            enriched["_attachments"] = "attachments/"
            coll["documents"][doc_id] = enriched
            return enriched, 200

    def delete_document(self, db_id, coll_id, doc_id):
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll = db["collections"].get(coll_id)
            if coll is None:
                return None, 404
            if doc_id not in coll["documents"]:
                return None, 404
            del coll["documents"][doc_id]
            return None, 204

    def query_documents(self, db_id, coll_id, query_body, partition_key=None):
        """Simple query support - returns all docs, optionally filtered by partition key."""
        with self.lock:
            db = self.databases.get(db_id)
            if not db:
                return None, 404
            coll = db["collections"].get(coll_id)
            if coll is None:
                return None, 404

            docs = list(coll["documents"].values())

            # Simple parameter substitution for contains() and equality queries
            query_str = query_body.get("query", "")
            params = {p["name"]: p["value"] for p in query_body.get("parameters", [])}

            # Try to apply simple filters
            filtered = self._apply_filter(docs, query_str, params, partition_key)
            return filtered, 200

    def _apply_filter(self, docs, query_str, params, partition_key):
        """Very basic SQL-like filter for common test patterns."""
        import re

        # Handle: SELECT * FROM c WHERE contains(c.<field>, @v1)
        contains_match = re.search(r'contains\(c\.(\w+),\s*(@\w+)\)', query_str, re.IGNORECASE)
        # Handle: SELECT * FROM c WHERE c.<field> = @v1
        eq_match = re.search(r'c\.(\w+)\s*=\s*(@\w+)', query_str, re.IGNORECASE)

        filters = []
        if contains_match:
            field, param = contains_match.groups()
            if param in params:
                val = params[param]
                filters.append(lambda d, f=field, v=val: v in str(d.get(f, "")))

        if eq_match:
            field, param = eq_match.groups()
            if param in params:
                val = params[param]
                filters.append(lambda d, f=field, v=val: d.get(f) == v)

        # Handle multi-condition: ... and c.<field2> = @v2
        and_parts = re.split(r'\s+and\s+', query_str, flags=re.IGNORECASE)
        for part in and_parts:
            m = re.search(r'c\.(\w+)\s*=\s*(@\w+)', part.strip())
            if m:
                field, param = m.groups()
                if param in params and not any(True for f in filters if False):
                    val = params[param]
                    # Avoid duplicate
                    if eq_match and eq_match.group(1) == field and eq_match.group(2) == param:
                        continue
                    filters.append(lambda d, f=field, v=val: d.get(f) == v)

        if not filters:
            return docs

        result = []
        for doc in docs:
            if all(f(doc) for f in filters):
                result.append(doc)
        return result


# Global state
STATE = CosmosState()


class CosmosHandler(BaseHTTPRequestHandler):
    """HTTP request handler simulating Cosmos DB REST API."""

    def log_message(self, format, *args):
        """Suppress default logging."""
        pass

    def _send_json(self, status, body):
        self.send_response(status)
        if body is not None:
            data = json.dumps(body).encode("utf-8")
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        else:
            self.send_header("Content-Length", "0")
            self.end_headers()

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0))
        if length > 0:
            return json.loads(self.rfile.read(length))
        return {}

    def _parse_path(self):
        """Parse the URL path into segments."""
        path = urlparse(self.path).path.strip("/")
        return [unquote(s) for s in path.split("/") if s]

    def do_GET(self):
        segments = self._parse_path()

        # GET / - discoverRegions (root)
        if len(segments) == 0:
            self._send_json(200, {
                "id": "mock-cosmos",
                "writableLocations": [{"name": "Mock Region", "databaseAccountEndpoint": f"http://127.0.0.1:{self.server.server_address[1]}/"}],
                "readableLocations": [{"name": "Mock Region", "databaseAccountEndpoint": f"http://127.0.0.1:{self.server.server_address[1]}/"}],
                "userReplicationPolicy": {"maxReplicaSetSize": 4, "asyncReplication": False},
                "userConsistencyPolicy": {"defaultConsistencyLevel": "Session"},
                "_rid": "",
                "_self": "",
            })
            return

        # GET /dbs - listDatabases
        if segments == ["dbs"]:
            dbs = STATE.list_databases()
            self._send_json(200, {"Databases": dbs, "_count": len(dbs), "_rid": ""})
            return

        # GET /dbs/{db} - findDatabase
        if len(segments) == 2 and segments[0] == "dbs":
            doc, status = STATE.find_database(segments[1])
            self._send_json(status, doc)
            return

        # GET /dbs/{db}/colls - listCollections
        if len(segments) == 3 and segments[0] == "dbs" and segments[2] == "colls":
            colls, status = STATE.list_collections(segments[1])
            if status == 200:
                self._send_json(200, {"DocumentCollections": colls, "_count": len(colls), "_rid": ""})
            else:
                self._send_json(status, None)
            return

        # GET /dbs/{db}/colls/{coll}/docs - listDocuments
        if len(segments) == 5 and segments[0] == "dbs" and segments[2] == "colls" and segments[4] == "docs":
            docs, status = STATE.list_documents(segments[1], segments[3])
            if status == 200:
                self._send_json(200, {"Documents": docs, "_count": len(docs), "_rid": ""})
            else:
                self._send_json(status, None)
            return

        # GET /dbs/{db}/colls/{coll}/docs/{docId} - findDocument
        if len(segments) == 6 and segments[0] == "dbs" and segments[2] == "colls" and segments[4] == "docs":
            doc, status = STATE.find_document(segments[1], segments[3], segments[5])
            self._send_json(status, doc)
            return

        self._send_json(404, {"message": "Not found"})

    def do_POST(self):
        segments = self._parse_path()
        body = self._read_body()

        # POST /dbs - createDatabase
        if segments == ["dbs"]:
            doc, status = STATE.create_database(body.get("id", ""))
            self._send_json(status, doc)
            return

        # POST /dbs/{db}/colls - createCollection
        if len(segments) == 3 and segments[0] == "dbs" and segments[2] == "colls":
            doc, status = STATE.create_collection(segments[1], body)
            self._send_json(status, doc)
            return

        # POST /dbs/{db}/colls/{coll}/docs - createDocument or query
        if len(segments) == 5 and segments[0] == "dbs" and segments[2] == "colls" and segments[4] == "docs":
            content_type = self.headers.get("Content-Type", "")

            # Query
            if "application/query+json" in content_type:
                pk = None
                pk_header = self.headers.get("x-ms-documentdb-partitionkey")
                if pk_header:
                    try:
                        pk = json.loads(pk_header)
                        if isinstance(pk, list) and len(pk) > 0:
                            pk = pk[0]
                    except Exception:
                        pass
                docs, status = STATE.query_documents(segments[1], segments[3], body, pk)
                if status == 200:
                    self._send_json(200, {"Documents": docs, "_count": len(docs), "_rid": ""})
                else:
                    self._send_json(status, None)
                return

            # Upsert
            is_upsert = self.headers.get("x-ms-documentdb-is-upsert", "").lower() == "true"
            doc, status = STATE.create_document(segments[1], segments[3], body, upsert=is_upsert)
            self._send_json(status, doc)
            return

        self._send_json(404, {"message": "Not found"})

    def do_PUT(self):
        segments = self._parse_path()
        body = self._read_body()

        # PUT /dbs/{db}/colls/{coll}/docs/{docId} - updateDocument
        if len(segments) == 6 and segments[0] == "dbs" and segments[2] == "colls" and segments[4] == "docs":
            doc, status = STATE.update_document(segments[1], segments[3], segments[5], body)
            self._send_json(status, doc)
            return

        self._send_json(404, {"message": "Not found"})

    def do_DELETE(self):
        segments = self._parse_path()

        # DELETE /dbs/{db} - deleteDatabase
        if len(segments) == 2 and segments[0] == "dbs":
            _, status = STATE.delete_database(segments[1])
            self._send_json(status, None)
            return

        # DELETE /dbs/{db}/colls/{coll}/docs/{docId} - deleteDocument
        if len(segments) == 6 and segments[0] == "dbs" and segments[2] == "colls" and segments[4] == "docs":
            _, status = STATE.delete_document(segments[1], segments[3], segments[5])
            self._send_json(status, None)
            return

        self._send_json(404, {"message": "Not found"})


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18081
    server = HTTPServer(("127.0.0.1", port), CosmosHandler)
    # Signal readiness
    print(f"READY {port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
