// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image_destruction_tracker.h"

namespace cc {

// static
PaintImageDestructionTracker* PaintImageDestructionTracker::GetInstance() {
  return base::Singleton<PaintImageDestructionTracker>::get();
}

PaintImageDestructionTracker::PaintImageDestructionTracker()
    : observers_(new base::ObserverListThreadSafe<Observer>) {}

PaintImageDestructionTracker::~PaintImageDestructionTracker() = default;

void PaintImageDestructionTracker::AddObserver(Observer* obs) {
  observers_->AddObserver(obs);
}

void PaintImageDestructionTracker::RemoveObserver(Observer* obs) {
  observers_->RemoveObserver(obs);
}

void PaintImageDestructionTracker::NotifyImageDestroyed(
    PaintImage::Id paint_image_id,
    PaintImage::ContentId content_id) {
  observers_->Notify(FROM_HERE, &Observer::DestroyCachedDecode, paint_image_id,
                     content_id);
}

}  // namespace cc
