// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/ScheduledNavigation.h"

#include "core/dom/UserGestureIndicator.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

ScheduledNavigation::ScheduledNavigation(
    Reason reason,
    double delay,
    Document* origin_document,
    scoped_refptr<SecurityOrigin> requestor_origin,
    bool replaces_current_item,
    bool is_location_change)
    : reason_(reason),
      delay_(delay),
      origin_document_(origin_document),
      requestor_origin_(requestor_origin),
      replaces_current_item_(replaces_current_item),
      is_location_change_(is_location_change) {
  if (UserGestureIndicator::ProcessingUserGesture())
    user_gesture_token_ = UserGestureIndicator::CurrentToken();
}

ScheduledNavigation::~ScheduledNavigation() {}

std::unique_ptr<UserGestureIndicator>
ScheduledNavigation::CreateUserGestureIndicator() {
  return WTF::MakeUnique<UserGestureIndicator>(user_gesture_token_);
}

scoped_refptr<SecurityOrigin> ScheduledNavigation::RequestorOrigin() const {
  return requestor_origin_;
}

}  // namespace blink
