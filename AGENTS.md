# AgentGPT

Next.js 13 app for assembling and deploying autonomous AI agents in the browser.

## Local / Cloud setup

```bash
bash .cursor/install.sh
npm run dev
```

- App: http://localhost:3000
- Tests: `npm test`
- Lint: `npm run lint`

The install script writes a gitignored `.env`, switches Prisma from MySQL to SQLite in-place, installs npm packages, and pushes the schema. Do not commit `prisma/schema.prisma` after that conversion.

## Cursor Cloud specific instructions

- Mock mode is on by default (`NEXT_PUBLIC_FF_MOCK_MODE_ENABLED=true`) so the UI works without an OpenAI key.
- Set `OPENAI_API_KEY` as a Cursor secret to run real agents.
- Auth and Stripe stay off unless you add the matching secrets (`GOOGLE_*`, `GITHUB_*`, `DISCORD_*`, `STRIPE_*`).
- Dev server: `npm run dev` on port 3000.
- After a git checkout, `bash .cursor/prepare-db.sh` re-applies SQLite and regenerates the Prisma client.
