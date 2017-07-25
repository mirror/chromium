#!/bin/bash

# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Build and upload to CIPD a hermetic Xcode installation.
# Usage:
#   ./package_mac_xcode.sh /full/path/to/Xcode.app
#
# Note: you need to be one of the writers / owners in
#   cipd acl-list infra_internal/ios

SOURCE_APP="$1"; shift

if [ -z "$SOURCE_APP" ]; then
    echo "Missing a required path to the Xcode.app to install."
    exit 1
fi

ROOT_DIR=`pwd`
echo "ROOD_DIR=${ROOT_DIR}"
if [ ! -e "${SOURCE_APP}" ]; then
    echo "${SOURCE_APP} does not exist."
    exit 1
fi

STAGE_DIR="${ROOT_DIR}/XcodePackage"
IMAGE_NAME="Xcode.sparseimage"
IMAGE_PATH="${STAGE_DIR}/${IMAGE_NAME}"
VOLUME_NAME="Xcode"
VOLUME_PATH="/Volumes/${VOLUME_NAME}"
APP_PATH="${VOLUME_PATH}/Applications/Xcode.app"
PKG_PATH="${APP_PATH}/Contents/Resources/Packages"
EXCLUDE_LIST=( \
  "--exclude" "${SOURCE_APP}/Contents/Developer/Platforms/AppleTVOS.platform" \
  "--exclude" "${SOURCE_APP}/Contents/Developer/Platforms/WatchOS.platform")

function finish {
    if [ -e "${VOLUME_PATH}" ]; then
        hdiutil detach "${VOLUME_PATH}"
    fi
    exit
}
trap finish EXIT INT TERM ERR

echo "Creating a staging directory ${STAGE_DIR}"
mkdir -p "$STAGE_DIR"

echo "Creating a new empty image at ${IMAGE_PATH}"
hdiutil create -type SPARSE -fs 'Case-sensitive Journaled HFS+' \
        -size 20g -volname "${VOLUME_NAME}" "$IMAGE_PATH"
hdiutil attach "$IMAGE_PATH"
# mkdir "${VOLUME_PATH}/Applications"
mkdir -p "${APP_PATH}"
echo "Copying ${SOURCE_APP} to ${APP_PATH} (this may take a while)"
exclude_args=()
for f in "${EXCLUDE_LIST[@]}"; do
    exclude_args+=("--exclude" "$f")
done
(cd "${SOURCE_APP}"; tar cf - "${exclude_args[@]}" .) | \
    (cd "${APP_PATH}"; tar xf -)
# cp -a "${SOURCE_APP}" "${APP_PATH}"

echo "Installing Xcode packages at ${VOLUME_PATH}"
for f in "${PKG_PATH}/"*; do
    echo "  Installing '$f'"
    sudo installer -package "$f" -target "${VOLUME_PATH}"
done

echo "Accepting a license"
sudo bash -c "export DEVELOPER_DIR='${APP_PATH}'; \
    sudo xcodebuild -license accept"

export DEVELOPER_DIR="${APP_PATH}"
XCODE_VERSION=`xcodebuild -version | grep Xcode | sed -e 's/^Xcode //'`
BUILD_VERSION=`xcodebuild -version | grep 'Build version ' \
    | sed -e 's/^Build version //'`

hdiutil detach "${VOLUME_PATH}"

echo "Image build is complete. Xcode version: ${XCODE_VERSION}, " \
     "build version: ${BUILD_VERSION}"

echo "Uploading a CIPD package (this may take a while)"
cipd create -in "${STAGE_DIR}" \
     -name "infra_internal/ios/xcode" \
     -compression-level 9 -install-mode symlink \
     -tag "xcode_version:${XCODE_VERSION}" \
     -tag "build_version:${BUILD_VERSION}"
