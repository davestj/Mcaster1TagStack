# ICY-META v2.2 Protocol Specification
**Mcaster1DNAS Digital Network Audio Server**

## Overview

ICY-META v2.2 is an extended metadata protocol for streaming media servers, providing rich metadata support beyond the legacy ICY 1.x protocol. It maintains backward compatibility while adding modern features for podcasts, video streams, social media integration, track-level metadata, station programming, listener engagement, audio technical parameters, broadcast distribution, and content verification.

**Status:** 🚧 Implementation in Progress — Mcaster1DNAS v2.6.x (February 2026)

**Specification Version:** 2.2
**Release Date:** February 2026
**Supersedes:** ICY-META v2.1 (February 15, 2026)
**Author:** David St John (Saint John) - MediaCast1/Mcaster1DNAS
**CSSI Spec:** ICY2-META-SPECS.md / https://casterclub.com/specs/icy-2

## Design Principles

1. **Backward Compatible**: ICY2 clients can connect to ICY1 servers; ICY1 clients can connect to ICY2 servers
2. **Zero Configuration**: Auto-detection via version header, no server config changes needed
3. **HTTP-Based**: Pure HTTP header communication, works over HTTP and HTTPS
4. **Extensible**: Version numbering allows future enhancements (2.2, 2.3, etc.)
5. **Optional**: All fields are optional; only send what's relevant

## Protocol Detection

### Version Header

The presence of the `icy-metadata-version` header indicates ICY2 support:

```http
icy-metadata-version: 2.1
```

**Server Behavior:**
- If header present with value `2.x` → Parse as ICY2
- If header absent or invalid → Fall back to ICY 1.x legacy parsing
- Version check uses prefix matching (`2.` matches `2.0`, `2.1`, `2.2`, etc.)

### Client Handshake

ICY2-compliant source clients send this header with their source PUT/POST request:

```http
PUT /mountpoint.mp3 HTTP/1.1
Host: server.example.com:9443
icy-metadata-version: 2.2
icy-meta-station-id: unique-station-identifier
icy-name: My Station Name
icy-genre: Electronic
icy-br: 128
icy-pub: 1
Authorization: Basic c291cmNlOnBhc3N3b3Jk
Content-Type: audio/mpeg

[audio data]
```

Note: v2.1 clients sending `icy-station-id` (without `meta`) instead of `icy-meta-station-id` are
fully supported. Legacy ICY 1.x fields (`icy-name`, `icy-genre`, `icy-br`, `icy-pub`) are always
parsed regardless of ICY2 version and must always be included for directory and player compatibility.

## ICY 1.x Legacy Headers (Unchanged — Always Preserved)

These are the original SHOUTcast/Icecast ICY protocol headers. They are never modified, never removed,
and are always parsed independently of any ICY2 logic. ICY2 fields are additional — these remain exactly
as they have always been for full backwards compatibility with all legacy source clients and listeners.

| Header      | Type    | Description                                      |
|-------------|---------|--------------------------------------------------|
| icy-name    | String  | Station or stream display name                   |
| icy-genre   | String  | Genre or content type                            |
| icy-url     | URL     | Station homepage or site                         |
| icy-pub     | Boolean | Public listing flag (1 = yes)                    |
| icy-br      | Integer | Bitrate in kbps                                  |
| icy-metaint | Integer | Interval in bytes between metadata blocks        |

Legacy auth fields (also unchanged):

| Field         | Description                                        |
|---------------|----------------------------------------------------|
| adminpassword | Admin interface password                           |
| password      | Source encoder password                            |
| user          | Optional username (SHOUTcast v2 / Icecast2)        |

---

## ICY2 v2.1 Header Forms (Backwards Compatible — Still Accepted)

These are the original ICY2 v2.1 header names as implemented in the server's C source
(`icy2_meta_parse_headers()`). They use the `icy-` prefix without `meta`. The server continues
to accept these from any client that connected using the v2.1 format. Do not remove them —
they must remain functional for backwards compatibility.

