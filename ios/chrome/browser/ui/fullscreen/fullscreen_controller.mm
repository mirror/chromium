// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_mediator.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_list_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

FullscreenController::FullscreenController()
    : model_(base::MakeUnique<FullscreenModel>()),
      mediator_(base::MakeUnique<FullscreenMediator>(this, model_.get())) {}

FullscreenController::~FullscreenController() {
  mediator_->Disconnect();
}

void FullscreenController::AddObserver(FullscreenControllerObserver* observer) {
  mediator_->AddObserver(observer);
}

void FullscreenController::RemoveObserver(
    FullscreenControllerObserver* observer) {
  mediator_->RemoveObserver(observer);
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
