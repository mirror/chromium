// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_SOURCE_H_
#define COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_SOURCE_H_

#include <string>

namespace download {
// The source of download.
// Used in UMA metrics. Entries in this enum can't be deleted or reused.
enum class DownloadSource {
  // The source is unknown.
  UNKNOWN = 0,

  // Download is triggered from navigation request.
  NAVIGATION = 1,

  // Drag and drop.
  DRAG_AND_DROP = 2,

  // User manually resume the download.
  MANUAL_RESUMPTION = 3,

  // Auto resumption in download system.
  AUTO_RESUMPTION = 4,

  // Renderer initiated download, mostly from Javascript or HTML <a> tag.
  FROM_RENDERER = 5,

  // Extension download API.
  EXTENSION_API = 6,

  // Extension web store installer.
  EXTENSION_INSTALLER = 7,

  // Plugin triggered download.
  PLUGIN = 8,

  // Plugin installer download.
  PLUGIN_INSTALLER = 9,

  // Download service API background download.
  INTERNAL_API = 10,

  // Save package download.
  SAVE_PACKAGE = 11,

  // Offline page download.
  OFFLINE_PAGE = 12,

  COUNT = 13
};

// Converts a string tag to DownloadSource.
DownloadSource DownloadSourceFromString(const std::string& download_source);

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_DOWNLOADER_IN_PROGRESS_DOWNLOAD_SOURCE_H_