| v2.1 Header (icy- prefix)   | v2.2 Header (icy-meta- prefix)    | Stats Key                    |
|-----------------------------|-----------------------------------|------------------------------|
| icy-station-id              | icy-meta-station-id               | icy2-station-id              |
| icy-podcast-host            | icy-meta-podcast-host             | icy2-podcast-host            |
| icy-podcast-rss             | icy-meta-podcast-rss              | icy2-podcast-rss             |
| icy-podcast-episode         | icy-meta-podcast-episode          | icy2-podcast-episode         |
| icy-duration                | icy-meta-duration                 | icy2-duration                |
| icy-language                | icy-meta-language                 | icy2-language                |
| icy-video-type              | icy-meta-videotype                | icy2-video-type              |
| icy-video-link              | icy-meta-videolink                | icy2-video-link              |
| icy-video-title             | icy-meta-videotitle               | icy2-video-title             |
| icy-video-platform          | icy-meta-videoplatform            | icy2-video-platform          |
| icy-video-resolution        | icy-meta-videoresolution          | icy2-video-resolution        |
| icy-dj-handle               | icy-meta-dj-handle                | icy2-dj-handle               |
| icy-social-twitter          | icy-meta-social-twitter           | icy2-social-twitter          |
| icy-social-ig               | icy-meta-social-ig                | icy2-social-instagram        |
| icy-social-tiktok           | icy-meta-social-tiktok            | icy2-social-tiktok           |
| icy-emoji                   | icy-meta-emoji                    | icy2-emoji                   |
| icy-hashtags                | icy-meta-hashtag-array            | icy2-hashtags                |
| icy-auth-token              | icy-meta-auth-token               | icy2-auth-token              |
| icy-nsfw                    | icy-meta-nsfw                     | icy2-nsfw                    |
| icy-ai-generated            | icy-meta-ai-generator             | icy2-ai-generated            |
| icy-geo-region              | icy-meta-geo-region               | icy2-geo-region              |
| icy-certificate-verify      | icy-meta-certificate              | icy2-certificate             |
| icy-verification-status     | icy-meta-verification-status      | icy2-verification-status     |

The C parser tries `icy-meta-X` first, then falls back to `icy-X` for each field — both always work.

---

## Field Reference (v2.2 — New and Updated Fields)

Full field descriptions and rationale are maintained in ICY2-META-SPECS.md (the CSSI canonical spec).
The following are the v2.2 standard header names. All v2.1 aliases in the table above remain valid.

### Station Identity

| Header                        | Type    | Stats Key                    | Notes                                  |
|-------------------------------|---------|------------------------------|----------------------------------------|
| icy-meta-station-id           | String  | icy2-station-id              | Permanent unique station identifier    |
| icy-meta-station-logo         | URL     | icy2-station-logo            | Station logo/branding image            |
| icy-meta-certissuer-id        | String  | icy2-certissuer-id           | Certificate authority ID               |
| icy-meta-cert-rootca          | String  | icy2-cert-rootca             | Root CA hash                           |
| icy-meta-certificate          | String  | icy2-certificate             | Base64 PEM certificate                 |
| icy-meta-ssh-pubkey           | String  | icy2-ssh-pubkey              | SSH public key for auth                |
| icy-meta-verification-status  | Enum    | icy2-verification-status     | unverified/pending/verified/gold       |

### Programming / Show

| Header                    | Type    | Stats Key                | Notes                                         |
|---------------------------|---------|--------------------------|-----------------------------------------------|
| icy-meta-show-title       | String  | icy2-show-title          | Current program title                         |
| icy-meta-show-start       | ISO8601 | icy2-show-start          | Current show start time                       |
| icy-meta-show-end         | ISO8601 | icy2-show-end            | Current show end time                         |
| icy-meta-next-show        | String  | icy2-next-show           | Next scheduled program                        |
| icy-meta-next-show-time   | ISO8601 | icy2-next-show-time      | Next show start time                          |
| icy-meta-schedule-url     | URL     | icy2-schedule-url        | Full program schedule URL                     |
| icy-meta-autodj           | Boolean | icy2-autodj              | 1=automation active, 0=live                   |
| icy-meta-playlist-name    | String  | icy2-playlist-name       | Current playlist or automation source         |

### DJ / Host

