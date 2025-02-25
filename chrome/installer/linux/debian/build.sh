#!/bin/bash
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e
set -o pipefail
if [ "$VERBOSE" ]; then
  set -x
fi
set -u

# Create the Debian changelog file needed by dpkg-gencontrol. This just adds a
# placeholder change, indicating it is the result of an automatic build.
# TODO(mmoss) Release packages should create something meaningful for a
# changelog, but simply grabbing the actual 'svn log' is way too verbose. Do we
# have any type of "significant/visible changes" log that we could use for this?
gen_changelog() {
  rm -f "${DEB_CHANGELOG}"
  process_template "${SCRIPTDIR}/changelog.template" "${DEB_CHANGELOG}"
  debchange -a --nomultimaint -m --changelog "${DEB_CHANGELOG}" \
    "Release Notes: ${RELEASENOTES}"
  GZLOG="${STAGEDIR}/usr/share/doc/${PACKAGE}-${CHANNEL}/changelog.gz"
  mkdir -p "$(dirname "${GZLOG}")"
  gzip -9 -c "${DEB_CHANGELOG}" > "${GZLOG}"
  chmod 644 "${GZLOG}"
}

# Create the Debian control file needed by dpkg-deb.
gen_control() {
  dpkg-gencontrol -v"${VERSIONFULL}" -c"${DEB_CONTROL}" -l"${DEB_CHANGELOG}" \
  -f"${DEB_FILES}" -p"${PACKAGE}-${CHANNEL}" -P"${STAGEDIR}" \
  -O > "${STAGEDIR}/DEBIAN/control"
  rm -f "${DEB_CONTROL}"
}

# Setup the installation directory hierachy in the package staging area.
prep_staging_debian() {
  prep_staging_common
  install -m 755 -d "${STAGEDIR}/DEBIAN" \
    "${STAGEDIR}/etc/cron.daily" \
    "${STAGEDIR}/usr/share/menu" \
    "${STAGEDIR}/usr/share/doc/${USR_BIN_SYMLINK_NAME}"
}

# Put the package contents in the staging area.
stage_install_debian() {
  # Always use a different name for /usr/bin symlink depending on channel.
  # First, to avoid file collisions. Second, to make it possible to
  # use update-alternatives for /usr/bin/google-chrome.
  local USR_BIN_SYMLINK_NAME="${PACKAGE}-${CHANNEL}"

  local PACKAGE_ORIG="${PACKAGE}"
  if [ "$CHANNEL" != "stable" ]; then
    # Avoid file collisions between channels.
    local INSTALLDIR="${INSTALLDIR}-${CHANNEL}"

    local PACKAGE="${PACKAGE}-${CHANNEL}"

    # Make it possible to distinguish between menu entries
    # for different channels.
    local MENUNAME="${MENUNAME} (${CHANNEL})"
  fi
  prep_staging_debian
  SHLIB_PERMS=644
  stage_install_common
  log_cmd echo "Staging Debian install files in '${STAGEDIR}'..."
  install -m 755 -d "${STAGEDIR}/${INSTALLDIR}/cron"
  process_template "${BUILDDIR}/installer/common/repo.cron" \
      "${STAGEDIR}/${INSTALLDIR}/cron/${PACKAGE}"
  chmod 755 "${STAGEDIR}/${INSTALLDIR}/cron/${PACKAGE}"
  pushd "${STAGEDIR}/etc/cron.daily/" > /dev/null
  ln -snf "${INSTALLDIR}/cron/${PACKAGE}" "${PACKAGE}"
  popd > /dev/null
  process_template "${BUILDDIR}/installer/debian/debian.menu" \
    "${STAGEDIR}/usr/share/menu/${PACKAGE}.menu"
  chmod 644 "${STAGEDIR}/usr/share/menu/${PACKAGE}.menu"
  process_template "${BUILDDIR}/installer/debian/postinst" \
    "${STAGEDIR}/DEBIAN/postinst"
  chmod 755 "${STAGEDIR}/DEBIAN/postinst"
  process_template "${BUILDDIR}/installer/debian/prerm" \
    "${STAGEDIR}/DEBIAN/prerm"
  chmod 755 "${STAGEDIR}/DEBIAN/prerm"
  process_template "${BUILDDIR}/installer/debian/postrm" \
    "${STAGEDIR}/DEBIAN/postrm"
  chmod 755 "${STAGEDIR}/DEBIAN/postrm"
}

