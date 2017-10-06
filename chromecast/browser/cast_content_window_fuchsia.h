// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_CONTENT_WINDOW_FUCHSIA_H_
#define CHROMECAST_BROWSER_CAST_CONTENT_WINDOW_FUCHSIA_H_

#include <memory>

#include "base/macros.h"
#include "chromecast/browser/cast_content_window.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace content {
class WebContents;
}  // namespace content

namespace chromecast {
namespace shell {

class CastContentWindowFuchsia : public CastContentWindow {
 public:
  // Removes the window from the screen.
  ~CastContentWindowFuchsia() override;

  // CastContentWindow implementation.
  void SetTransparent() override;
  void ShowWebContents(content::WebContents* web_contents,
                       CastWindowManager* window_manager) override;

 private:
  friend class CastContentWindow;

  // This class should only be instantiated by CastContentWindow::Create.
  CastContentWindowFuchsia();

  bool transparent_ = false;

  DISALLOW_COPY_AND_ASSIGN(CastContentWindowFuchsia);
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_CONTENT_WINDOW_FUCHSIA_H_
