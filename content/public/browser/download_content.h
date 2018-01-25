// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DOWNLOAD_CONTENT_H_
#define CONTENT_PUBLIC_BROWSER_DOWNLOAD_CONTENT_H_

// The type of download based on mimetype.
// This is used by UMA and UKM metrics.
namespace content {

// NOTE: Keep in sync with DownloadContentType in
// tools/metrics/histograms/enums.xml.
enum CONTENT_EXPORT DownloadContent {
  DOWNLOAD_CONTENT_UNRECOGNIZED = 0,
  DOWNLOAD_CONTENT_TEXT = 1,
  DOWNLOAD_CONTENT_IMAGE = 2,
  DOWNLOAD_CONTENT_AUDIO = 3,
  DOWNLOAD_CONTENT_VIDEO = 4,
  DOWNLOAD_CONTENT_OCTET_STREAM = 5,
  DOWNLOAD_CONTENT_PDF = 6,
  DOWNLOAD_CONTENT_DOCUMENT = 7,
  DOWNLOAD_CONTENT_SPREADSHEET = 8,
  DOWNLOAD_CONTENT_PRESENTATION = 9,
  DOWNLOAD_CONTENT_ARCHIVE = 10,
  DOWNLOAD_CONTENT_EXECUTABLE = 11,
  DOWNLOAD_CONTENT_DMG = 12,
  DOWNLOAD_CONTENT_CRX = 13,
  DOWNLOAD_CONTENT_WEB = 14,
  DOWNLOAD_CONTENT_EBOOK = 15,
  DOWNLOAD_CONTENT_FONT = 16,
  DOWNLOAD_CONTENT_APK = 17,
  DOWNLOAD_CONTENT_MAX = 18,
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DOWNLOAD_CONTENT_H_
