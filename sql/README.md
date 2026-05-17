# sql/ — MariaDB migrations

Numeric, append-only. Never edit a committed migration; write a new
one. The daemon's startup applies pending migrations from this dir
in numeric order.

## Migrations

| #   | File                            | What it does                            |
|-----|---------------------------------|-----------------------------------------|
| 001 | init.sql                        | Schema baseline                         |
| 002 | servers.sql                     | Server connections table                |
| 003 | media_weight.sql                | Rotation weighting columns              |
| 004 | media_extended.sql              | BPM / key / MBID / ISRC / label / mood  |
| 005 | composer_tables.sql             | Playlist composer + clock templates     |
| 006 | playlist_extended.sql           | Playlist scheduling extensions          |
| 007 | icy2_queue.sql                  | Outgoing ICY 2.2 push queue             |
| 008 | meta_composer_sessions.sql      | Metadata edit sessions                  |
| 009 | session_rules.sql               | Edit-approval workflow rules            |
| 010 | broadcast_events.sql            | Event log (spins, status changes)       |
| 011 | users_audit.sql                 | RBAC users + append-only audit log      |