| Header                | Type   | Stats Key           | Notes                                              |
|-----------------------|--------|---------------------|----------------------------------------------------|
| icy-meta-dj-handle    | String | icy2-dj-handle      | Social handle (e.g., @djsynthwave)                 |
| icy-meta-dj-bio       | String | icy2-dj-bio         | Short biography or tagline, max 280 chars          |
| icy-meta-dj-genre     | String | icy2-dj-genre       | Comma-separated genres, max 5                      |
| icy-meta-dj-showrating| Enum   | icy2-dj-showrating  | all-ages, teen, mature, explicit                   |

### Track Metadata

| Header                 | Type    | Stats Key             | Notes                                          |
|------------------------|---------|-----------------------|------------------------------------------------|
| icy-meta-track-artwork | URL     | icy2-track-artwork    | Album/track art URL — primary player display   |
| icy-meta-track-album   | String  | icy2-track-album      | Album or release name                          |
| icy-meta-track-year    | Integer | icy2-track-year       | Release year                                   |
| icy-meta-track-label   | String  | icy2-track-label      | Record label                                   |
| icy-meta-track-bpm     | Integer | icy2-track-bpm        | Beats per minute                               |
| icy-meta-track-key     | String  | icy2-track-key        | Musical key (e.g., 8B, Am)                     |
| icy-meta-track-genre   | String  | icy2-track-genre      | Per-track genre                                |
| icy-meta-track-mbid    | UUID    | icy2-track-mbid       | MusicBrainz Recording ID                       |
| icy-meta-track-isrc    | String  | icy2-track-isrc       | International Standard Recording Code         |

### Podcast

| Header                    | Type    | Stats Key              | Notes                                      |
|---------------------------|---------|------------------------|--------------------------------------------|
| icy-meta-podcast-host     | String  | icy2-podcast-host      | Podcast creator/host name                  |
| icy-meta-podcast-rating   | Enum    | icy2-podcast-rating    | all-ages, teen, mature, explicit            |
| icy-meta-podcast-rss      | URL     | icy2-podcast-rss       | RSS feed URL                               |
| icy-meta-podcast-episode  | String  | icy2-podcast-episode   | Episode title or ID                        |
| icy-meta-duration         | Integer | icy2-duration          | Content runtime in seconds (all types)     |
| icy-meta-language         | String  | icy2-language          | ISO 639-1 language tag                     |

### Video

| Header                    | Type    | Stats Key               | Notes                                         |
|---------------------------|---------|-------------------------|-----------------------------------------------|
| icy-meta-videotype        | Enum    | icy2-video-type         | live, short, clip, trailer, ad                |
| icy-meta-videorating      | Enum    | icy2-video-rating       | all-ages, teen, mature, explicit              |
| icy-meta-videolink        | URL     | icy2-video-link         | Video content URL                             |
| icy-meta-videotitle       | String  | icy2-video-title        | Video title                                   |
| icy-meta-videoposter      | URL     | icy2-video-poster       | Thumbnail/preview image                       |
| icy-meta-videochannel     | String  | icy2-video-channel      | Creator/channel handle                        |
| icy-meta-videoplatform    | Enum    | icy2-video-platform     | youtube, tiktok, twitch, kick, rumble, vimeo  |
| icy-meta-videostart       | ISO8601 | icy2-video-start        | Scheduled video start time                    |
| icy-meta-videolive        | Boolean | icy2-video-live         | 1=currently live                              |
| icy-meta-videocodec       | String  | icy2-video-codec        | h264, vp9, av1                                |
| icy-meta-videofps         | Integer | icy2-video-fps          | Frames per second                             |
| icy-meta-videoresolution  | String  | icy2-video-resolution   | e.g., 1080p, 4K                               |
| icy-meta-videonsfw        | Boolean | icy2-video-nsfw         | Video-specific NSFW flag                      |

### Audio Technical

| Header               | Type    | Stats Key          | Notes                                           |
|----------------------|---------|--------------------|-------------------------------------------------|
| icy-meta-audio-codec | Enum    | icy2-audio-codec   | mp3, aac, aac-he, ogg, opus, flac               |
| icy-meta-samplerate  | Integer | icy2-samplerate    | Sample rate in Hz                               |
| icy-meta-channels    | Integer | icy2-channels      | 1=mono, 2=stereo, 6=5.1                         |
| icy-meta-loudness    | Float   | icy2-loudness      | LUFS integrated loudness (EBU R128)             |
| icy-meta-encoder     | String  | icy2-encoder       | Encoder name and version                        |

