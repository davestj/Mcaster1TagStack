# Build rule for the Qt6 desktop application.

bin_PROGRAMS = tagstack-desktop

# Hand-written sources.
tagstack_desktop_SOURCES = \
    main.cpp \
    MainWindow.cpp \
    ServersPage.cpp \
    MediaLibraryPage.cpp \
    Icy22Page.cpp \
    LiveStreamPage.cpp \
    PodcastsPage.cpp \
    SettingsPage.cpp \
    LoggingPage.cpp \
    SocialcastingPage.cpp \
    DataQueuePage.cpp \
    EventsPage.cpp \
    AddServerDlg.cpp \
    SelectMountDlg.cpp \
    ScanProgressDlg.cpp \
    MetadataEditorDlg.cpp \
    DbBackupDlg.cpp \
    DbMigrationWizardDlg.cpp

# Generated MOC sources — one per header that declares Q_OBJECT. These must
# be built before the .cpp files compile because the .cpp files include them
# transitively via Qt's signal/slot machinery.
nodist_tagstack_desktop_SOURCES = \
    moc_MainWindow.cpp \
    moc_ServersPage.cpp \
    moc_MediaLibraryPage.cpp \
    moc_Icy22Page.cpp \
    moc_LiveStreamPage.cpp \
    moc_PodcastsPage.cpp \
    moc_SettingsPage.cpp \
    moc_LoggingPage.cpp \
    moc_SocialcastingPage.cpp \
    moc_DataQueuePage.cpp \
    moc_EventsPage.cpp \
    moc_AddServerDlg.cpp \
    moc_SelectMountDlg.cpp \
    moc_ScanProgressDlg.cpp \
    moc_MetadataEditorDlg.cpp \
    moc_DbBackupDlg.cpp \
    moc_DbMigrationWizardDlg.cpp

BUILT_SOURCES = $(nodist_tagstack_desktop_SOURCES)
CLEANFILES    = $(nodist_tagstack_desktop_SOURCES)

# Pattern rule: moc_Foo.cpp depends on Foo.h. We pass -o explicitly so the
# generated file lands in the build dir even with out-of-tree builds.
moc_%.cpp: %.h
	$(MOC) -o $@ $<

# Compilation flags. Aggregate the pkg-config-supplied flags so every file
# sees Qt6 + curl + openssl + yaml-cpp + taglib (when present).
AM_CXXFLAGS = \
    $(QT6CORE_CFLAGS) \
    $(QT6WIDGETS_CFLAGS) \
    $(QT6NETWORK_CFLAGS) \
    $(CURL_CFLAGS) \
    $(OPENSSL_CFLAGS) \
    $(YAMLCPP_CFLAGS) \
    $(TAGLIB_CFLAGS) \
    -fPIC

tagstack_desktop_LDADD = \
    $(QT6CORE_LIBS) \
    $(QT6WIDGETS_LIBS) \
    $(QT6NETWORK_LIBS) \
    $(CURL_LIBS) \
    $(OPENSSL_LIBS) \
    $(YAMLCPP_LIBS) \
    $(TAGLIB_LIBS)

if HAVE_MARIADB
AM_CXXFLAGS         += $(MARIADB_CFLAGS)
tagstack_desktop_LDADD += $(MARIADB_LIBS)
endif

if PLATFORM_MACOS
# On macOS, Qt6's plugin platform is "cocoa". Bundling is done by macdeployqt
# in a separate step (`make app` target) — Phase 4.
AM_CXXFLAGS += -DMC1_PLATFORM_MACOS=1
endif

if HOST_IS_ARM64
# Qt 6.11+ uses the __yield ARM intrinsic in qyieldcpu.h but doesn't include
# <arm_acle.h> itself. Apple clang 21 + macOS arm64 surface this as an error.
# Force-include the ACLE header to satisfy the intrinsic prototype.
AM_CXXFLAGS += -include arm_acle.h
endif

if PLATFORM_LINUX
AM_CXXFLAGS += -DMC1_PLATFORM_LINUX=1
endif

if PLATFORM_WINDOWS
AM_CXXFLAGS += -DMC1_PLATFORM_WINDOWS=1
endif
