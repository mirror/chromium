// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_ARC_MEDIA_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_ARC_MEDIA_SOURCE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"

namespace chromeos {

// RecentSource implementation for ARC media view.
//
// All member functions must be called on the IO thread.
class RecentArcMediaSource : public RecentSource {
 public:
  RecentArcMediaSource();
  ~RecentArcMediaSource() override;

  // RecentSource overrides:
  void GetRecentFiles(RecentContext context,
                      GetRecentFilesCallback callback) override;

 private:
  class RecentFileLister;

  void OnIsArcAllowedForProfile(RecentContext context,
                                GetRecentFilesCallback callback,
                                bool is_arc_allowed);
  void OnRecentFileListed(std::unique_ptr<RecentFileLister> lister,
                          GetRecentFilesCallback callback,
                          std::vector<std::unique_ptr<RecentFile>> files);

  base::WeakPtrFactory<RecentArcMediaSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecentArcMediaSource);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_ARC_MEDIA_SOURCE_H_
