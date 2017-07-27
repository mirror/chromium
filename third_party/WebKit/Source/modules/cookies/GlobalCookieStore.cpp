// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cookies/GlobalCookieStore.h"

#include "core/frame/LocalDOMWindow.h"
#include "core/workers/ServiceWorkerGlobalScope.h"
#include "modules/cookies/CookieStore.h"
#include "platform/Supplementable.h"
#include "platform/heap/Handle.h"

namespace blink {

namespace {

template <typename T>
class GlobalCookieStoreImpl final
    : public GarbageCollected<GlobalCookieStoreImpl<T>>,
      public Supplement<T> {
  USING_GARBAGE_COLLECTED_MIXIN(GlobalCookieStoreImpl);

 public:
  static GlobalCookieStoreImpl& From(T& supplementable) {
    GlobalCookieStoreImpl* supplement = static_cast<GlobalCookieStoreImpl*>(
        Supplement<T>::From(supplementable, GetName()));
    if (!supplement) {
      supplement = new GlobalCookieStoreImpl;
      Supplement<T>::ProvideTo(supplementable, GetName(), supplement);
    }
    return *supplement;
  }

  CookieStore* CookieStore(T& fetching_scope) {
    if (!cookie_store_)
      cookie_store_ = CookieStore::Create();
    return cookie_store_;
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(cookie_store_);
    Supplement<T>::Trace(visitor);
  }

 private:
  GlobalCookieStoreImpl() {}

  static const char* GetName() { return "CookieStore"; }

  Member<CookieStore> cookie_store_;
};

}  // namespace

CookieStore* GlobalCookieStore::cookieStore(LocalDOMWindow& window) {
  return GlobalCookieStoreImpl<LocalDOMWindow>::From(window).CookieStore(
      window);
}

CookieStore* GlobalCookieStore::cookieStore(ServiceWorkerGlobalScope& worker) {
  return GlobalCookieStoreImpl<ServiceWorkerGlobalScope>::From(worker)
      .CookieStore(worker);
}

}  // namespace blink
