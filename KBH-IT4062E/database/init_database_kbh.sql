
-- Keyboard Heroes - PostgreSQL schema
-- Generated for Group 13 - "Keyboard Heroes – The Typing Game"
-- Requires: PostgreSQL 13+

-- 0) Extensions
CREATE EXTENSION IF NOT EXISTS citext;
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- 1) ENUM types
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'language_code') THEN
        CREATE TYPE language_code     AS ENUM ('vi','en');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'game_mode') THEN
        CREATE TYPE game_mode         AS ENUM ('SELF_TRAINING','ARENA','SURVIVAL');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'visibility_type') THEN
        CREATE TYPE visibility_type   AS ENUM ('PUBLIC','PRIVATE');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'room_status') THEN
        CREATE TYPE room_status       AS ENUM ('LOBBY','READY','IN_PROGRESS','ENDED');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'session_status') THEN
        CREATE TYPE session_status    AS ENUM ('PENDING','ACTIVE','FINISHED','CANCELLED');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'elimination_reason') THEN
        CREATE TYPE elimination_reason AS ENUM ('NONE','FAILED_THRESHOLD','AFK','DISCONNECT');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'leaderboard_type') THEN
        CREATE TYPE leaderboard_type  AS ENUM ('SELF_TRAINING','SURVIVAL');
    END IF;
END $$;

-- 2) Users & profile
CREATE TABLE IF NOT EXISTS app_user (
  user_id        BIGSERIAL PRIMARY KEY,
  username       CITEXT UNIQUE NOT NULL CHECK (username ~ '^[A-Za-z0-9_.]{3,30}$'),
  email          CITEXT UNIQUE,
  password_hash  TEXT,
  is_guest       BOOLEAN NOT NULL DEFAULT FALSE,
  created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  last_login_at  TIMESTAMPTZ,
  CONSTRAINT guest_fields CHECK (
    (is_guest AND email IS NULL AND password_hash IS NULL)
    OR (NOT is_guest)
  )
);

CREATE TABLE IF NOT EXISTS user_profile (
  user_id             BIGINT PRIMARY KEY REFERENCES app_user(user_id) ON DELETE CASCADE,
  display_name        TEXT NOT NULL,
  preferred_language  language_code NOT NULL DEFAULT 'en',
  country_code        CHAR(2),
  avatar_url          TEXT
);

-- 3) Auth session
CREATE TABLE IF NOT EXISTS auth_session (
  session_id   UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  user_id      BIGINT NOT NULL REFERENCES app_user(user_id) ON DELETE CASCADE,
  conn_id      UUID UNIQUE,
  is_active    BOOLEAN NOT NULL DEFAULT TRUE,
  created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  expires_at   TIMESTAMPTZ NOT NULL,
  last_seen_at TIMESTAMPTZ,
  ip           INET
);
CREATE INDEX IF NOT EXISTS idx_auth_session_user_active ON auth_session (user_id, is_active);
CREATE INDEX IF NOT EXISTS idx_auth_session_expires ON auth_session (expires_at);

-- 4) Paragraphs
CREATE TABLE IF NOT EXISTS paragraph (
  paragraph_id  BIGSERIAL PRIMARY KEY,
  language      language_code NOT NULL,
  body          TEXT NOT NULL,
  word_count    INT GENERATED ALWAYS AS (
    coalesce(array_length(regexp_split_to_array(trim(regexp_replace(body, '\s+', ' ', 'g')), ' '), 1), 0)
  ) STORED,
  difficulty    SMALLINT CHECK (difficulty BETWEEN 1 AND 5),
  source        TEXT,
  is_active     BOOLEAN NOT NULL DEFAULT TRUE,
  created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_paragraph_lang_active ON paragraph(language, is_active);

-- 5) Rooms (multi only)
CREATE TABLE IF NOT EXISTS room (
  room_id      BIGSERIAL PRIMARY KEY,
  code         VARCHAR(8) UNIQUE NOT NULL,
  host_user_id BIGINT NOT NULL REFERENCES app_user(user_id),
  mode         game_mode NOT NULL,
  visibility   visibility_type NOT NULL DEFAULT 'PUBLIC',
  status       room_status NOT NULL DEFAULT 'LOBBY',
  language     language_code NOT NULL,
  max_players  SMALLINT NOT NULL DEFAULT 10 CHECK (max_players BETWEEN 2 AND 10),
  created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  started_at   TIMESTAMPTZ,
  ended_at     TIMESTAMPTZ,
  CHECK (mode IN ('ARENA','SURVIVAL'))
);
CREATE INDEX IF NOT EXISTS idx_room_mode_visibility_status ON room(mode, visibility, status);

