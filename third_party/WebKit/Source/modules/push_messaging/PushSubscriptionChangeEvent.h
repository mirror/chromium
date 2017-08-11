// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PushSubscriptionChangeEvent_h
#define PushSubscriptionChangeEvent_h

#include "modules/EventModules.h"
#include "modules/ModulesExport.h"
#include "modules/serviceworkers/ExtendableEvent.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/AtomicString.h"

namespace blink {

class PushSubscription;
class PushSubscriptionChangeEventInit;

class MODULES_EXPORT PushSubscriptionChangeEvent final
    : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PushSubscriptionChangeEvent* Create(const AtomicString& type,
                                             PushSubscription* new_subscription,
                                             PushSubscription* old_subscription,
                                             WaitUntilObserver* observer) {
    return new PushSubscriptionChangeEvent(type, new_subscription,
                                           old_subscription, observer);
  }

  static PushSubscriptionChangeEvent* Create(
      const AtomicString& type,
      const PushSubscriptionChangeEventInit& initializer) {
    return new PushSubscriptionChangeEvent(type, initializer);
  }

  ~PushSubscriptionChangeEvent() override;

  // ExtendableEvent interface.
  const AtomicString& InterfaceName() const override;

  PushSubscription* newSubscription();
  PushSubscription* oldSubscription();

  DECLARE_VIRTUAL_TRACE();

 private:
  PushSubscriptionChangeEvent(const AtomicString& type,
                              PushSubscription* new_subscription,
                              PushSubscription* old_subscription,
                              WaitUntilObserver*);
  PushSubscriptionChangeEvent(const AtomicString& type,
                              const PushSubscriptionChangeEventInit&);

  Member<PushSubscription> new_subscription_;
  Member<PushSubscription> old_subscription_;
};

}  // namespace blink

#endif  // PushSubscriptionChangeEvent_h
