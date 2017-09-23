// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GlobalFlagEnvironment_h
#define GlobalFlagEnvironment_h

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/modules/v8/StringOrStringSequence.h"
#include "modules/ModulesExport.h"
#include "modules/flags/FlagOptions.h"
#include "public/platform/modules/flags/flags_service.mojom-blink.h"

namespace blink {

class DOMWindow;
class ExceptionState;
class ScriptState;
class WorkerGlobalScope;

class GlobalFlagEnvironment {
  STATIC_ONLY(GlobalFlagEnvironment);

 public:
  static ScriptPromise requestFlag(ScriptState*,
                                   DOMWindow&,
                                   const StringOrStringSequence& scope,
                                   const String& mode,
                                   const FlagOptions&,
                                   ExceptionState&);
  static ScriptPromise requestFlag(ScriptState*,
                                   WorkerGlobalScope&,
                                   const StringOrStringSequence& scope,
                                   const String& mode,
                                   const FlagOptions&,
                                   ExceptionState&);
};

}  // namespace blink

#endif  // GlobalFlagEnvironment_h