CREATE TABLE IF NOT EXISTS room_member (
  room_id    BIGINT REFERENCES room(room_id) ON DELETE CASCADE,
  user_id    BIGINT REFERENCES app_user(user_id) ON DELETE CASCADE,
  is_host    BOOLEAN NOT NULL DEFAULT FALSE,
  is_ready   BOOLEAN NOT NULL DEFAULT FALSE,
  joined_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  left_at    TIMESTAMPTZ,
  PRIMARY KEY (room_id, user_id)
);
CREATE INDEX IF NOT EXISTS idx_room_member_room ON room_member(room_id);
CREATE INDEX IF NOT EXISTS idx_room_member_user ON room_member(user_id);

-- 6) Game sessions
CREATE TABLE IF NOT EXISTS game_session (
  session_id   BIGSERIAL PRIMARY KEY,
  mode         game_mode NOT NULL,
  room_id      BIGINT REFERENCES room(room_id) ON DELETE SET NULL,
  language     language_code NOT NULL,
  variant      TEXT,
  status       session_status NOT NULL DEFAULT 'PENDING',
  is_public    BOOLEAN NOT NULL DEFAULT TRUE,
  created_by   BIGINT REFERENCES app_user(user_id),
  created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  started_at   TIMESTAMPTZ,
  ended_at     TIMESTAMPTZ,
  CHECK (
    (mode='SELF_TRAINING' AND room_id IS NULL)
    OR (mode IN ('ARENA','SURVIVAL') AND room_id IS NOT NULL)
  )
);
CREATE INDEX IF NOT EXISTS idx_game_session_mode_started ON game_session(mode, started_at);
CREATE INDEX IF NOT EXISTS idx_game_session_room ON game_session(room_id);

-- 7) Session players
CREATE TABLE IF NOT EXISTS session_player (
  session_id            BIGINT REFERENCES game_session(session_id) ON DELETE CASCADE,
  user_id               BIGINT REFERENCES app_user(user_id) ON DELETE CASCADE,
  display_name_snapshot TEXT NOT NULL,
  is_guest              BOOLEAN NOT NULL,
  final_rank            SMALLINT,
  survival_points       INT DEFAULT 0,
  best_stage            INT DEFAULT 0,
  PRIMARY KEY (session_id, user_id)
);
CREATE INDEX IF NOT EXISTS idx_session_player_user ON session_player(user_id);

-- 8) Rounds / Stages
CREATE TABLE IF NOT EXISTS round (
  round_id           BIGSERIAL PRIMARY KEY,
  session_id         BIGINT NOT NULL REFERENCES game_session(session_id) ON DELETE CASCADE,
  seq_no             INT NOT NULL,
  round_kind         TEXT NOT NULL,
  stage_no           INT,
  paragraph_id       BIGINT NOT NULL REFERENCES paragraph(paragraph_id),
  target_word_count  INT  NOT NULL CHECK (target_word_count > 0),
  required_accuracy  NUMERIC(5,2),
  required_wpm       NUMERIC(6,2),
  started_at         TIMESTAMPTZ,
  ended_at           TIMESTAMPTZ,
  UNIQUE (session_id, seq_no)
);
CREATE INDEX IF NOT EXISTS idx_round_session ON round(session_id);

-- 9) Round results
CREATE TABLE IF NOT EXISTS round_result (
  round_id      BIGINT REFERENCES round(round_id) ON DELETE CASCADE,
  user_id       BIGINT REFERENCES app_user(user_id) ON DELETE CASCADE,
  finished      BOOLEAN NOT NULL DEFAULT FALSE,
  finish_rank   SMALLINT,
  wpm           NUMERIC(6,2) NOT NULL CHECK (wpm >= 0),
  accuracy      NUMERIC(5,2) NOT NULL CHECK (accuracy >= 0 AND accuracy <= 100),
  correct_chars INT NOT NULL DEFAULT 0,
  wrong_chars   INT NOT NULL DEFAULT 0,
  elapsed_ms    INT NOT NULL CHECK (elapsed_ms >= 0),
  elimination   elimination_reason NOT NULL DEFAULT 'NONE',
  afk_seconds   INT NOT NULL DEFAULT 0,
  submitted_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  PRIMARY KEY (round_id, user_id)
);
CREATE INDEX IF NOT EXISTS idx_round_result_user ON round_result(user_id);
CREATE INDEX IF NOT EXISTS idx_round_result_round ON round_result(round_id);