### Social

| Header                        | Type   | Stats Key                  | Notes                                    |
|-------------------------------|--------|----------------------------|------------------------------------------|
| icy-meta-creator-handle       | String | icy2-creator-handle        | Platform-agnostic public identity        |
| icy-meta-social-twitter       | String | icy2-social-twitter        | Twitter/X handle                         |
| icy-meta-social-twitch        | String | icy2-social-twitch         | Twitch handle                            |
| icy-meta-social-ig            | String | icy2-social-instagram      | Instagram username                       |
| icy-meta-social-tiktok        | String | icy2-social-tiktok         | TikTok profile name                      |
| icy-meta-social-youtube       | URL    | icy2-social-youtube        | YouTube channel URL (static profile)     |
| icy-meta-social-facebook-page | URL    | icy2-social-facebook       | Facebook page URL                        |
| icy-meta-social-linkedin      | URL    | icy2-social-linkedin       | LinkedIn profile URL                     |
| icy-meta-social-linktree      | URL    | icy2-social-linktree       | Unified profile link                     |
| icy-meta-emoji                | String | icy2-emoji                 | Mood/emotion indicators                  |
| icy-meta-hashtag-array        | JSON   | icy2-hashtags              | JSON array of hashtag strings            |

### Listener Engagement

| Header                    | Type    | Stats Key              | Notes                               |
|---------------------------|---------|------------------------|-------------------------------------|
| icy-meta-request-enabled  | Boolean | icy2-request-enabled   | 1=requests open                     |
| icy-meta-request-url      | URL     | icy2-request-url       | Song request / dedication form      |
| icy-meta-chat-url         | URL     | icy2-chat-url          | Live listener chat URL              |
| icy-meta-tip-url          | URL     | icy2-tip-url           | Listener donation/tip URL           |
| icy-meta-events-url       | URL     | icy2-events-url        | Upcoming events page                |

### Broadcast Distribution

| Header                        | Type   | Stats Key                | Notes                                                |
|-------------------------------|--------|--------------------------|------------------------------------------------------|
| icy-meta-crosspost-platforms  | String | icy2-crosspost-platforms | Comma-sep active platforms (e.g., youtube,twitch)    |
| icy-meta-stream-session-id    | String | icy2-stream-session-id   | Unique ID for this broadcast session                 |
| icy-meta-cdn-region           | String | icy2-cdn-region          | CDN distribution region (e.g., us-east, eu-west)     |
| icy-meta-relay-origin         | URL    | icy2-relay-origin        | Origin server URL if this is a relay mount           |

### Station Notices

| Header                   | Type    | Stats Key           | Notes                                    |
|--------------------------|---------|---------------------|------------------------------------------|
| icy-meta-notice          | String  | icy2-notice         | General listener notice/announcement     |
| icy-meta-notice-url      | URL     | icy2-notice-url     | Click-through URL for notice             |
| icy-meta-notice-expires  | ISO8601 | icy2-notice-expires | Datetime after which notice is hidden    |

### Access & Authentication

| Header               | Type | Stats Key        | Notes                                         |
|----------------------|------|------------------|-----------------------------------------------|
| icy-meta-auth-token  | JWT  | icy2-auth-token  | Bearer JWT or custom token                    |

### Content Flags & Compliance

| Header                     | Type    | Stats Key                 | Notes                                             |
|----------------------------|---------|---------------------------|---------------------------------------------------|
| icy-meta-nsfw              | Boolean | icy2-nsfw                 | Binary NSFW filter flag for directory filtering   |
| icy-meta-ai-generator      | Boolean | icy2-ai-generated         | AI-generated content transparency flag            |
| icy-meta-geo-region        | String  | icy2-geo-region           | Target region (ISO 3166-1, e.g., US, EU, GLOBAL)  |
| icy-meta-license-type      | Enum    | icy2-license-type         | cc-by, cc0, pro-licensed, all-rights-reserved     |
| icy-meta-royalty-free      | Boolean | icy2-royalty-free         | 1=royalty-free content                            |
| icy-meta-license-territory | String  | icy2-license-territory    | Comma-sep ISO country codes, or GLOBAL            |

