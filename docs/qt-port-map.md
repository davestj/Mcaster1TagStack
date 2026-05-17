# MFC v1 → Qt6 Port Map

This document is the canonical port table for the desktop client. It maps
every UI surface, command-line interface, and external binary in the
legacy `~/dev/01_mcaster1.com/Mcaster1TagStack/` MFC tree to its Qt6
counterpart in `desktop/src/`.

The build for the Qt6 port uses GNU **autotools** (autoconf + automake +
pkg-config), not CMake or qmake. See `desktop/configure.ac` and the
per-directory `Makefile.am` files for the build wiring. UI styling uses
Qt's **default native theme** — no `QStyle`, `QPalette`, or stylesheet
overrides — which gives macOS-Cocoa look on macOS, Fusion on Linux, and
native chrome on Windows.


## 1. The MFC solution had six projects

| # | v1 Project | Binary | Role | Qt6 status |
|---|------------|--------|------|------------|
| 1 | Mcaster1TagStack            | `Mcaster1TagStack.exe` | Main application (10 pages, 6 modals) | **In progress** — `desktop/` |
| 2 | Mcaster1TagCap              | `Mcaster1TagCap.exe`   | External: video capture + RTMP/HTTP push (FFmpeg) | Future: launched via `QProcess` from `LiveStreamPage` |
| 3 | Mcaster1MediaPlayer         | `Mcaster1MediaPlayer.exe` | External: audio playback (FFmpeg + waveOut) | Future: launched via `QProcess::startDetached` from `MediaLibraryPage` |
| 4 | Mcaster1PlaylistComposerPro | `Mcaster1PlaylistComposerPro.exe` | External: playlist composer / broadcast log editor | Future: launched with `--playlist-id=N` from `MediaLibraryPage` |
| 5 | Mcaster1VirtualCam          | `Mcaster1VirtualCam.dll` | COM DirectShow filter (Windows-only) | **Skipped** for macOS/Linux MVP — Windows-only |
| 6 | ResizableLib                | static lib (inline)    | Window resizing framework | **Not needed** — Qt layouts handle this |


## 2. Main-app UI: MFC class → Qt class

### Root window

| v1 MFC | v1 IDD | Qt6 class | File |
|--------|--------|-----------|------|
| `CMainFrame` (custom-chrome, borderless `WS_POPUP`) | — | `MainWindow` (`QMainWindow`, **native chrome**) | `desktop/src/MainWindow.{h,cpp}` |

### Content pages (10 — hosted in `QStackedWidget`)

| v1 MFC class | v1 IDD | Qt6 widget | File | Tabs |
|--------------|--------|------------|------|------|
| `CMcaster1TagStackDlg` (Servers) | 102  | `ServersPage`       | `ServersPage.{h,cpp}`       | none — single pane |
| `CMediaLibraryPage`              | 600  | `MediaLibraryPage`  | `MediaLibraryPage.{h,cpp}`  | Library, Playlist, Export |
| `CIcy22Page`                     | 400  | `Icy22Page`         | `Icy22Page.{h,cpp}`         | Identity, Content, Engagement, Technical |
| `CLiveStreamPage`                | 700  | `LiveStreamPage`    | `LiveStreamPage.{h,cpp}`    | Audio, Video (+ dynamic) |
| `CPodcastsPage`                  | 800  | `PodcastsPage`      | `PodcastsPage.{h,cpp}`      | Episodes, Feed, Publish |
| `CSettingsPage`                  | 300  | `SettingsPage`      | `SettingsPage.{h,cpp}`      | Servers, Application, Database, About |
| `CLoggingPage`                   | 900  | `LoggingPage`       | `LoggingPage.{h,cpp}`       | Config, App Log, MySQL Log |
| `CSocialcastingPage`             | 1000 | `SocialcastingPage` | `SocialcastingPage.{h,cpp}` | Playlist, Stream, Coming Soon |
| `CDataQueuePage`                 | 1100 | `DataQueuePage`     | `DataQueuePage.{h,cpp}`     | Queue, History |
| `CEventsPage`                    | 1102 | `EventsPage`        | `EventsPage.{h,cpp}`        | Events, Run Log |

### Modal dialogs (6)

| v1 MFC class | v1 IDD | Qt6 class | File |
|--------------|--------|-----------|------|
| `CAddServerDlg`         | 200  | `AddServerDlg`         | `AddServerDlg.{h,cpp}`         |
| `CSelectMountDlg`       | 500  | `SelectMountDlg`       | `SelectMountDlg.{h,cpp}`       |
| `CScanProgressDlg`      | —    | `ScanProgressDlg`      | `ScanProgressDlg.{h,cpp}`      |
| `CMetadataEditorDlg`    | 1001 | `MetadataEditorDlg`    | `MetadataEditorDlg.{h,cpp}`    |
| `CDbBackupDlg`          | —    | `DbBackupDlg`          | `DbBackupDlg.{h,cpp}`          |
| `CDbMigrationWizardDlg` | —    | `DbMigrationWizardDlg` | `DbMigrationWizardDlg.{h,cpp}` |


