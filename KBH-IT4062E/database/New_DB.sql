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
('silent rivers carry small dreams across open fields where tired travelers rest beside warm stones and listen to wind in tall grass until morning brings new plans and gentle courage for everyone who keeps trying today', 'en'),
('the old lantern glows in a narrow room while friends trade stories about distant markets lost maps and lucky meals they laugh softly then breathe slowly and feel the night grow kind until sleep arrives softly', 'en'),
('bright clouds drift above city rooftops as bicycles roll along quiet streets people share simple greetings and carry baskets of fruit they stop near parks watch birds and forget worries at home they breathe easy again', 'en'),
('when winter fades the garden wakes with tender shoots and green leaves a child waters each corner patiently learning rhythm and care the soil responds with colors scents and steady joy before dinner they whisper thanks', 'en'),
('a wandering musician plays soft chords under a bridge at dusk strangers pause for a moment then continue their paths the notes linger like warm tea and make cold air feel gentle for anyone waiting patiently', 'en'),
('books stacked on a wooden table invite curious hands each page offers small surprises and hidden lessons readers sit together without speaking yet they share attention and sense common peaceful thought as hours pass in comfort', 'en'),
('on a long train ride passengers gaze through windows and count farms rivers and hills some nap some write letters others imagine future homes and listen to wheels tapping steady hopeful beat through tunnels and stations', 'en'),
('the baker rises early kneads dough with strong arms and smiles at the scent of heat neighbors arrive for bread and conversation they leave with full bags and a feeling of care to start work smiling', 'en'),
('at the edge of the sea a lighthouse stands firm against spray fishermen mend nets and speak of weather tides and patience the horizon stays wide and their hearts stay steady and humble in every season', 'en'),
('a small workshop hums with tools and focus makers shape wood into toys they sand edges smooth and test each wheel the work is simple but pride is deep and honest and children play with them', 'en'),
('morning coffee steams beside a notebook as a student plans the day with lists of goals and breaks the mind feels clear the hands feel ready and time seems like friendly companion as sunlight climbs higher', 'en'),
('under tall pines hikers follow a narrow trail and hear needles crunch they breathe resin scented air and watch sunlight flicker on moss the path twists yet confidence grows with every step until ridge opens wide', 'en'),
('a quiet library corner holds a single lamp and a soft chair someone opens a history book and sinks into far years then closes the cover and returns to present feeling wiser to face tomorrow calmly', 'en'),
('city rain falls steadily on umbrellas and windows buses hiss at stops and street vendors cover their goods inside a cafe people warm their hands and share smiles that need no words as thunder rolls away', 'en'),
('after a long shift a nurse walks home and watches the sky change from gold to blue the streets feel slower breathing becomes easier and gratitude rises for small ordinary comforts and steps feel less heavy', 'en'),
('a simple kite climbs above a grassy hill two siblings run and tug the string laughing as fabric turns they fall into the grass and look up at freedom in motion until dusk paints hill gold', 'en'),
('late at night a programmer reviews lines of code and searches for a hidden bug the screen shines gently patience wins at last and the final test passes like quiet victory and mind finally rests satisfied', 'en'),
('an artist mixes paint on a palette choosing shades of earth and sun the canvas waits the first stroke feels risky then the scene appears and confidence settles into the wrist and studio air turns warm', 'en'),
('in a crowded market aromas of spice and citrus float between stalls buyers bargain kindly and share tips about recipes laughter slips through the noise and strangers feel briefly like neighbors before markets close for rest', 'en'),
('the classroom grows still as the teacher asks a thoughtful question students consider then speak with care ideas connect like threads in cloth and a sense of possibility fills the room and every voice adds light', 'en');


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
