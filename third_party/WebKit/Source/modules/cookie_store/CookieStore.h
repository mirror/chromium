// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieStore_h
#define CookieStore_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
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
                          public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~CookieStore();  // needed because of the mojom::blink::CookieStorePtr

  static CookieStore* Create(mojom::blink::CookieStorePtr backend);

  ScriptPromise getAll(ScriptState*,
                       const String& name,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise getAll(ScriptState*,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const CookieStoreGetOptions&,
                    ExceptionState&);

  ScriptPromise set(ScriptState*,
                    const String& name,
                    const String& value,
                    const CookieStoreSetOptions&,
                    ExceptionState&);
  ScriptPromise deleteFunction(ScriptState*,
                               const String& name,
                               const CookieStoreSetOptions&,
                               ExceptionState&);

  DEFINE_INLINE_VIRTUAL_TRACE() {}

 private:
  CookieStore(mojom::blink::CookieStorePtr backend);

  void OnGetAllResult(ScriptPromiseResolver*,
                      WTF::Vector<mojom::blink::CookieInfoPtr> backend_result);
  void OnSetResult(ScriptPromiseResolver*, bool backend_result);
  void OnBackendConnectionError();

  // Extracts
  void ExtractCookieURLs(ScriptState*,
                         KURL* cookie_url,
                         KURL* first_party_for_cookies);

  mojom::blink::CookieStoreGetOptionsPtr ToBackendOptions(
      const CookieStoreGetOptions&,
      ExceptionState&);
  mojom::blink::CookieStoreSetOptionsPtr ToBackendOptions(
      const KURL& cookie_store,
      const CookieStoreSetOptions&,
      ExceptionState&);

  mojom::blink::CookieStorePtr backend_;
};

}  // namespace blink

#endif