## 3. MFC control → Qt widget cheat sheet

| MFC control          | Qt6 widget          |
|----------------------|---------------------|
| `CDialog`            | `QDialog`           |
| `CFrameWnd`          | `QMainWindow`       |
| `CPropertyPage`      | `QWidget` in `QTabWidget` |
| `CWnd` (custom chrome)| `QWidget` (default chrome) |
| `CListCtrl`          | `QTableWidget` (report mode) or `QListWidget` |
| `CTabCtrl`           | `QTabWidget`        |
| `CEdit`              | `QLineEdit`         |
| `CEdit ES_MULTILINE` | `QPlainTextEdit`    |
| `CRichEditCtrl`      | `QTextEdit`         |
| `CComboBox`          | `QComboBox`         |
| `CButton`            | `QPushButton`       |
| `CButton BS_CHECKBOX`| `QCheckBox`         |
| `CButton BS_RADIO`   | `QRadioButton`      |
| `CSpinButtonCtrl`    | `QSpinBox`          |
| `CProgressCtrl`      | `QProgressBar`      |
| `CStatic`            | `QLabel`            |
| `CDateTimeCtrl`      | `QDateEdit` / `QDateTimeEdit` |
| `ResizableDialog`    | (built-in to `QLayout`) |
| `CTheme` / `DarkControls` | **omitted** — Qt native style |


## 4. External-binary launch contracts

These contracts are how the v1 main app talked to its sibling binaries.
They must stay identical when we re-implement so the existing TagCap /
MediaPlayer / ComposerPro builds continue to work side-by-side with the
Qt6 desktop.

### Mcaster1TagCap
`QProcess::start("Mcaster1TagCap", args)` from `LiveStreamPage`:

```
--server   <url>
--mount    <mount>
--pass     <source-password>
--codec    vp9|vp8|h264
--res      native|1920x1080|1280x720|854x480
--fps      15|24|30|60
--bitrate  <kbps>
--vsrc     desktop|desktop-region|camera
--asrc     wasapi-loopback|mic|none
--acodec   opus|vorbis|none
--autostart           (optional)
```

Window embedding (Windows-only) used `FindWindowEx` to locate the TagCap
HWND and `SetWindowPos` to re-parent. The Qt6 port does NOT attempt
embedding — TagCap stays standalone for now.

### Mcaster1MediaPlayer
`QProcess::startDetached("Mcaster1MediaPlayer", { filePath })` from
`MediaLibraryPage`. No IPC — pure shell invocation.

### Mcaster1PlaylistComposerPro
`QProcess::startDetached("Mcaster1PlaylistComposerPro", {"--playlist-id=N"})`
from `MediaLibraryPage`. ComposerPro reads the playlist row directly from
the shared MariaDB instance via libmysql.

### Mcaster1VirtualCam (COM DLL — Windows only)
Registered via `regsvr32`. Shared-memory frame transport. **Not ported.**


## 5. Build system

```
desktop/
├── autogen.sh          # bootstrap: autoreconf -fi
├── configure.ac        # autotools config (pkg-config Qt6 + curl + openssl + yaml-cpp + taglib + libmariadb)
├── Makefile.am         # top-level: SUBDIRS = src
├── m4/                 # autoconf macros (generated)
├── build-aux/          # autotools support scripts (generated)
└── src/
    ├── Makefile.am     # bin_PROGRAMS = tagstack-desktop + MOC pattern rule
    ├── main.cpp
    ├── MainWindow.{h,cpp}
    ├── <Page>.{h,cpp}  ×10
    └── <Dialog>.{h,cpp} ×6
```

Build steps:

```bash
cd desktop
./autogen.sh
./configure
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
./src/tagstack-desktop
```

`./configure` resolves Qt6 via `pkg-config Qt6Core Qt6Widgets Qt6Network`.
MOC files are auto-generated by `Makefile.am`'s `moc_%.cpp: %.h` pattern
rule and treated as `BUILT_SOURCES`. No qmake project file, no .pro, no
.ui XML — every layout is constructed in C++.


## 6. Phase boundaries

This port is split into discrete phases so progress is observable:

| Phase | Scope | Status |
|-------|-------|--------|
| 3c-1  | Build scaffold + MainWindow + all 17 widget classes (layout only) | **Done** |
| 3c-2  | Wire login → SelectMountDlg → setActiveSession dispatch          | Pending |
| 3c-3  | Servers CRUD against daemon REST API                              | Pending |
| 3c-4  | Library scan with TagLib worker thread                            | Pending |
| 3c-5  | ICY 2.2 composer push (libcurl → daemon `/now-playing`)           | Pending |
| 3c-6  | TagCap subprocess launcher + monitor                              | Pending |
| 3c-7  | macOS bundle (`make app` target via macdeployqt)                  | Pending |

The Phase 3a + 3b daemon work (auth, directory ingest, tag CRUD, social
push queue) is already shipped and reachable from this client via
`https://tagstack.mcaster1.com:9890`.
