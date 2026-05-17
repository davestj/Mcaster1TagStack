# JSON API Contract — Mcaster1TagStack v2

All endpoints rooted at `https://tagstack.mcaster1.com/api/v1/`.
All requests + responses are `application/json; charset=utf-8`.
Response envelope: `{ "data": <payload>, "meta": {...}, "errors": [...] }`.

Authentication: Bearer token (Keycloak-issued JWT) in `Authorization`
header. Service-to-service connections use mTLS in addition.


## Resources

| Resource          | Endpoints                                                |
|-------------------|----------------------------------------------------------|
| stations          | `GET /stations` · `GET /stations/{id}` · `PUT /stations/{id}` |
| now-playing       | `GET /now-playing/{station}` · `PUT /now-playing/{station}` · `WS /stream/now-playing/{station}` |
| library           | `GET /library` · `POST /library/items` · `GET/PUT/DELETE /library/items/{id}` |
| sources           | `GET /sources/{provider}/lookup?...` (MusicBrainz, AcoustID, Discogs, Spotify, Last.fm, LRCLIB) |
| enrichment        | `POST /enrich/{id}` · `GET /enrich/{id}/status` |
| distribution      | `POST /distribute/icy2/{station}/{mount}` · `POST /distribute/webhook` · `POST /distribute/social/{network}` |
| compliance        | `GET /compliance/soundexchange?period=...` · `GET /compliance/ascap?...` |
| intel             | `GET /intel/competitors/{group}` · `GET /intel/trends?...` |
| ai                | `POST /ai/persona/{persona}` (Ollama proxy) · `POST /ai/embed` · `POST /ai/transcribe` |
| audit             | `GET /audit?from=...&to=...&user=...&resource=...` |


## Pagination

List endpoints accept `?page=N&per_page=M` (default 1, 50; max 200).
Response `meta` includes `total`, `page`, `per_page`, `next_page`,
`prev_page`.


## Errors

HTTP status carries semantics. `errors[]` is always an array:

```
{
  "data": null,
  "meta": {},
  "errors": [
    { "code": "VALIDATION", "field": "station_id", "message": "..." }
  ]
}
```

Codes: `VALIDATION`, `AUTHN`, `AUTHZ`, `NOT_FOUND`, `CONFLICT`,
`RATE_LIMITED`, `UPSTREAM`, `INTERNAL`.


## Versioning

URL-versioned (`/api/v1/`). Breaking changes bump to `/v2/`; the
previous version is supported for at least 6 months.


## WebSocket

`wss://tagstack.mcaster1.com/api/v1/stream/now-playing/{station}`
yields JSON messages of shape `{ "type": "spin", "data": { ... } }`
on every metadata change. Heartbeats every 30s.
