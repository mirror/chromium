// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieStore_h
#define CookieStore_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/events/EventTarget.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "services/network/public/interfaces/restricted_cookie_manager.mojom-blink.h"

namespace blink {

class CookieStoreGetOptions;
class CookieStoreSetOptions;
class ScriptPromiseResolver;
class ScriptState;

class CookieStore final : public EventTargetWithInlineData,
                          public ContextLifecycleObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(CookieStore);

 public:
  // Needed because of the network::mojom::blink::RestrictedCookieManagerPtr
  ~CookieStore();

  static CookieStore* Create(
      ExecutionContext*,
      network::mojom::blink::RestrictedCookieManagerPtr backend);

  ScriptPromise getAll(ScriptState*,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise getAll(ScriptState*,
                       const String& name,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);

  ScriptPromise set(ScriptState*,
                    const CookieStoreSetOptions&,
                    ExceptionState&);
  ScriptPromise set(ScriptState*,
                    const String& name,
                    const String& value,
                    const CookieStoreSetOptions&,
                    ExceptionState&);
  ScriptPromise Delete(ScriptState*,
                       const CookieStoreSetOptions&,
                       ExceptionState&);
  ScriptPromise Delete(ScriptState*,
                       const String& name,
                       const CookieStoreSetOptions&,
                       ExceptionState&);

  void Trace(blink::Visitor* visitor) override {
    EventTargetWithInlineData::Trace(visitor);
    ContextLifecycleObserver::Trace(visitor);
  }

  // ActiveScriptWrappable
  void ContextDestroyed(ExecutionContext*) override;

  // EventTargetWithInlineData
  DEFINE_ATTRIBUTE_EVENT_LISTENER(change);
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;
  void RemoveAllEventListeners() override;

 protected:
  // EventTarget overrides.
  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) final;
  void RemovedEventListener(const AtomicString& event_type,
                            const RegisteredEventListener&) final;

 private:
  using DoReadBackendResultConverter =
      void (*)(ScriptPromiseResolver*,
               Vector<network::mojom::blink::CanonicalCookiePtr>);

  CookieStore(ExecutionContext*,
              network::mojom::blink::RestrictedCookieManagerPtr backend);

  // Common code in CookieStore::{get,getAll,has}.
  //
  // All cookie-reading methods use the same RestrictedCookieManager API, and
  // only differ in how they present the returned data. The difference is
  // captured in the DoReadBackendResultConverter argument, which should point
  // to one of the static methods below.
  ScriptPromise DoRead(ScriptState*,
                       const String& name,
                       const CookieStoreGetOptions&,
                       DoReadBackendResultConverter,
                       ExceptionState&);

  // Converts the result of a RestrictedCookieManager::GetAllForUrl mojo call to
  // the promise result expected by CookieStore.getAll.
  static void GetAllForUrlToGetAllResult(
      ScriptPromiseResolver*,
      Vector<network::mojom::blink::CanonicalCookiePtr> backend_result);

  // Converts the result of a RestrictedCookieManager::GetAllForUrl mojo call to
  // the promise result expected by CookieStore.get.
  static void GetAllForUrlToGetResult(
      ScriptPromiseResolver*,
      Vector<network::mojom::blink::CanonicalCookiePtr> backend_result);

  // Converts the result of a RestrictedCookieManager::GetAllForUrl mojo call to
  // the promise result expected by CookieStore.has.
  static void GetAllForUrlToHasResult(
      ScriptPromiseResolver*,
      Vector<network::mojom::blink::CanonicalCookiePtr> backend_result);

  // Common code in CookieStore::delete and CookieStore::set.
  ScriptPromise DoWrite(ScriptState*,
                        const String& name,
                        const String& value,
                        const CookieStoreSetOptions&,
                        bool is_deletion,
                        ExceptionState&);

  static void OnSetCanonicalCookieResult(ScriptPromiseResolver*,
                                         bool backend_result);

  // Called when a change event listener is added.
  //
  // This is idempotent during the time intervals between StopObserving() calls.
  void StartObserving();

  // Called when all the change event listeners have been removed.
  void StopObserving();

  network::mojom::blink::RestrictedCookieManagerPtr backend_;
};

}  // namespace blink

#endif  // CookieStore_h
