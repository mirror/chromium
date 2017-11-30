// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/media_router/presentation_receiver_window_factory.h"

#include "base/logging.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/media_router/presentation_receiver_window_views.h"

namespace media_router {

// static
std::unique_ptr<PresentationReceiverWindow>
PresentationReceiverWindowFactory::CreateFromOriginalProfile(
    Profile* profile,
    const gfx::Rect& bounds) {
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  return base::WrapUnique(new PresentationReceiverWindowViews(profile, bounds));
}

}  // namespace media_router
