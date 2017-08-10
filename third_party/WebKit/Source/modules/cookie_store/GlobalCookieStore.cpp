// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cookie_store/GlobalCookieStore.h"

#include <utility>

#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/workers/WorkerThread.h"
#include "modules/cookie_store/CookieStore.h"
#include "modules/serviceworkers/ServiceWorkerGlobalScope.h"
#include "platform/Supplementable.h"
#include "platform/heap/Handle.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/modules/cookie_store/cookie_store.mojom-blink.h"

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
      supplement = new GlobalCookieStoreImpl();
      Supplement<T>::ProvideTo(supplementable, GetName(), supplement);
    }
    return *supplement;
  }

  CookieStore* GetCookieStore(T& scope);

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(cookie_store_);
    Supplement<T>::Trace(visitor);
  }

 private:
  GlobalCookieStoreImpl() {}

  static const char* GetName() { return "CookieStore"; }

  CookieStore* GetCookieStore(
      service_manager::InterfaceProvider& interface_provider) {
    if (!cookie_store_) {
      mojom::blink::CookieStorePtr cookie_store_ptr;
      interface_provider.GetInterface(mojo::MakeRequest(&cookie_store_ptr));
      cookie_store_ = CookieStore::Create(std::move(cookie_store_ptr));
    }
    return cookie_store_;
  }

  Member<CookieStore> cookie_store_;
};

template <>
CookieStore* GlobalCookieStoreImpl<WorkerGlobalScope>::GetCookieStore(
    WorkerGlobalScope& worker_global_scope) {
  WorkerThread* worker_thread = worker_global_scope.GetThread();
  return GetCookieStore(worker_thread->GetInterfaceProvider());
}

template <>
CookieStore* GlobalCookieStoreImpl<LocalDOMWindow>::GetCookieStore(
    LocalDOMWindow& window) {
  LocalFrame* local_frame = window.GetFrame();
  return GetCookieStore(local_frame->GetInterfaceProvider());
}

}  // namespace

CookieStore* GlobalCookieStore::cookieStore(LocalDOMWindow& window) {
  return GlobalCookieStoreImpl<LocalDOMWindow>::From(window).GetCookieStore(
      window);
}

CookieStore* GlobalCookieStore::cookieStore(ServiceWorkerGlobalScope& worker) {
  // ServiceWorkerGlobalScope is Supplementable<WorkerGlobalScope>, not
  // Supplementable<ServiceWorkerGlobalScope>.
  return GlobalCookieStoreImpl<WorkerGlobalScope>::From(worker).GetCookieStore(
      worker);
}

}  // namespace blink
