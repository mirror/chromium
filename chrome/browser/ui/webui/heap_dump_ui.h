// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_HEAP_DUMP_UI_H_
#define CHROME_BROWSER_UI_WEBUI_HEAP_DUMP_UI_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

// The implementation for the chrome://heap-dump page.
class HeapDumpUI : public content::WebUIController {
 public:
  explicit HeapDumpUI(content::WebUI* web_ui);
  ~HeapDumpUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HeapDumpUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_HEAP_DUMP_UI_H_
