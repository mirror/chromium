// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Flag_h
#define Flag_h

#include "core/dom/SuspendableObject.h"
#include "modules/ModulesExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/flags/flags_service.mojom-blink.h"

namespace blink {

class ExceptionState;
class ScriptPromise;
class ScriptState;

class Flag final : public GarbageCollectedFinalized<Flag>,
                   public SuspendableObject,
                   public ScriptWrappable {
  USING_PRE_FINALIZER(Flag, Dispose);
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(Flag);

 public:
  static Flag* Create(ScriptState*,
                      const Vector<String>& scope,
                      mojom::blink::FlagMode,
                      WTF::Closure release_task);

  // Flag.idl
  Vector<String> scope() const { return scope_; }
  String mode() const;
  void waitUntil(ScriptState*, ScriptPromise&, ExceptionState&);

  // USING_PRE_FINALIZER interface - called before GC.
  void Dispose();

  // SuspendableObject
  void ContextDestroyed(ExecutionContext*) override;

  DECLARE_VIRTUAL_TRACE();

  static mojom::blink::FlagMode StringToMode(const String&);
  static String ModeToString(mojom::blink::FlagMode);

 private:
  class ThenFunction;

  Flag(ScriptState*,
       const Vector<String>& scope,
       mojom::blink::FlagMode,
       WTF::Closure release_task);

  void IncrementPending();
  void DecrementPending(bool fulfilled);
  void ReleaseIfHeld();

  int pending_count_ = 0;
  bool held_ = true;

  const Vector<String> scope_;
  const mojom::blink::FlagMode mode_;
  WTF::Closure release_task_;
};

}  // namespace blink

#endif  // Flag_h
