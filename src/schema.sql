-- Note, this file assumes two-adjacent newlines always means
-- statement separation.
--
-- Workspace page stuff
--
CREATE TABLE IF NOT EXISTS workspace (
  id INTEGER PRIMARY KEY NOT NULL,
  name TEXT NOT NULL UNIQUE,
  last_opened INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS workspace_page (
  id INTEGER PRIMARY KEY NOT NULL,
  workspace_id INTEGER NOT NULL,
  parent_id INTEGER,
  pos INTEGER NOT NULL,
  icon BLOB,
  title TEXT,
  url TEXT,
  bubble TEXT,
  suspended BOOLEAN,
  expanded BOOLEAN,
  FOREIGN KEY(workspace_id) REFERENCES workspace(id) ON DELETE CASCADE,
  FOREIGN KEY(parent_id) REFERENCES workspace_page(id) ON DELETE CASCADE
);

-- Page index stuff
--
CREATE TABLE IF NOT EXISTS favicon (
  id INTEGER PRIMARY KEY NOT NULL,
  url TEXT NOT NULL,
  url_hash INTEGER NOT NULL,
  data_key INTEGER NOT NULL,
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
  schemaless_url,
  title,
  frecency UNINDEXED,
  content=autocomplete_page,
  content_rowid=id,
  -- Make _, -, and ' as part of the token
  tokenize = "unicode61 tokenchars '-_'"
);

CREATE TRIGGER IF NOT EXISTS autocomplete_page_trig_ai
AFTER INSERT ON autocomplete_page BEGIN
  INSERT INTO autocomplete_page_fts(rowid, schemaless_url, title, frecency)
    VALUES (new.id, new.schemaless_url, new.title, new.frecency);
END;

CREATE TRIGGER IF NOT EXISTS autocomplete_page_trig_ad
AFTER DELETE ON autocomplete_page BEGIN
  INSERT INTO autocomplete_page_fts(autocomplete_page_fts, rowid, schemaless_url, title, frecency)
    VALUES ('delete', old.id, old.schemaless_url, old.title, old.frecency);
END;

CREATE TRIGGER IF NOT EXISTS autocomplete_page_trig_au
AFTER UPDATE ON autocomplete_page BEGIN
  INSERT INTO autocomplete_page_fts(autocomplete_page_fts, rowid, schemaless_url, title, frecency)
    VALUES ('delete', old.id, old.schemaless_url, old.title, old.frecency);
  INSERT INTO autocomplete_page_fts(rowid, schemaless_url, title, frecency)
    VALUES (new.id, new.schemaless_url, new.title, new.frecency);
END;