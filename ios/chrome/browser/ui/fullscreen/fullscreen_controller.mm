// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

#import <Foundation/Foundation.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_forwarder.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_receiver.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer_manager.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animation_controller.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_web_state_list_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(FullscreenController);

FullscreenController::FullscreenController(Browser* browser)
    : model_(base::MakeUnique<FullscreenModel>()),
      observer_manager_(
          base::MakeUnique<FullscreenControllerObserverManager>(this,
                                                                model_.get())),
      receiver_([[FullscreenModelBroadcastReceiver alloc]
          initWithModel:model_.get()]),
      forwarder_([[FullscreenBroadcastForwarder alloc]
          initWithBroadcaster:browser->broadcaster()
                     receiver:receiver_]),
      web_state_list_observer_(base::MakeUnique<FullscreenWebStateListObserver>(
          model_.get(),
          &browser->web_state_list())) {}

FullscreenController::~FullscreenController() {
  observer_manager_->Disconnect();
  [forwarder_ disconnect];
  web_state_list_observer_->Disconnect();
}

void FullscreenController::AddObserver(FullscreenControllerObserver* observer) {
  observer_manager_->AddObserver(observer);
}

void FullscreenController::RemoveObserver(
    FullscreenControllerObserver* observer) {
  observer_manager_->RemoveObserver(observer);
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
