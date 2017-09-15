// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_CLIENT_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_CLIENT_H_

#include "ui/aura/keyboard_lock/client.h"
#include "ui/aura/keyboard_lock/special_key_detector.h"

#include "base/logging.h"

namespace content {
class RenderWidgetHost;
class WebContents;

namespace keyboard_lock {

class Client final
    : public ui::aura::keyboard_lock::Client,
      public ui::aura::keyboard_lock::SpecialKeyDetector::Delegate {
 public:
  using NativeKeyCode = ui::aura::keyboard_lock::NativeKeyCode;
  static std::unique_ptr<ui::aura::keyboard_lock::Client> Create(
      content::WebContents* web_contents);

  ~Client() override;

  void OnKeyEvent(const ui::KeyEvent& event) override;

 private:
  Client(content::WebContents* web_contents);

  NativeKeyCode code() const override;
  void LongPressDetected() override;
  void RepeatPressDetected() override;
  void PressDetected(int repeated_count) override;

  content::WebContents* const contents_;
  content::RenderWidgetHost* const host_;
};

}  // namespace keyboard_lock
}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_CLIENT_H_
