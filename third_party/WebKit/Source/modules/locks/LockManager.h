// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LockManager_h
#define LockManager_h

#include "bindings/modules/v8/string_or_string_sequence.h"
#include "modules/ModulesExport.h"
#include "modules/locks/LockOptions.h"
#include "platform/bindings/ScriptWrappable.h"

namespace blink {

class ScriptPromise;
class ScriptState;
class V8LockGrantedCallback;

class LockManager final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ScriptPromise acquire(ScriptState*,
                        const StringOrStringSequence& scope,
                        V8LockGrantedCallback*,
                        const LockOptions&,
                        ExceptionState&);
};

}  // namespace blink

#endif  // LockManager_h
