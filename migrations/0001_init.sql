-- Migration: 0001_init
-- Creates core identity tables: users, api_tokens, board, schema_migrations.
-- All timestamps are TIMESTAMPTZ. All primary keys are UUID v4.

BEGIN;

-- ── Migration tracking ────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS schema_migrations (
    version     TEXT        NOT NULL PRIMARY KEY,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Users ─────────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS users (
    id            UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    username      TEXT        NOT NULL UNIQUE,
    password_hash TEXT        NOT NULL,   -- argon2id encoded hash
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE UNIQUE INDEX IF NOT EXISTS users_username_idx ON users (username);

-- ── API Tokens ────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS api_tokens (
    id           UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    user_id      UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name         TEXT        NOT NULL,
    token_hash   TEXT        NOT NULL UNIQUE,   -- SHA-256 hex of the plaintext token
    last_used_at TIMESTAMPTZ,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    revoked_at   TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS api_tokens_user_idx ON api_tokens (user_id);
CREATE INDEX IF NOT EXISTS api_tokens_hash_idx ON api_tokens (token_hash);

-- ── Board (single-row, enforced by constraint) ────────────────────────────────
CREATE TABLE IF NOT EXISTS board (
    id                  UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    name                TEXT        NOT NULL,
    hostname            TEXT,
    model               TEXT,
    os_version          TEXT,
    hal_version         TEXT,
    client_version      TEXT,
    tags                JSONB       NOT NULL DEFAULT '{}',
    hal_module_inventory JSONB      NOT NULL DEFAULT '[]',
    first_seen          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_seen           TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    status              TEXT        NOT NULL DEFAULT 'offline'  -- online | offline
        CHECK (status IN ('online', 'offline')),
    -- Enforce single-board constraint
    CONSTRAINT board_single_row CHECK (true)
);

-- Allow only one row in the board table.
CREATE UNIQUE INDEX IF NOT EXISTS board_single_row_idx ON board ((TRUE));

INSERT INTO schema_migrations (version) VALUES ('0001_init')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN (for reference — not auto-applied):
-- DROP TABLE IF EXISTS api_tokens;
-- DROP TABLE IF EXISTS users;
-- DROP TABLE IF EXISTS board;
-- DROP TABLE IF EXISTS schema_migrations;
