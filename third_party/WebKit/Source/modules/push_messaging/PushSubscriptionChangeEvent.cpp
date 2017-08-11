// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/push_messaging/PushSubscriptionChangeEvent.h"

#include "modules/push_messaging/PushSubscription.h"
#include "modules/push_messaging/PushSubscriptionChangeEventInit.h"

namespace blink {

PushSubscriptionChangeEvent::PushSubscriptionChangeEvent(
    const AtomicString& type,
    PushSubscription* new_subscription,
    PushSubscription* old_subscription,
    WaitUntilObserver* observer)
    : ExtendableEvent(type, ExtendableEventInit(), observer),
      new_subscription_(new_subscription),
      old_subscription_(old_subscription) {}

PushSubscriptionChangeEvent::PushSubscriptionChangeEvent(
    const AtomicString& type,
    const PushSubscriptionChangeEventInit& initializer)
    : ExtendableEvent(type, initializer) {
  if (initializer.hasNewSubscription())
    new_subscription_ = initializer.newSubscription();
  if (initializer.hasOldSubscription())
    old_subscription_ = initializer.oldSubscription();
}

PushSubscriptionChangeEvent::~PushSubscriptionChangeEvent() {}

const AtomicString& PushSubscriptionChangeEvent::InterfaceName() const {
  return EventNames::PushSubscriptionChangeEvent;
}

PushSubscription* PushSubscriptionChangeEvent::newSubscription() {
  return new_subscription_.Get();
}

PushSubscription* PushSubscriptionChangeEvent::oldSubscription() {
  return old_subscription_.Get();
}

DEFINE_TRACE(PushSubscriptionChangeEvent) {
  visitor->Trace(new_subscription_);
  visitor->Trace(old_subscription_);
  ExtendableEvent::Trace(visitor);
}

}  // namespace blink
