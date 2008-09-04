#!/bin/sh -x

# TODO(eseidel): This script assumes it's run in third_party/WebKit
# This script should be converted to python some day
# and be made less-lame

# This script also assumes that all of the WebKit files are
# currently unforked (no .obs suffix)

# Eventually this should be read from BASE_REVISION
OLD_BASE="http://svn.webkit.org/repository/webkit/trunk"
OLD_REV=35999

NEW_BASE="http://svn.webkit.org/repository/webkit/trunk"
NEW_REV=36102

svn update
svn merge --accept theirs-full "${OLD_BASE}/JavaScriptCore@${OLD_REV}" "${NEW_BASE}/JavaScriptCore@${NEW_REV}" JavaScriptCore/
svn merge --accept theirs-full "${OLD_BASE}/WebCore@${OLD_REV}" "${NEW_BASE}/WebCore@${NEW_REV}" WebCore/
svn merge --accept theirs-full "${OLD_BASE}/WebKit@${OLD_REV}" "${NEW_BASE}/WebKit@${NEW_REV}" WebKit/
svn merge --accept theirs-full "${OLD_BASE}/WebKitLibraries@${OLD_REV}" "${NEW_BASE}/WebKitLibraries@${NEW_REV}" WebKitLibraries/

echo "${NEW_BASE}@${NEW_REV}" > BASE_REVISION
