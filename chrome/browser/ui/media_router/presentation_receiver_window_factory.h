// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_FACTORY_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_FACTORY_H_

#include <memory>

class Profile;

namespace gfx {
class Rect;
}

namespace media_router {

class PresentationReceiverWindow;

class PresentationReceiverWindowFactory {
 public:
  // Factory method for creating a window from the original user profile.  This
  // will create a new OTR profile (separate from the "user" OTR profile) for
  // the window.
  static std::unique_ptr<PresentationReceiverWindow> CreateFromOriginalProfile(
      Profile* profile,
      const gfx::Rect& bounds);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_FACTORY_H_
