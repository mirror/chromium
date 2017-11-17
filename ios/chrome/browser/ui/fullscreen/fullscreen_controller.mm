// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_forwarder.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenController::FullscreenController(ChromeBroadcaster* broadcaster)
    : broadcaster_(broadcaster),
      model_(base::MakeUnique<FullscreenModel>()),
      forwarder_([[FullscreenBroadcastForwarder alloc]
          initWithBroadcaster:broadcaster_
                     receiver:receiver_]) {
  DCHECK(broadcaster_);
}

FullscreenController::~FullscreenController() {}

void FullscreenController::AddObserver(FullscreenControllerObserver* observer) {
  // TODO(crbug.com/785671): Use FullscreenControllerObserverManager to keep
  // track of observers.
}

void FullscreenController::RemoveObserver(
    FullscreenControllerObserver* observer) {
  // TODO(crbug.com/785671): Use FullscreenControllerObserverManager to keep
  // track of observers.
}

bool FullscreenController::IsEnabled() const {
  return model_->enabled();
}

void FullscreenController::IncrementDisabledCounter() {
  model_->IncrementDisabledCounter();
}

void FullscreenController::DecrementDisabledCounter() {
  model_->DecrementDisabledCounter();
}
