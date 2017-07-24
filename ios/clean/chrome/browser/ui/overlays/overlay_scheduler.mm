// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/overlay_scheduler.h"

#include <list>

#include "base/logging.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/clean/chrome/browser/ui/commands/tab_grid_commands.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_queue.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_queue_manager.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_BROWSER_USER_DATA_KEY(OverlayScheduler);

OverlayScheduler::OverlayScheduler(Browser* browser)
    : queue_manager_(nullptr), dispatcher_(nil) {}

OverlayScheduler::~OverlayScheduler() {}

#pragma mark - Public

void OverlayScheduler::set_queue_manager(OverlayQueueManager* queue_manager) {
  if (queue_manager_) {
    queue_manager_->RemoveObserver(this);
    for (OverlayQueue* queue : queue_manager_->queues()) {
      StopObservingQueue(queue);
    }
  }
  queue_manager_ = queue_manager;
  if (queue_manager_) {
    queue_manager_->AddObserver(this);
    for (OverlayQueue* queue : queue_manager_->queues()) {
      queue->AddObserver(this);
    }
  }
}

bool OverlayScheduler::IsShowingOverlay() const {
  return !overlay_queues_.empty() &&
         overlay_queues_.front()->IsShowingOverlay();
}

void OverlayScheduler::ReplaceVisibleOverlay(
    OverlayCoordinator* overlay_coordinator) {
  DCHECK(overlay_coordinator);
  DCHECK(IsShowingOverlay());
  overlay_queues_.front()->ReplaceVisibleOverlay(overlay_coordinator);
}

void OverlayScheduler::CancelOverlays() {
  // |overlay_queues_| will be updated in OverlayQueueDidCancelOverlays(), so a
  // while loop is used to avoid using invalidated iterators.
  while (!overlay_queues_.empty()) {
    overlay_queues_.front()->CancelOverlays();
  }
}

#pragma mark - OverlayQueueManagerObserver

void OverlayScheduler::OverlayQueueManagerDidAddQueue(
    OverlayQueueManager* manager,
    OverlayQueue* queue) {
  queue->AddObserver(this);
}

void OverlayScheduler::OverlayQueueManagerWillRemoveQueue(
    OverlayQueueManager* manager,
    OverlayQueue* queue) {
  StopObservingQueue(queue);
}

#pragma mark - OverlayQueueObserver

void OverlayScheduler::OverlayQueueDidAddOverlay(OverlayQueue* queue) {
  DCHECK(queue);
  overlay_queues_.push_back(queue);
  TryToStartNextOverlay();
}

void OverlayScheduler::OverlayQueueWillReplaceVisibleOverlay(
    OverlayQueue* queue) {
  DCHECK(queue);
  DCHECK_EQ(overlay_queues_.front(), queue);
  DCHECK(queue->IsShowingOverlay());
  // An OverlayQueue's visible overlay can only be replaced if it's the first
  // queue in the scheduler and is already showing an overlay.  The queue is
  // added here so that its replacement overlay can be displayed when its
  // currently-visible overlay is stopped.
  overlay_queues_.push_front(queue);
}

void OverlayScheduler::OverlayQueueDidStopVisibleOverlay(OverlayQueue* queue) {
  DCHECK(!overlay_queues_.empty());
  DCHECK_EQ(overlay_queues_.front(), queue);
  // Only the first queue in the scheduler can start overlays, so it is expected
  // that this function is only called for that queue.
  overlay_queues_.pop_front();
  TryToStartNextOverlay();
}

void OverlayScheduler::OverlayQueueDidCancelOverlays(OverlayQueue* queue) {
  DCHECK(queue);
  // Remove all scheduled instances of |queue| from the |overlay_queues_|.
  auto i = overlay_queues_.begin();
  while (i != overlay_queues_.end()) {
    if (*i == queue)
      overlay_queues_.erase(i);
  }
  // If |queue| is currently showing an overlay, prepend it to
  // |overlay_queues_|.  It will be removed when the cancelled overlay is
  // stopped.
  if (queue->IsShowingOverlay())
    overlay_queues_.push_front(queue);
}

#pragma mark -

void OverlayScheduler::TryToStartNextOverlay() {
  // Early return if an overlay is already started or if there are no queued
  // overlays to show.
  if (overlay_queues_.empty() || IsShowingOverlay())
    return;
  // If the next queue requires a WebState's content area to be shown, switch
  // the active WebState before starting the next overlay.
  OverlayQueue* queue = overlay_queues_.front();
  web::WebState* web_state = queue->GetWebState();
  WebStateList* web_state_list = queue_manager_->web_state_list();
  if (web_state && web_state_list->GetActiveWebState() != web_state) {
    int new_active_index = web_state_list->GetIndexOfWebState(web_state);
    DCHECK_NE(new_active_index, WebStateList::kInvalidIndex);
    web_state_list->ActivateWebStateAt(new_active_index);
    [dispatcher_ showTabGridTabAtIndex:new_active_index];
  }
  // Start the next overlay in the first queue.
  queue->StartNextOverlay();
}

void OverlayScheduler::StopObservingQueue(OverlayQueue* queue) {
  queue->CancelOverlays();
  queue->RemoveObserver(this);
}
