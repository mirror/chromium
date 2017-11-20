// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/download_source.h"

namespace download {

DownloadSource DownloadSourceFromString(const std::string& download_source) {
  if (download_source == "NAVIGATION")
    return DownloadSource::NAVIGATION;
  if (download_source == "DRAG_AND_DROP")
    return DownloadSource::DRAG_AND_DROP;
  if (download_source == "MANUAL_RESUMPTION")
    return DownloadSource::MANUAL_RESUMPTION;
  if (download_source == "AUTO_RESUMPTION")
    return DownloadSource::AUTO_RESUMPTION;
  if (download_source == "FROM_RENDERER")
    return DownloadSource::FROM_RENDERER;
  if (download_source == "EXTENSION_API")
    return DownloadSource::EXTENSION_API;
  if (download_source == "EXTENSION_INSTALLER")
    return DownloadSource::EXTENSION_INSTALLER;
  if (download_source == "PLUGIN")
    return DownloadSource::PLUGIN;
  if (download_source == "PLUGIN_INSTALLER")
    return DownloadSource::PLUGIN_INSTALLER;
  if (download_source == "INTERNAL_API")
    return DownloadSource::INTERNAL_API;
  if (download_source == "SAVE_PACKAGE")
    return DownloadSource::SAVE_PACKAGE;
  if (download_source == "OFFLINE_PAGE")
    return DownloadSource::OFFLINE_PAGE;

  return DownloadSource::UNKNOWN;
}

}  // namespace download
