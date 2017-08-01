// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieStore_h
#define CookieStore_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "modules/cookie_store/CookieStoreGetOptions.h"
#include "modules/cookie_store/CookieStoreSetOptions.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CookieStore final : public GarbageCollected<CookieStore>,
                          public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  CookieStore();

  static CookieStore* Create();

  ScriptPromise getAll(ScriptState*,
                       const String& name,
                       const CookieStoreGetOptions&,
                       ExceptionState&);
  ScriptPromise getAll(ScriptState*,
                       const CookieStoreGetOptions&,
                       const CookieStoreGetOptions& more_options,
                       ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise get(ScriptState*,
                    const CookieStoreGetOptions&,
                    const CookieStoreGetOptions& more_options,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const String& name,
                    const CookieStoreGetOptions&,
                    ExceptionState&);
  ScriptPromise has(ScriptState*,
                    const CookieStoreGetOptions&,
                    const CookieStoreGetOptions& more_options,
                    ExceptionState&);

  ScriptPromise set(ScriptState*,
                    const String& name,
                    const String& value,
                    const CookieStoreSetOptions&,
                    ExceptionState&);
  ScriptPromise remove(ScriptState*,
                       const String& name,
                       const CookieStoreSetOptions&,
                       ExceptionState&);

  DEFINE_INLINE_VIRTUAL_TRACE() {}
};

}  // namespace blink

#endif
