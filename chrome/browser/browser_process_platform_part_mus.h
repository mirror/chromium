// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_MUS_H_
#define CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_MUS_H_

#include <memory>

#include "chrome/browser/browser_process_platform_part_base.h"

namespace ui {
class ImageCursorsSet;
}

class BrowserProcessPlatformPartMus : public BrowserProcessPlatformPartBase {
 public:
  BrowserProcessPlatformPartMus();
  ~BrowserProcessPlatformPartMus() override;

  // Overridden from BrowserProcessPlatformPartBase:
  void RegisterInProcessServices(
      content::ContentBrowserClient::StaticServiceMap* services) override;

  void DestroyImageCursorsSet();

 private:
  // Used by the UI Service.
  std::unique_ptr<ui::ImageCursorsSet> image_cursors_set_;

  DISALLOW_COPY_AND_ASSIGN(BrowserProcessPlatformPartMus);
};

#endif  // CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_MUS_H_
