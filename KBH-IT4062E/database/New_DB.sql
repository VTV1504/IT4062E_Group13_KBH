-- kbh_schema.sql (PostgreSQL)
-- Minimal DB for Keyboard Heroes backend:
-- - accounts (sign in / create / change password)
-- - paragraphs (random paragraph for game_init)
-- - results (save + leaderboard)

BEGIN;

-- For password hashing with crypt()/gen_salt()
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- =========================
-- 1) USERS
-- =========================
DROP TABLE IF EXISTS game_result CASCADE;
DROP TABLE IF EXISTS paragraph CASCADE;
DROP TABLE IF EXISTS app_user CASCADE;

CREATE TABLE app_user (
  user_id       BIGSERIAL PRIMARY KEY,
  username      TEXT NOT NULL,
  password_hash TEXT NOT NULL,
  created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Case-insensitive uniqueness (snake_case convention everywhere)
CREATE UNIQUE INDEX ux_app_user_username_ci ON app_user (lower(username));

-- =========================
-- 2) PARAGRAPHS
-- =========================
CREATE TABLE paragraph (
  paragraph_id BIGSERIAL PRIMARY KEY,
  body         TEXT NOT NULL,
  language     TEXT NOT NULL DEFAULT 'en',
  is_active    BOOLEAN NOT NULL DEFAULT TRUE,
  created_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX ix_paragraph_active ON paragraph (is_active);

-- =========================
-- 3) RESULTS (for leaderboard)
-- =========================
CREATE TABLE game_result (
  result_id       BIGSERIAL PRIMARY KEY,
  user_id         BIGINT REFERENCES app_user(user_id) ON DELETE SET NULL,
  paragraph_id    BIGINT REFERENCES paragraph(paragraph_id) ON DELETE SET NULL,

  wpm             DOUBLE PRECISION NOT NULL CHECK (wpm >= 0),
  accuracy        DOUBLE PRECISION NOT NULL CHECK (accuracy >= 0 AND accuracy <= 100),

  duration_ms     INT NOT NULL CHECK (duration_ms > 0),
  words_committed INT NOT NULL CHECK (words_committed >= 0),

  created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX ix_game_result_user_time ON game_result (user_id, created_at DESC);
CREATE INDEX ix_game_result_wpm ON game_result (wpm DESC, accuracy DESC, created_at ASC);

-- =========================
-- 4) LEADERBOARD VIEW (best run per user)
-- =========================
DROP VIEW IF EXISTS v_user_best;
CREATE VIEW v_user_best AS
SELECT DISTINCT ON (r.user_id)
  r.user_id,
  u.username,
  r.wpm,
  r.accuracy,
  r.duration_ms,
  r.words_committed,
  r.created_at
FROM game_result r
JOIN app_user u ON u.user_id = r.user_id
WHERE r.user_id IS NOT NULL
ORDER BY r.user_id, r.wpm DESC, r.accuracy DESC, r.created_at ASC;

-- =========================
-- 5) OPTIONAL HELPERS (keeps backend simpler)
-- =========================

-- Create user (returns user_id). Fails on duplicate username (case-insensitive).
CREATE OR REPLACE FUNCTION kbh_create_user(p_username TEXT, p_password TEXT)
RETURNS BIGINT
LANGUAGE plpgsql
AS $$
DECLARE
  new_id BIGINT;
BEGIN
  INSERT INTO app_user(username, password_hash)
  VALUES (p_username, crypt(p_password, gen_salt('bf')))
  RETURNING user_id INTO new_id;

  RETURN new_id;
END $$;

-- Authenticate (returns user_id, username) or no rows if invalid.
CREATE OR REPLACE FUNCTION kbh_authenticate(p_username TEXT, p_password TEXT)
RETURNS TABLE(user_id BIGINT, username TEXT)
LANGUAGE sql
AS $$
  SELECT u.user_id, u.username
  FROM app_user u
  WHERE lower(u.username) = lower(p_username)
    AND u.password_hash = crypt(p_password, u.password_hash)
  LIMIT 1;
$$;

-- Change password (returns true if updated).
CREATE OR REPLACE FUNCTION kbh_change_password(p_username TEXT, p_old TEXT, p_new TEXT)
RETURNS BOOLEAN
LANGUAGE plpgsql
AS $$
DECLARE
  updated_count INT;
BEGIN
  UPDATE app_user u
  SET password_hash = crypt(p_new, gen_salt('bf'))
  WHERE lower(u.username) = lower(p_username)
    AND u.password_hash = crypt(p_old, u.password_hash);

  GET DIAGNOSTICS updated_count = ROW_COUNT;
  RETURN updated_count = 1;
END $$;

-- Get random active paragraph (small + simple).
CREATE OR REPLACE FUNCTION kbh_random_paragraph()
RETURNS TABLE(paragraph_id BIGINT, body TEXT, language TEXT)
LANGUAGE sql
AS $$
  SELECT p.paragraph_id, p.body, p.language
  FROM paragraph p
  WHERE p.is_active = TRUE
  ORDER BY random()
  LIMIT 1;
$$;

-- =========================
-- 6) SEED PARAGRAPHS (minimal)
-- =========================
INSERT INTO paragraph(body, language) VALUES
('The quick brown fox jumps over the lazy dog. Practice makes perfect, so keep typing steadily.', 'en'),
('Typing fast is good, but typing accurately is better. Focus on rhythm and consistency.', 'en'),
('Multiplayer typing is a race of focus. Breathe, commit each word with space, and stay calm.', 'en');

COMMIT;

-- =========================
-- QUICK QUERIES BACKEND WILL USE (examples)
-- =========================
-- Create account:
--   SELECT kbh_create_user($1, $2);
--
-- Sign in:
--   SELECT * FROM kbh_authenticate($1, $2);
--
-- Change password:
--   SELECT kbh_change_password($1, $2, $3);
--
-- Get paragraph for game_init:
--   SELECT * FROM kbh_random_paragraph();
--
-- Save result (backend inserts after game_end for logged-in users):
--   INSERT INTO game_result(user_id, paragraph_id, wpm, accuracy, duration_ms, words_committed)
--   VALUES ($1, $2, $3, $4, $5, $6);
--
-- Top 15 leaderboard:
--   SELECT *, dense_rank() OVER (ORDER BY wpm DESC, accuracy DESC, created_at ASC) AS rank
--   FROM v_user_best
--   ORDER BY rank
--   LIMIT 15;
--
-- Self rank:
--   SELECT rank FROM (
--     SELECT user_id,
--            dense_rank() OVER (ORDER BY wpm DESC, accuracy DESC, created_at ASC) AS rank
--     FROM v_user_best
--   ) t
--   WHERE t.user_id = $1;
