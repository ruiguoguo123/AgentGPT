#!/bin/bash
# Switch the Prisma schema from MySQL/Postgres to SQLite for local development.
# Idempotent: safe to run more than once.
cd "$(dirname "$0")" || exit 1

python3 - << 'PY'
from pathlib import Path

path = Path("schema.prisma")
text = path.read_text()
updated = (
    text.replace("mysql", "sqlite")
    .replace("postgres", "sqlite")
    .replace("@db.Text", "")
)
if updated != text:
    path.write_text(updated)
    print("Converted Prisma schema to SQLite")
else:
    print("Prisma schema already uses SQLite")
PY
