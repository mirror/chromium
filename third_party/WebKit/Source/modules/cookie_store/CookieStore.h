// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieStore_h
#define CookieStore_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/cookie_store/cookie_store.mojom-blink.h"

namespace blink {

class CookieStoreGetOptions;
class CookieStoreSetOptions;
class ScriptPromiseResolver;
class ScriptState;

class CookieStore final : public GarbageCollectedFinalized<CookieStore>,
                          public ScriptWrappable,
                          public ContextLifecycleObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(CookieStore);

 public:
  ~CookieStore();  // needed because of the mojom::blink::CookieStorePtr

  static CookieStore* Create(ExecutionContext*,
                             mojom::blink::CookieStorePtr backend);

  ScriptPromise getAll(ScriptState*,
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

  DEFINE_INLINE_VIRTUAL_TRACE() { ContextLifecycleObserver::Trace(visitor); }

  // ActiveScriptWrappable
  void ContextDestroyed(ExecutionContext*) override;

 private:
  explicit CookieStore(ExecutionContext*, mojom::blink::CookieStorePtr backend);

  void OnGetAllResult(ScriptPromiseResolver*,
                      Vector<mojom::blink::CookieInfoPtr> backend_result);
  void OnSetResult(ScriptPromiseResolver*, bool backend_result);

  mojom::blink::CookieStorePtr backend_;
};

}  // namespace blink

#endif  // CookieStore_h
