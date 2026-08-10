-- Migration: 0007_notifications
-- Creates the notifications table.

BEGIN;

CREATE TABLE IF NOT EXISTS notifications (
    id         UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    -- Kind: run.completed | run.failed | test.failure | ota.completed | ...
    kind       TEXT        NOT NULL,
    -- JSON payload carrying context (run_id, module_id, message, etc.)
    payload    JSONB       NOT NULL DEFAULT '{}',
    read_at    TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS notifications_read_at_idx
    ON notifications (read_at) WHERE read_at IS NULL;

CREATE INDEX IF NOT EXISTS notifications_created_at_idx
    ON notifications (created_at DESC);

CREATE INDEX IF NOT EXISTS notifications_kind_idx
    ON notifications (kind);

INSERT INTO schema_migrations (version) VALUES ('0007_notifications')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN:
-- DROP TABLE IF EXISTS notifications;
