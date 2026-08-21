#!/usr/bin/env bash
# Idempotent bootstrap for local and Cursor Cloud Builds.
# Installs Node deps and prepares a SQLite database. Does not start servers.
set -euo pipefail
cd "$(dirname "$0")/.." || exit 1

# .env is required before `prisma generate` (npm postinstall).
if [ ! -f .env ]; then
  NEXTAUTH_SECRET="${NEXTAUTH_SECRET:-$(openssl rand -base64 32)}"
  cat > .env << EOF
NODE_ENV=development
NEXTAUTH_SECRET=${NEXTAUTH_SECRET}
NEXTAUTH_URL=${NEXTAUTH_URL:-http://localhost:3000}
DATABASE_URL=${DATABASE_URL:-file:./db.sqlite}
OPENAI_API_KEY=${OPENAI_API_KEY:-changeme}
NEXT_PUBLIC_VERCEL_ENV=development
NEXT_PUBLIC_FF_MOCK_MODE_ENABLED=${NEXT_PUBLIC_FF_MOCK_MODE_ENABLED:-true}
NEXT_PUBLIC_FF_AUTH_ENABLED=${NEXT_PUBLIC_FF_AUTH_ENABLED:-false}
NEXT_PUBLIC_FF_SUB_ENABLED=${NEXT_PUBLIC_FF_SUB_ENABLED:-false}
EOF
  echo "Wrote .env (OPENAI_API_KEY defaults to changeme; set a Cursor secret for real agents)"
fi

./prisma/useSqlite.sh
HUSKY=0 npm ci
bash .cursor/prepare-db.sh