verify_package() {
  local DEPENDS="$1"
  local EXPECTED_DEPENDS="${TMPFILEDIR}/expected_deb_depends"
  local ACTUAL_DEPENDS="${TMPFILEDIR}/actual_deb_depends"
  echo ${DEPENDS} | sed 's/, /\n/g' | LANG=C sort > "${EXPECTED_DEPENDS}"
  dpkg -I "${PACKAGE}-${CHANNEL}_${VERSIONFULL}_${ARCHITECTURE}.deb" | \
      grep '^ Depends: ' | sed 's/^ Depends: //' | sed 's/, /\n/g' | \
      LANG=C sort > "${ACTUAL_DEPENDS}"
  BAD_DIFF=0
  diff -u "${EXPECTED_DEPENDS}" "${ACTUAL_DEPENDS}" || BAD_DIFF=1
  if [ $BAD_DIFF -ne 0 ]; then
    echo
    echo "ERROR: bad dpkg dependencies!"
    echo
    exit $BAD_DIFF
  fi
}

# Actually generate the package file.
do_package() {
  log_cmd echo "Packaging ${ARCHITECTURE}..."
  PREDEPENDS="$COMMON_PREDEPS"
  DEPENDS="${COMMON_DEPS}"
  PROVIDES="www-browser"
  gen_changelog
  process_template "${SCRIPTDIR}/control.template" "${DEB_CONTROL}"
  export DEB_HOST_ARCH="${ARCHITECTURE}"
  if [ -f "${DEB_CONTROL}" ]; then
    gen_control
  fi
  if [ ${IS_OFFICIAL_BUILD} -ne 0 ]; then
    local COMPRESSION_OPTS="-Zxz -z9"
  else
    local COMPRESSION_OPTS="-Znone"
  fi
  log_cmd fakeroot dpkg-deb ${COMPRESSION_OPTS} -b "${STAGEDIR}" .
  verify_package "$DEPENDS"
}

# Remove temporary files and unwanted packaging output.
cleanup() {
  log_cmd echo "Cleaning..."
  rm -rf "${STAGEDIR}"
  rm -rf "${TMPFILEDIR}"
}

usage() {
  echo "usage: $(basename $0) [-a target_arch] [-b 'dir'] -c channel"
  echo "                      -d branding [-f] [-o 'dir'] -s 'dir' -t target_os"
  echo "-a arch      package architecture (ia32 or x64)"
  echo "-b dir       build input directory    [${BUILDDIR}]"
  echo "-c channel   the package channel (unstable, beta, stable)"
  echo "-d brand     either chromium or google_chrome"
  echo "-f           indicates that this is an official build"
  echo "-h           this help message"
  echo "-o dir       package output directory [${OUTPUTDIR}]"
  echo "-s dir       /path/to/sysroot"
  echo "-t platform  target platform"
}

# Check that the channel name is one of the allowable ones.
verify_channel() {
  case $CHANNEL in
    stable )
      CHANNEL=stable
      RELEASENOTES="http://googlechromereleases.blogspot.com/search/label/Stable%20updates"
      ;;
    unstable|dev|alpha )
      CHANNEL=unstable
      RELEASENOTES="http://googlechromereleases.blogspot.com/search/label/Dev%20updates"
      ;;
    testing|beta )
      CHANNEL=beta
      RELEASENOTES="http://googlechromereleases.blogspot.com/search/label/Beta%20updates"
      ;;
    * )
      echo
      echo "ERROR: '$CHANNEL' is not a valid channel type."
      echo
      exit 1
      ;;
  esac
}

