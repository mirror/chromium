// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/background_fetch/BackgroundFetchedEvent.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/core/v8/ScriptState.h"
#include "core/dom/DOMException.h"
#include "modules/EventModulesNames.h"
#include "modules/background_fetch/BackgroundFetchBridge.h"
#include "modules/background_fetch/BackgroundFetchSettledFetch.h"
#include "modules/background_fetch/BackgroundFetchedEventInit.h"

namespace blink {

BackgroundFetchedEvent::BackgroundFetchedEvent(
    const AtomicString& type,
    const BackgroundFetchedEventInit& init,
    ServiceWorkerRegistration* registration)
    : BackgroundFetchEvent(type, init),
      m_fetches(init.fetches()),
      m_registration(registration) {}

BackgroundFetchedEvent::~BackgroundFetchedEvent() = default;

HeapVector<Member<BackgroundFetchSettledFetch>>
BackgroundFetchedEvent::fetches() const {
  return m_fetches;
}

ScriptPromise BackgroundFetchedEvent::updateUI(ScriptState* scriptState,
                                               const String& title) {
  if (!m_registration) {
    // Return a Promise that will never settle when a developer calls this
    // method on a BackgroundFetchedEvent instance they created themselves.
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::create(scriptState);
  ScriptPromise promise = resolver->promise();

  BackgroundFetchBridge::from(m_registration)
      ->updateUI(tag(), title,
                 WTF::bind(&BackgroundFetchedEvent::didUpdateUI,
                           wrapPersistent(this), wrapPersistent(resolver)));

  return promise;
}

void BackgroundFetchedEvent::didUpdateUI(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
      resolver->resolve();
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_TAG:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

const AtomicString& BackgroundFetchedEvent::interfaceName() const {
  return EventNames::BackgroundFetchedEvent;
}

DEFINE_TRACE(BackgroundFetchedEvent) {
  visitor->trace(m_fetches);
  visitor->trace(m_registration);
  BackgroundFetchEvent::trace(visitor);
}

}  // namespace blink