-- 10) Session result
CREATE TABLE IF NOT EXISTS session_result (
  session_id               BIGINT REFERENCES game_session(session_id) ON DELETE CASCADE,
  user_id                  BIGINT REFERENCES app_user(user_id) ON DELETE CASCADE,
  total_wpm                NUMERIC(6,2),
  avg_accuracy             NUMERIC(5,2),
  total_time_ms            INT,
  placement                SMALLINT,
  points                   INT DEFAULT 0,
  counted_for_leaderboard  BOOLEAN NOT NULL DEFAULT FALSE,
  PRIMARY KEY (session_id, user_id)
);
CREATE INDEX IF NOT EXISTS idx_session_result_user_flag ON session_result(user_id, counted_for_leaderboard);

-- 11) Leaderboard (weekly snapshot)
CREATE TABLE IF NOT EXISTS leaderboard_weekly (
  leaderboard_id  BIGSERIAL PRIMARY KEY,
  leaderboard     leaderboard_type NOT NULL,
  week_start_date DATE NOT NULL,
  user_id         BIGINT NOT NULL REFERENCES app_user(user_id) ON DELETE CASCADE,
  score           NUMERIC(8,2) NOT NULL,
  best_stage      INT,
  rank            INT,
  computed_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  UNIQUE (leaderboard, week_start_date, user_id)
);
CREATE INDEX IF NOT EXISTS idx_lb_weekly_main ON leaderboard_weekly(leaderboard, week_start_date, rank);

-- Triggers & functions
CREATE OR REPLACE FUNCTION mark_leaderboard_flag()
RETURNS trigger LANGUAGE plpgsql AS $$
BEGIN
  NEW.counted_for_leaderboard :=
    (EXISTS (
       SELECT 1
       FROM game_session gs
       JOIN session_player sp ON sp.session_id = gs.session_id AND sp.user_id = NEW.user_id
       WHERE gs.session_id = NEW.session_id
         AND gs.is_public = TRUE
         AND sp.is_guest = FALSE
    ));
  RETURN NEW;
END $$;

DROP TRIGGER IF EXISTS trg_sr_mark_flag ON session_result;
CREATE TRIGGER trg_sr_mark_flag
BEFORE INSERT OR UPDATE ON session_result
FOR EACH ROW EXECUTE FUNCTION mark_leaderboard_flag();

CREATE OR REPLACE FUNCTION calc_survival_points(p_session BIGINT, p_user BIGINT)
RETURNS INT LANGUAGE SQL AS $$
  WITH s AS (
    SELECT
      COALESCE(
        CASE sp.final_rank
          WHEN 1 THEN 30 WHEN 2 THEN 24 WHEN 3 THEN 12 ELSE 0
        END, 0
      ) AS base_pts,
      GREATEST(COALESCE(sp.best_stage,0) - 8, 0) * 4 AS bonus_pts
    FROM session_player sp
    JOIN game_session gs ON gs.session_id = sp.session_id
    WHERE sp.session_id = p_session AND sp.user_id = p_user AND gs.mode = 'SURVIVAL'
  )
  SELECT base_pts + bonus_pts FROM s;
$$;

-- Views for weekly leaderboards
CREATE OR REPLACE VIEW v_self_training_week AS
SELECT
  date_trunc('week', gs.started_at)::date AS week_start_date,
  rr.user_id,
  MAX(rr.wpm) AS best_wpm
FROM game_session gs
JOIN round r        ON r.session_id = gs.session_id
JOIN round_result rr ON rr.round_id   = r.round_id
JOIN session_result sr ON sr.session_id = gs.session_id AND sr.user_id = rr.user_id
WHERE gs.mode = 'SELF_TRAINING'
  AND sr.counted_for_leaderboard = TRUE
GROUP BY 1, rr.user_id;

CREATE OR REPLACE VIEW v_survival_points_week AS
SELECT
  date_trunc('week', gs.started_at)::date AS week_start_date,
  sr.user_id,
  MAX(sr.points) AS best_points,
  MAX(sp.best_stage) AS best_stage
FROM game_session gs
JOIN session_result sr ON sr.session_id = gs.session_id
JOIN session_player sp ON sp.session_id = gs.session_id AND sp.user_id = sr.user_id
WHERE gs.mode = 'SURVIVAL'
  AND sr.counted_for_leaderboard = TRUE
GROUP BY 1, sr.user_id;

-- End of schema