---

## Complete Example Request

```http
PUT /my-radio-station.mp3 HTTP/1.1
Host: stream.mcaster1.com:9443
Authorization: Basic c291cmNlOmhhY2ttZQ==
Content-Type: audio/mpeg
User-Agent: Mcaster1DSPEncoder/1.2.0

icy-metadata-version: 2.2
icy-name: Mcaster1 Electronic Radio
icy-url: https://mcaster1.com
icy-genre: Electronic/House/Techno
icy-br: 320
icy-pub: 1

icy-meta-station-id: mcaster1-electronic-001
icy-meta-station-logo: https://mcaster1.com/logo.png
icy-meta-verification-status: verified

icy-meta-show-title: Late Night House Sessions
icy-meta-show-start: 2026-02-21T22:00:00Z
icy-meta-show-end: 2026-02-22T02:00:00Z
icy-meta-autodj: 0
icy-meta-playlist-name: House Rotation Vol. 12

icy-meta-dj-handle: @djsynthwave
icy-meta-dj-bio: Berlin-based electronic DJ. Deep house, techno, and everything in between.
icy-meta-dj-genre: Electronic, House, Techno, Deep House
icy-meta-dj-showrating: all-ages

icy-meta-track-artwork: https://cdn.mcaster1.com/art/track456.jpg
icy-meta-track-album: Midnight Sessions Vol. 3
icy-meta-track-year: 2025
icy-meta-track-label: Hypnotic Records
icy-meta-track-bpm: 124
icy-meta-track-key: 8B
icy-meta-track-mbid: 3a8e7c21-1234-5678-abcd-ef0123456789
icy-meta-track-isrc: USRC12345678

icy-meta-podcast-host: DJ Synthwave
icy-meta-duration: 7200
icy-meta-language: en-US

icy-meta-videotype: live
icy-meta-videolink: https://youtube.com/watch?v=live-stream
icy-meta-videotitle: Live from Studio 1
icy-meta-videoplatform: youtube
icy-meta-videoresolution: 1080p
icy-meta-videocodec: h264
icy-meta-videofps: 60
icy-meta-videolive: 1

icy-meta-audio-codec: mp3
icy-meta-samplerate: 44100
icy-meta-channels: 2
icy-meta-loudness: -14.0
icy-meta-encoder: Mcaster1DSP/1.2.0

icy-meta-creator-handle: @mcaster1electronic
icy-meta-social-twitter: @mcaster1dnas
icy-meta-social-ig: @mcaster1official
icy-meta-social-tiktok: @mcaster1music
icy-meta-social-youtube: https://youtube.com/@mcaster1
icy-meta-social-linktree: https://linktr.ee/mcaster1
icy-meta-emoji: 🎵🔥🎧💿🌃
icy-meta-hashtag-array: ["#electronic","#house","#techno","#livemusic"]

icy-meta-request-enabled: 1
icy-meta-request-url: https://mcaster1.com/requests
icy-meta-chat-url: https://mcaster1.com/chat
icy-meta-tip-url: https://ko-fi.com/mcaster1

icy-meta-crosspost-platforms: youtube,twitch
icy-meta-stream-session-id: session-20260221-mc1-001
icy-meta-cdn-region: us-east

icy-meta-notice: Tune in to our YouTube stream for the video feed tonight!
icy-meta-notice-url: https://youtube.com/watch?v=live-stream
icy-meta-notice-expires: 2026-02-22T02:00:00Z

icy-meta-nsfw: 0
icy-meta-ai-generator: 0
icy-meta-geo-region: GLOBAL
icy-meta-license-type: pro-licensed

[audio stream data follows]
```

## Server Implementation

### Detection Logic

```c
// Check for ICY2 version header
const char *version = httpp_getvar(parser, "icy-metadata-version");
if (version && strncmp(version, "2.", 2) == 0) {
    // Parse as ICY2
    icy2_metadata *meta = icy2_meta_new();
    icy2_meta_parse_headers(meta, parser);
    icy2_meta_copy_to_stats(meta, source->stats);
    icy2_meta_free(meta);
} else {
    // Fall back to ICY 1.x legacy parsing
    parse_legacy_icy_headers(parser);
}
```

