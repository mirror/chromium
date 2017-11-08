// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GlobalLockEnvironment_h
#define GlobalLockEnvironment_h

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/modules/v8/string_or_string_sequence.h"
#include "modules/ModulesExport.h"
#include "modules/locks/LockOptions.h"
#include "public/platform/modules/locks/locks_service.mojom-blink.h"

namespace blink {

class DOMWindow;
class ExceptionState;
class ScriptState;
class WorkerGlobalScope;
class V8LockGrantedCallback;

class GlobalLockEnvironment {
  STATIC_ONLY(GlobalLockEnvironment);

 public:
  static ScriptPromise requestLock(ScriptState*,
                                   DOMWindow&,
                                   const StringOrStringSequence& scope,
                                   V8LockGrantedCallback*,
                                   const LockOptions&,
                                   ExceptionState&);
  static ScriptPromise requestLock(ScriptState*,
                                   WorkerGlobalScope&,
                                   const StringOrStringSequence& scope,
                                   V8LockGrantedCallback*,
                                   const LockOptions&,
                                   ExceptionState&);
};

}  // namespace blink

#endif  // GlobalLockEnvironment_h
