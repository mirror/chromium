// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/browser/notifications/balloons.h"

#include "base/logging.h"

BalloonCollection::BalloonCollection(BalloonCollectionObserver* observer)
    : observer_(observer) {
  DCHECK(observer);
  observer_->OnBalloonSpaceChanged();
}

bool BalloonCollection::HasSpace() const {
  return true;
}

void BalloonCollection::Add(const Notification& notification) {
  observer_->OnBalloonSpaceChanged();
}

void BalloonCollection::ShowAll() {
}

void BalloonCollection::HideAll() {
}

#endif  // ENABLE_BACKGROUND_TASK