### Parser Implementation Notes

The v2.2 parser should accept both `icy-meta-X` (spec-compliant) and legacy `icy-X` (v2.1 clients)
header forms during the migration period. Try `icy-meta-X` first; fall back to `icy-X`:

```c
/* Example: station-id — try spec-compliant prefix first, fall back for v2.1 clients */
str = httpp_getvar(parser, "icy-meta-station-id");
if (!str) str = httpp_getvar(parser, "icy-station-id");
if (str) meta->station_id = icy2_strdup_safe(str);
```

Apply this pattern to all ICY2-specific fields in `icy2_meta_parse_headers()`.

### Statistics Integration

All ICY2 metadata fields are stored in the server's statistics system with the `icy2-` prefix.
The complete stats key mapping is listed in the Field Reference tables above.

```xml
<source mount="/my-radio-station.mp3">
    <icy2-version>2.2</icy2-version>
    <icy2-station-id>mcaster1-electronic-001</icy2-station-id>
    <icy2-station-logo>https://mcaster1.com/logo.png</icy2-station-logo>
    <server_name>Mcaster1 Electronic Radio</server_name>
    <server_url>https://mcaster1.com</server_url>
    <genre>Electronic/House/Techno</genre>
    <bitrate>320</bitrate>
    <public>1</public>

    <icy2-show-title>Late Night House Sessions</icy2-show-title>
    <icy2-autodj>0</icy2-autodj>
    <icy2-dj-handle>@djsynthwave</icy2-dj-handle>
    <icy2-dj-showrating>all-ages</icy2-dj-showrating>

    <icy2-track-artwork>https://cdn.mcaster1.com/art/track456.jpg</icy2-track-artwork>
    <icy2-track-album>Midnight Sessions Vol. 3</icy2-track-album>
    <icy2-track-bpm>124</icy2-track-bpm>
    <icy2-track-key>8B</icy2-track-key>
    <icy2-track-mbid>3a8e7c21-1234-5678-abcd-ef0123456789</icy2-track-mbid>
    <icy2-track-isrc>USRC12345678</icy2-track-isrc>

    <icy2-podcast-host>DJ Synthwave</icy2-podcast-host>
    <icy2-duration>7200</icy2-duration>
    <icy2-language>en-US</icy2-language>

    <icy2-video-type>live</icy2-video-type>
    <icy2-video-link>https://youtube.com/watch?v=live-stream</icy2-video-link>
    <icy2-video-platform>youtube</icy2-video-platform>
    <icy2-video-resolution>1080p</icy2-video-resolution>
    <icy2-video-live>1</icy2-video-live>

    <icy2-audio-codec>mp3</icy2-audio-codec>
    <icy2-samplerate>44100</icy2-samplerate>
    <icy2-channels>2</icy2-channels>
    <icy2-loudness>-14.0</icy2-loudness>
    <icy2-encoder>Mcaster1DSP/1.2.0</icy2-encoder>

    <icy2-social-twitter>@mcaster1dnas</icy2-social-twitter>
    <icy2-social-instagram>@mcaster1official</icy2-social-instagram>
    <icy2-social-tiktok>@mcaster1music</icy2-social-tiktok>
    <icy2-emoji>🎵🔥🎧💿🌃</icy2-emoji>
    <icy2-hashtags>["#electronic","#house","#techno","#livemusic"]</icy2-hashtags>

    <icy2-request-enabled>1</icy2-request-enabled>
    <icy2-chat-url>https://mcaster1.com/chat</icy2-chat-url>
    <icy2-tip-url>https://ko-fi.com/mcaster1</icy2-tip-url>

    <icy2-crosspost-platforms>youtube,twitch</icy2-crosspost-platforms>
    <icy2-cdn-region>us-east</icy2-cdn-region>

    <icy2-notice>Tune in to our YouTube stream for the video feed tonight!</icy2-notice>
    <icy2-notice-expires>2026-02-22T02:00:00Z</icy2-notice-expires>

    <icy2-nsfw>0</icy2-nsfw>
    <icy2-ai-generated>0</icy2-ai-generated>
    <icy2-geo-region>GLOBAL</icy2-geo-region>
    <icy2-license-type>pro-licensed</icy2-license-type>
</source>
```

