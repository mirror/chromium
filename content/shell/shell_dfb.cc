// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/shell.h"

#include "base/logging.h"

namespace content {

void Shell::PlatformInitialize(const gfx::Size& default_window_size) {
}

void Shell::PlatformCleanUp() {
}

void Shell::PlatformEnableUIControl(UIControl control, bool is_enabled) {
  if (headless_)
    return;

  NOTIMPLEMENTED();
  return;
}

void Shell::PlatformSetAddressBarURL(const GURL& url) {
  if (headless_)
    return;

  NOTIMPLEMENTED();
}

void Shell::PlatformSetIsLoading(bool loading) {
  if (headless_)
    return;

  NOTIMPLEMENTED();
}

void Shell::PlatformCreateWindow(int width, int height) {
  if (headless_)
    return;

  NOTIMPLEMENTED();
}

void Shell::PlatformSetContents() {
  if (headless_)
    return;

  NOTIMPLEMENTED();
}

void Shell::PlatformResizeSubViews() {
  NOTIMPLEMENTED();
}

void Shell::Close() {
  if (headless_) {
    delete this;
    return;
  }

  NOTIMPLEMENTED();
}

void Shell::PlatformSetTitle(const string16& title) {
  if (headless_)
    return;

  NOTIMPLEMENTED();
}

}  // namespace content
