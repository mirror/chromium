// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_DOWNLOAD_METADATA_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_DOWNLOAD_METADATA_H_

#include "base/files/file_path.h"
#include "base/optional.h"
#include "components/download/public/client.h"

namespace download {

// Struct to describe general download status.
struct DownloadMetaData {
  // If the download is completed.
  bool completed;

  // The GUID of the download.
  std::string guid;

  // The file path of the download file, null if download is not completed yet
  // or failed.
  base::Optional<base::FilePath> path;

  DownloadMetaData();
  ~DownloadMetaData();
  DownloadMetaData(const DownloadMetaData& other);
  bool operator==(const DownloadMetaData& other) const;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_DOWNLOAD_METADATA_H_