## Client Implementation Guidelines

### Minimal ICY2 Client

At minimum, an ICY2 client should send:

```http
icy-metadata-version: 2.2
icy-meta-station-id: unique-identifier
icy-name: Station Name
icy-genre: Genre
```

### Best Practices

1. **Always include `icy-metadata-version`** - This triggers ICY2 parsing
2. **Use consistent station-id** - Helps with stream tracking and analytics
3. **Provide social media handles** - Enhances discoverability
4. **Set appropriate content flags** - NSFW, AI-generated for transparency
5. **Include video metadata** - If streaming video alongside audio
6. **Use emoji sparingly** - 2-5 emoji max for UI enhancement
7. **Validate URLs** - Ensure all URL fields are properly formatted
8. **Keep hashtags relevant** - 3-10 hashtags, space or comma-separated

### Backward Compatibility

ICY2 clients should gracefully degrade when connecting to ICY 1.x servers:

- Server ignores unknown headers (ICY2 fields)
- Legacy fields (icy-name, icy-genre, etc.) still work
- Client receives standard ICY 1.x response

## Testing with cURL

### Basic Test

```bash
curl -k -X PUT \
  -H "icy-metadata-version: 2.2" \
  -H "icy-meta-station-id: test-001" \
  -H "icy-name: Test Station" \
  -H "icy-genre: Test" \
  -H "Authorization: Basic c291cmNlOmhhY2ttZQ==" \
  -H "Content-Type: audio/mpeg" \
  --data-binary @audio-file.mp3 \
  https://server.example.com:9443/test.mp3
```

### Full Metadata Test

```bash
curl -k -X PUT \
  -H "icy-metadata-version: 2.2" \
  -H "icy-meta-station-id: full-test-999" \
  -H "icy-name: Full ICY2 Test Stream" \
  -H "icy-genre: Electronic/Test" \
  -H "icy-url: https://mcaster1.com" \
  -H "icy-br: 128" \
  -H "icy-pub: 1" \
  -H "icy-meta-show-title: Test Show" \
  -H "icy-meta-autodj: 0" \
  -H "icy-meta-dj-handle: @testdj" \
  -H "icy-meta-dj-showrating: all-ages" \
  -H "icy-meta-track-artwork: https://example.com/art.jpg" \
  -H "icy-meta-track-bpm: 128" \
  -H "icy-meta-podcast-host: DJ Test" \
  -H "icy-meta-podcast-rss: https://mcaster1.com/rss" \
  -H "icy-meta-duration: 3600" \
  -H "icy-meta-language: en-US" \
  -H "icy-meta-videotype: live" \
  -H "icy-meta-videoplatform: youtube" \
  -H "icy-meta-videolive: 1" \
  -H "icy-meta-audio-codec: mp3" \
  -H "icy-meta-samplerate: 44100" \
  -H "icy-meta-channels: 2" \
  -H "icy-meta-loudness: -14.0" \
  -H "icy-meta-encoder: curl-test/1.0" \
  -H "icy-meta-social-twitter: @mcaster1" \
  -H "icy-meta-emoji: 🎵🔥" \
  -H "icy-meta-hashtag-array: [\"#test\",\"#icy2\"]" \
  -H "icy-meta-request-enabled: 1" \
  -H "icy-meta-chat-url: https://mcaster1.com/chat" \
  -H "icy-meta-notice: Test notice message" \
  -H "icy-meta-nsfw: 0" \
  -H "icy-meta-ai-generator: 0" \
  -H "icy-meta-geo-region: GLOBAL" \
  -H "icy-meta-license-type: pro-licensed" \
  -H "Authorization: Basic c291cmNlOmhhY2ttZQ==" \
  -H "Content-Type: audio/mpeg" \
  --data-binary @audio-file.mp3 \
  https://server.example.com:9443/test-full.mp3
```

### Verify Detection

Check server logs for:
```
INFO icy2/icy2_meta_is_icy2 Detected ICY-META version 2.2
INFO icy2/icy2_meta_parse_headers Parsed XX ICY2 metadata fields for station-id: test-001
INFO icy2/icy2_meta_copy_to_stats ICY2 metadata copied to stats
```

