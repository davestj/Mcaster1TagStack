# configure.ac — Mcaster1 TagStack desktop client.
#
# Autotools-based build for the Qt6 desktop application. Resolves Qt6 via
# pkg-config (Qt6Core / Qt6Widgets / Qt6Network ship .pc files), TagLib via
# pkg-config, libcurl via curl-config or pkg-config.

AC_PREREQ([2.69])
AC_INIT([mcaster1-tagstack-desktop], [0.1.0], [davestj@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/main.cpp])

# foreign  -> don't require GNU-standard NEWS/AUTHORS/ChangeLog files
# subdir-objects -> per-target objs live next to their sources
AM_INIT_AUTOMAKE([1.16 foreign subdir-objects -Wall -Wno-portability])

# Modern silent rules so the user sees CXX foo.o instead of the full command.
AM_SILENT_RULES([yes])

# ───────────────────────────────────────────────────────────────────────────
# Toolchain
# ───────────────────────────────────────────────────────────────────────────
AC_PROG_CXX
AC_PROG_INSTALL
AC_LANG([C++])

AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])

# ───────────────────────────────────────────────────────────────────────────
# pkg-config — required for Qt6 + every other dep
# ───────────────────────────────────────────────────────────────────────────
PKG_PROG_PKG_CONFIG([0.29])

# Qt6 — Core / Widgets / Network are the minimum for the v1 layout port.
# Multimedia is required for the player strip (QMediaPlayer + QAudioOutput).
PKG_CHECK_MODULES([QT6CORE],       [Qt6Core       >= 6.5])
PKG_CHECK_MODULES([QT6WIDGETS],    [Qt6Widgets    >= 6.5])
PKG_CHECK_MODULES([QT6NETWORK],    [Qt6Network    >= 6.5])
PKG_CHECK_MODULES([QT6MULTIMEDIA], [Qt6Multimedia >= 6.5])

# moc / rcc — find the binaries. Qt6 hides them in libexecdir; pkg-config
# exposes that. We also try the conventional names in PATH for Linux distros
# that ship qt6-base-dev with moc-qt6 / moc6 / moc.
QT6_LIBEXEC=`$PKG_CONFIG --variable=libexecdir Qt6Core 2>/dev/null`
AC_PATH_PROGS([MOC], [moc-qt6 moc6 moc], [no],
              [$QT6_LIBEXEC$PATH_SEPARATOR$PATH])
AS_IF([test "x$MOC" = "xno"],
      [AC_MSG_ERROR([moc not found — looked in $QT6_LIBEXEC and \$PATH])])
AC_PATH_PROGS([RCC], [rcc-qt6 rcc6 rcc], [no],
              [$QT6_LIBEXEC$PATH_SEPARATOR$PATH])
AS_IF([test "x$RCC" = "xno"],
      [AC_MSG_ERROR([rcc not found — looked in $QT6_LIBEXEC and \$PATH])])
AC_SUBST([MOC])
AC_SUBST([RCC])

# TagLib — audio metadata reader. Used by MediaLibrary scanning.
PKG_CHECK_MODULES([TAGLIB], [taglib >= 1.13],
                  [AC_DEFINE([HAVE_TAGLIB], [1], [TagLib present])],
                  [AC_MSG_WARN([taglib not found — media metadata scan will be a stub])])

# libcurl — server probing, ICY metadata push.
PKG_CHECK_MODULES([CURL], [libcurl >= 7.80])

# OpenSSL — TLS material for talking to the daemon API.
PKG_CHECK_MODULES([OPENSSL], [openssl >= 1.1.1])

# yaml-cpp — for parsing tagstack.yaml (server config + app settings).
PKG_CHECK_MODULES([YAMLCPP], [yaml-cpp >= 0.6])

# MariaDB client (optional — phase 3+ only; many users will route through the
# daemon API and never need a direct DB connection from the desktop).
PKG_CHECK_MODULES([MARIADB], [libmariadb],
                  [AC_DEFINE([HAVE_MARIADB], [1], [MariaDB client present])
                   have_mariadb=yes],
                  [have_mariadb=no])
AM_CONDITIONAL([HAVE_MARIADB], [test "x$have_mariadb" = "xyes"])

# ───────────────────────────────────────────────────────────────────────────
# Platform-specific
# ───────────────────────────────────────────────────────────────────────────
AC_CANONICAL_HOST
case "$host_os" in
    darwin*)  platform=macos ;;
    linux*)   platform=linux ;;
    mingw*|cygwin*|msys*) platform=windows ;;
    *)        platform=unknown ;;
esac
case "$host_cpu" in
    aarch64|arm64) host_is_arm64=yes ;;
    *)             host_is_arm64=no ;;
esac
AC_SUBST([PLATFORM], [$platform])
AM_CONDITIONAL([PLATFORM_MACOS],   [test "x$platform" = "xmacos"])
AM_CONDITIONAL([PLATFORM_LINUX],   [test "x$platform" = "xlinux"])
AM_CONDITIONAL([PLATFORM_WINDOWS], [test "x$platform" = "xwindows"])
AM_CONDITIONAL([HOST_IS_ARM64],    [test "x$host_is_arm64" = "xyes"])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_OUTPUT

# Friendly summary
cat <<EOF

──────────────────────────────────────────────────────────────────
  ${PACKAGE_NAME} ${PACKAGE_VERSION} configured.

  Compiler         : ${CXX}
  Platform         : ${platform}
  Qt6 Core         : ${QT6CORE_CFLAGS}
  TagLib           : ${TAGLIB_CFLAGS:-(not found — stub mode)}
  MariaDB client   : ${have_mariadb}
  moc              : ${MOC}
  rcc              : ${RCC}

  Next:   make -j\$(nproc 2>/dev/null || sysctl -n hw.ncpu)
──────────────────────────────────────────────────────────────────
EOF
