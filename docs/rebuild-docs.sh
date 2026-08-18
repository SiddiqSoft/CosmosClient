#!/bin/bash
set -e
cd "$(dirname "$0")/.."
python3 scripts/generate_dependencies_md.py
mkdocs build