## Future Protocol Versions

### ICY-META v2.3 (Proposed)

- **Dynamic Mid-stream Updates**: Update metadata blocks without reconnecting
- **Subscriber-only Metadata**: Private metadata fields for authenticated listeners
- **Multi-language Metadata**: Parallel metadata in multiple language tags
- **Listener Demographics**: Aggregate listener location, device, skip/like signals
- **Interactive Elements**: Poll data, live chat integration metadata
- **Spatial Audio**: 3D/binaural audio positioning metadata

## Version History

| Version | Release Date | Status | Key Features |
|---------|-------------|--------|--------------|
| 2.1 | February 15, 2026 | ✅ Released | Core station ID, podcast, video, social, content flags |
| 2.2 | February 2026 | 🚧 In Progress | Track metadata, artwork, show scheduling, audio technical, notices, engagement, distribution, PKI, licensing |
| 2.3 | Q3 2026 | 💡 Proposed | Dynamic updates, multi-language, subscriber metadata, interactivity |

## Compliance & Standards

### Character Encoding

All text fields should use UTF-8 encoding to support international characters and emoji.

### URL Validation

All URL fields should be valid HTTP/HTTPS URLs:
- Must start with `http://` or `https://`
- Should be properly URL-encoded
- Should be publicly accessible (for verification)

### Language Codes

Use ISO 639-1 two-letter codes, optionally with ISO 3166-1 country codes:
- `en` - English
- `en-US` - US English
- `es` - Spanish
- `fr` - French
- `de` - German
- `ja` - Japanese

### Content Flags

**NSFW Flag Requirements:**
- `icy-meta-nsfw: 1` must be set for explicit content
- Failure to set may violate platform policies
- Affects directory listings and recommendations

**AI-Generated Flag:**
- Required for AI-generated or AI-assisted content in some jurisdictions
- Transparency requirement for content attribution
- May affect copyright and licensing

## Security Considerations

### Authentication

ICY2 supports multiple authentication methods:
1. **Basic Auth**: Standard HTTP Basic authentication (username:password)
2. **Token-based**: OAuth, JWT, or custom tokens via `icy-auth-token`
3. **Certificate**: SSL/TLS client certificates (future)
4. **SSH Keys**: Public/private key pairs (future)

### HTTPS/SSL

**Recommended:** All ICY2 streams should use HTTPS for:
- Encrypted metadata transmission
- Password protection
- Token security
- Content integrity

### Token Format

If using `icy-auth-token`:
```
icy-auth-token: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

Token should be:
- Short-lived (1-24 hours)
- Revocable
- Scoped to specific mount points

## License & Attribution

**ICY-META v2.1+ Protocol Specification**

Copyright (C) 2026 David St John (Saint John)
MediaCast1 / Mcaster1DNAS Project

This specification is released under the Creative Commons Attribution 4.0 International License (CC BY 4.0).

You are free to:
- Share: Copy and redistribute the specification
- Adapt: Implement in any software project
- Commercial Use: Use in commercial products

Under the following terms:
- Attribution: Give appropriate credit to Mcaster1DNAS project

**Implementation in Mcaster1DNAS is licensed under GNU GPL v2.**

## References

- **Mcaster1DNAS**: https://github.com/davestj/mcaster1dnas
- **ICY Protocol (Legacy)**: Icecast/Shoutcast documentation
- **HTTP/1.1 Specification**: RFC 7230-7235
- **ISO 639-1 Language Codes**: https://en.wikipedia.org/wiki/ISO_639-1
- **ISO 3166-1 Country Codes**: https://en.wikipedia.org/wiki/ISO_3166-1

## Contact & Support

**Project Maintainer:** David St John (Saint John)
**Email:** davestj@gmail.com
**Website:** https://mcaster1.com
**GitHub:** https://github.com/davestj/mcaster1dnas
**Issues:** https://github.com/davestj/mcaster1dnas/issues

---

**Last Updated:** February 21, 2026
**Specification Version:** 2.2
**Implementation:** Mcaster1DNAS v2.6.x (in progress)