process_opts() {
  while getopts ":a:b:c:d:fho:s:t:" OPTNAME
  do
    case $OPTNAME in
      a )
        TARGETARCH="$OPTARG"
        ;;
      b )
        BUILDDIR=$(readlink -f "${OPTARG}")
        ;;
      c )
        CHANNEL="$OPTARG"
        ;;
      d )
        BRANDING="$OPTARG"
        ;;
      f )
        IS_OFFICIAL_BUILD=1
        ;;
      h )
        usage
        exit 0
        ;;
      o )
        OUTPUTDIR=$(readlink -f "${OPTARG}")
        mkdir -p "${OUTPUTDIR}"
        ;;
      s )
        SYSROOT="$OPTARG"
        ;;
      t )
        TARGET_OS="$OPTARG"
        ;;
     \: )
        echo "'-$OPTARG' needs an argument."
        usage
        exit 1
        ;;
      * )
        echo "invalid command-line option: $OPTARG"
        usage
        exit 1
        ;;
    esac
  done
}

#=========
# MAIN
#=========

SCRIPTDIR=$(readlink -f "$(dirname "$0")")
OUTPUTDIR="${PWD}"
# Default target architecture to same as build host.
if [ "$(uname -m)" = "x86_64" ]; then
  TARGETARCH="x64"
else
  TARGETARCH="ia32"
fi

# call cleanup() on exit
trap cleanup 0
process_opts "$@"
BUILDDIR=${BUILDDIR:=$(readlink -f "${SCRIPTDIR}/../../../../out/Release")}
IS_OFFICIAL_BUILD=${IS_OFFICIAL_BUILD:=0}

STAGEDIR="${BUILDDIR}/deb-staging-${CHANNEL}"
mkdir -p "${STAGEDIR}"
TMPFILEDIR="${BUILDDIR}/deb-tmp-${CHANNEL}"
mkdir -p "${TMPFILEDIR}"
DEB_CHANGELOG="${TMPFILEDIR}/changelog"
DEB_FILES="${TMPFILEDIR}/files"
DEB_CONTROL="${TMPFILEDIR}/control"

source ${BUILDDIR}/installer/common/installer.include

get_version_info
VERSIONFULL="${VERSION}-${PACKAGE_RELEASE}"

if [ "$BRANDING" = "google_chrome" ]; then
  source "${BUILDDIR}/installer/common/google-chrome.info"
else
  source "${BUILDDIR}/installer/common/chromium-browser.info"
fi
eval $(sed -e "s/^\([^=]\+\)=\(.*\)$/export \1='\2'/" \
  "${BUILDDIR}/installer/theme/BRANDING")

verify_channel

# Some Debian packaging tools want these set.
export DEBFULLNAME="${MAINTNAME}"
export DEBEMAIL="${MAINTMAIL}"

DEB_COMMON_DEPS="${BUILDDIR}/deb_common.deps"
COMMON_DEPS=$(sed ':a;N;$!ba;s/\n/, /g' "${DEB_COMMON_DEPS}")
COMMON_PREDEPS="dpkg (>= 1.14.0)"


# Make everything happen in the OUTPUTDIR.
cd "${OUTPUTDIR}"

case "$TARGETARCH" in
  arm )
    export ARCHITECTURE="armhf"
    ;;
  ia32 )
    export ARCHITECTURE="i386"
    ;;
  x64 )
    export ARCHITECTURE="amd64"
    ;;
  mipsel )
    export ARCHITECTURE="mipsel"
    ;;
  mips64el )
    export ARCHITECTURE="mips64el"
    ;;
  * )
    echo
    echo "ERROR: Don't know how to build DEBs for '$TARGETARCH'."
    echo
    exit 1
    ;;
esac
BASEREPOCONFIG="dl.google.com/linux/chrome/deb/ stable main"
# Only use the default REPOCONFIG if it's unset (e.g. verify_channel might have
# set it to an empty string)
REPOCONFIG="${REPOCONFIG-deb [arch=${ARCHITECTURE}] http://${BASEREPOCONFIG}}"
# Allowed configs include optional HTTPS support and explicit multiarch
# platforms.
REPOCONFIGREGEX="deb (\\\\[arch=[^]]*\\\\b${ARCHITECTURE}\\\\b[^]]*\\\\]"
REPOCONFIGREGEX+="[[:space:]]*) https?://${BASEREPOCONFIG}"
stage_install_debian

do_package
