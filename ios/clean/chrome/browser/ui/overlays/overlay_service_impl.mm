// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/overlay_service_impl.h"

#include "base/logging.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/clean/chrome/browser/ui/overlays/browser_overlay_queue.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_queue_manager.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_scheduler.h"
#import "ios/clean/chrome/browser/ui/overlays/web_state_overlay_queue.h"
#import "ios/shared/chrome/browser/ui/browser_list/browser_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OverlayServiceImpl::OverlayServiceImpl(BrowserList* browser_list)
    : browser_list_(browser_list) {
  DCHECK(browser_list_);
  for (int i = 0; i < browser_list_->count(); ++i) {
    StartServiceForBrowser(browser_list_->GetBrowserAtIndex(i));
  }
  browser_list_->AddObserver(this);
}

void OverlayServiceImpl::OnBrowserCreated(BrowserList* browser_list,
                                          Browser* browser) {
  DCHECK_EQ(browser_list, browser_list_);
  StartServiceForBrowser(browser);
}

void OverlayServiceImpl::OnBrowserRemoved(BrowserList* browser_list,
                                          Browser* browser) {
  DCHECK_EQ(browser_list, browser_list_);
  StopServiceForBrowser(browser);
}

void OverlayServiceImpl::Shutdown() {
  for (int i = 0; i < browser_list_->count(); ++i) {
    StopServiceForBrowser(browser_list_->GetBrowserAtIndex(i));
  }
  browser_list_->RemoveObserver(this);
}

bool OverlayServiceImpl::IsBrowserShowingOverlay(Browser* browser) const {
  if (browser_list_->GetIndexOfBrowser(browser) == BrowserList::kInvalidIndex)
    return false;
  OverlayScheduler* scheduler = OverlayScheduler::FromBrowser(browser);
  return scheduler && scheduler->IsShowingOverlay();
}

void OverlayServiceImpl::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator,
    Browser* browser) {
  DCHECK(overlay_coordinator);
  DCHECK(IsBrowserShowingOverlay(browser));
  OverlayScheduler::FromBrowser(browser)->ReplaceVisibleOverlay(
      overlay_coordinator);
}

void OverlayServiceImpl::ShowOverlayForWebState(
    OverlayCoordinator* overlay_coordinator,
    web::WebState* web_state) {
  WebStateOverlayQueue::FromWebState(web_state)->AddWebStateOverlay(
      overlay_coordinator);
}

void OverlayServiceImpl::SetWebStateParentCoordinator(
    BrowserCoordinator* parent_coordinator,
    web::WebState* web_state) {
  WebStateOverlayQueue::FromWebState(web_state)->SetWebStateParentCoordinator(
      parent_coordinator);
}

void OverlayServiceImpl::ShowOverlayForBrowser(
    OverlayCoordinator* overlay_coordinator,
    BrowserCoordinator* parent_coordiantor,
    Browser* browser) {
  BrowserOverlayQueue::FromBrowser(browser)->AddBrowserOverlay(
      overlay_coordinator, parent_coordiantor);
}

void OverlayServiceImpl::CancelOverlays() {
  for (int i = 0; i < browser_list_->count(); ++i) {
    OverlayScheduler::FromBrowser(browser_list_->GetBrowserAtIndex(i))
        ->CancelOverlays();
  }
}

void OverlayServiceImpl::CancelOverlayForWebState(web::WebState* web_state) {
  WebStateOverlayQueue::FromWebState(web_state)->CancelOverlays();
}

void OverlayServiceImpl::StartServiceForBrowser(Browser* browser) {
  OverlayScheduler::CreateForBrowser(browser);
  OverlayQueueManager::CreateForBrowser(browser);
  OverlayScheduler::FromBrowser(browser)->set_queue_manager(
      OverlayQueueManager::FromBrowser(browser));
}

void OverlayServiceImpl::StopServiceForBrowser(Browser* browser) {
  OverlayScheduler::FromBrowser(browser)->set_queue_manager(nullptr);
}
