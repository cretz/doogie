-- Note, this file assumes two-adjacent newlines always means
-- statement separation.
--
-- Profile
--
CREATE TABLE IF NOT EXISTS profile (
  cache_path TEXT,
  user_agent TEXT,
  user_data_path TEXT
);

CREATE TABLE IF NOT EXISTS action_shortcut (
  key TEXT UNIQUE NOT NULL,
  shortcuts TEXT NOT NULL
);

-- Bubble stuff
--
CREATE TABLE IF NOT EXISTS bubble (
  id INTEGER PRIMARY KEY NOT NULL,
  order_index INTEGER NOT NULL UNIQUE,
  name TEXT NOT NULL UNIQUE,
  cache_path TEXT,
  icon_path TEXT,
  icon_color TEXT,
  overrides_blocker_lists BOOLEAN NOT NULL
);

CREATE TABLE IF NOT EXISTS browser_setting (
  bubble_id INTEGER,
  key TEXT NOT NULL,
  value BOOLEAN NOT NULL,
  FOREIGN KEY(bubble_id) REFERENCES bubble(id) ON DELETE CASCADE,
  UNIQUE(bubble_id, key)
);

-- Workspace page stuff
--
CREATE TABLE IF NOT EXISTS workspace (
  id INTEGER PRIMARY KEY NOT NULL,
  name TEXT NOT NULL UNIQUE,
  last_opened INTEGER NOT NULL,
  open_index INTEGER UNIQUE
);

CREATE TABLE IF NOT EXISTS workspace_page (
  id INTEGER PRIMARY KEY NOT NULL,
  workspace_id INTEGER NOT NULL,
  parent_id INTEGER,
  pos INTEGER NOT NULL,
  icon BLOB,
  title TEXT,
  url TEXT,
  -- This is intentionally not FK'd
  bubble_id INTEGER NOT NULL,
  suspended BOOLEAN,
  expanded BOOLEAN,
  FOREIGN KEY(workspace_id) REFERENCES workspace(id) ON DELETE CASCADE,
  FOREIGN KEY(parent_id) REFERENCES workspace_page(id) ON DELETE CASCADE
);

-- Download stuff
--
CREATE TABLE IF NOT EXISTS download (
  id INTEGER PRIMARY KEY NOT NULL,
  mime_type TEXT NOT NULL,
  orig_url TEXT NOT NULL,
  url TEXT NOT NULL,
  path TEXT NOT NULL,
  success BOOLEAN NOT NULL,
  -- Unix timestamp
  start_time INTEGER NOT NULL,
  end_time INTEGER,
  size INTEGER NOT NULL
);

-- Blocker list stuff
--
CREATE TABLE IF NOT EXISTS blocker_list (
  id INTEGER PRIMARY KEY NOT NULL,
  name TEXT NOT NULL,
  homepage TEXT,
  url TEXT,
  local_path TEXT NOT NULL,
  version INTEGER,
  -- Unix timestamp
  last_refreshed INTEGER,
  expiration_hours INTEGER,
  last_known_rule_count INTEGER
);

CREATE TABLE IF NOT EXISTS enabled_blocker_list (
  bubble_id INTEGER,
  blocker_list_id INTEGER NOT NULL,
  FOREIGN KEY(bubble_id) REFERENCES bubble(id),
  FOREIGN KEY(blocker_list_id) REFERENCES blocker_list(id),
  UNIQUE(bubble_id, blocker_list_id)
);

-- Page index stuff
--
CREATE TABLE IF NOT EXISTS favicon (
  id INTEGER PRIMARY KEY NOT NULL,
  url TEXT NOT NULL,
  url_hash INTEGER NOT NULL,
  data BLOB NOT NULL
);

CREATE INDEX IF NOT EXISTS favicon_url_hash_idx
  ON favicon(url_hash);

CREATE TABLE IF NOT EXISTS autocomplete_page (
  id INTEGER PRIMARY KEY NOT NULL,
  url TEXT NOT NULL,
  url_hash INTEGER NOT NULL,
  schemeless_url TEXT NOT NULL,
  title TEXT NOT NULL,
  favicon_id INTEGER,
  -- In unix seconds
  last_visited INTEGER NOT NULL,
  visit_count INTEGER NOT NULL,
  -- This is visit_count * visit_worth_seconds
  frecency INTEGER NOT NULL,
  FOREIGN KEY(favicon_id) REFERENCES favicon(id)
);

CREATE INDEX IF NOT EXISTS autocomplete_page_url_hash_idx
  ON autocomplete_page(url_hash);

CREATE VIRTUAL TABLE IF NOT EXISTS autocomplete_page_fts USING FTS5(
  schemeless_url,
  title,
  frecency UNINDEXED,
  content=autocomplete_page,
  content_rowid=id,
  -- Make _, -, and ' as part of the token
  tokenize = "unicode61 tokenchars '-_'"
);

CREATE TRIGGER IF NOT EXISTS autocomplete_page_trig_ai
AFTER INSERT ON autocomplete_page BEGIN
  INSERT INTO autocomplete_page_fts(rowid, schemeless_url, title, frecency)
    VALUES (new.id, new.schemeless_url, new.title, new.frecency);
END;

CREATE TRIGGER IF NOT EXISTS autocomplete_page_trig_ad
AFTER DELETE ON autocomplete_page BEGIN
  INSERT INTO autocomplete_page_fts(autocomplete_page_fts, rowid, schemeless_url, title, frecency)
    VALUES ('delete', old.id, old.schemeless_url, old.title, old.frecency);
END;

CREATE TRIGGER IF NOT EXISTS autocomplete_page_trig_au
AFTER UPDATE ON autocomplete_page BEGIN
  INSERT INTO autocomplete_page_fts(autocomplete_page_fts, rowid, schemeless_url, title, frecency)
    VALUES ('delete', old.id, old.schemeless_url, old.title, old.frecency);
  INSERT INTO autocomplete_page_fts(rowid, schemeless_url, title, frecency)
    VALUES (new.id, new.schemeless_url, new.title, new.frecency);
END;