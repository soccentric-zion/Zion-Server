-- Migration: 0005_ota
-- Creates the ota_upgrades table.

BEGIN;

CREATE TABLE IF NOT EXISTS ota_upgrades (
    id                UUID        NOT NULL DEFAULT gen_random_uuid() PRIMARY KEY,
    artifact_path     TEXT        NOT NULL,
    artifact_hash     TEXT        NOT NULL,   -- SHA-256 hex
    artifact_size     BIGINT      NOT NULL DEFAULT 0,
    from_version      TEXT        NOT NULL DEFAULT '',
    to_version        TEXT        NOT NULL DEFAULT '',
    status            TEXT        NOT NULL DEFAULT 'uploaded'
        CHECK (status IN (
            'uploaded',    -- artifact present, not yet triggered
            'pending',     -- queued for push
            'pushing',     -- actively sending to board
            'verifying',   -- board verifying the image
            'rebooting',   -- board rebooting into new image
            'completed',   -- successfully applied
            'failed',      -- push/verify/reboot failed
            'rolled_back'  -- rollback completed
        )),
    -- Progress 0-100 (board-reported during push/verify phases).
    progress          INTEGER     NOT NULL DEFAULT 0 CHECK (progress BETWEEN 0 AND 100),
    started_at        TIMESTAMPTZ,
    ended_at          TIMESTAMPTZ,
    error_message     TEXT,
    -- Self-referential: set when this upgrade was created as a rollback from another.
    rolled_back_from  UUID        REFERENCES ota_upgrades(id) ON DELETE SET NULL,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_by        UUID        REFERENCES users(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS ota_upgrades_status_idx     ON ota_upgrades (status);
CREATE INDEX IF NOT EXISTS ota_upgrades_created_at_idx ON ota_upgrades (created_at DESC);

INSERT INTO schema_migrations (version) VALUES ('0005_ota')
    ON CONFLICT (version) DO NOTHING;

COMMIT;

-- DOWN:
-- DROP TABLE IF EXISTS ota_upgrades;
