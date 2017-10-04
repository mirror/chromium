// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Lock_h
#define Lock_h

#include "core/dom/SuspendableObject.h"
#include "modules/ModulesExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/locks/locks_service.mojom-blink.h"

namespace blink {

class ScriptPromise;
class ScriptPromiseResolver;
class ScriptState;

class Lock final : public GarbageCollectedFinalized<Lock>,
                   public SuspendableObject,
                   public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(Lock);

 public:
  static Lock* Create(ScriptState*,
                      const Vector<String>& scope,
                      mojom::blink::LockMode,
                      mojom::blink::LockHandlePtr);

  ~Lock() override;

  DECLARE_VIRTUAL_TRACE();
  EAGERLY_FINALIZE();

  // Lock.idl
  Vector<String> scope() const { return scope_; }
  String mode() const;

  // SuspendableObject
  void ContextDestroyed(ExecutionContext*) override;

  // The lock is held until the passed promise resolves. When it is released,
  // the passed resolver is invoked with the promise's result.
  void HoldUntil(ScriptPromise, ScriptPromiseResolver*);

  static mojom::blink::LockMode StringToMode(const String&);
  static String ModeToString(mojom::blink::LockMode);

 private:
  class ThenFunction;

  Lock(ScriptState*,
       const Vector<String>& scope,
       mojom::blink::LockMode,
       mojom::blink::LockHandlePtr);

  void ReleaseIfHeld();

  Member<ScriptPromiseResolver> resolver_;

  const Vector<String> scope_;
  const mojom::blink::LockMode mode_;
  mojom::blink::LockHandlePtr handle_;
};

}  // namespace blink

#endif  // Lock_h
