// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DedicatedWorker_h
#define DedicatedWorker_h

#include "core/CoreExport.h"
#include "core/dom/MessagePort.h"
#include "core/dom/SuspendableObject.h"
#include "core/dom/events/EventListener.h"
#include "core/dom/events/EventTarget.h"
#include "core/workers/AbstractWorker.h"
#include "platform/bindings/ActiveScriptWrappable.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class DedicatedWorkerMessagingProxy;
class ExceptionState;
class ExecutionContext;
class ScriptState;
class WorkerScriptLoader;

// TODO(nhiroki): Add the class-level comment once
// InProcessWorker->DedicatedWorker move is done (https://crbug.com/688116).
class CORE_EXPORT DedicatedWorker final
    : public AbstractWorker,
      public ActiveScriptWrappable<DedicatedWorker> {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(DedicatedWorker);
  // Eager finalization is needed to notify the parent object destruction of the
  // GC-managed messaging proxy and to initiate worker termination.
  EAGERLY_FINALIZE();

 public:
  static DedicatedWorker* Create(ExecutionContext*,
                                 const String& url,
                                 ExceptionState&);

  ~DedicatedWorker() override;

  void postMessage(ScriptState*,
                   RefPtr<SerializedScriptValue> message,
                   const MessagePortArray&,
                   ExceptionState&);
  static bool CanTransferArrayBuffersAndImageBitmaps() { return true; }
  void terminate();

  // Implements SuspendableObject.
  void ContextDestroyed(ExecutionContext*) override;

  // Implements ScriptWrappable.
  bool HasPendingActivity() const final;

  DEFINE_ATTRIBUTE_EVENT_LISTENER(message);

  DECLARE_VIRTUAL_TRACE();

 private:
  explicit DedicatedWorker(ExecutionContext*);
  bool Initialize(ExecutionContext*, const String&, ExceptionState&);

  // Creates a proxy to allow communicating with the worker's global scope.
  // DedicatedWorker does not take ownership of the created proxy. The proxy
  // is expected to manage its own lifetime, and delete itself in response to
  // terminateWorkerGlobalScope().
  DedicatedWorkerMessagingProxy* CreateMessagingProxy(ExecutionContext*);

  // Callbacks for |script_loader_|.
  void OnResponse();
  void OnFinished();

  // Implements EventTarget.
  const AtomicString& InterfaceName() const final;

  RefPtr<WorkerScriptLoader> script_loader_;

  Member<DedicatedWorkerMessagingProxy> context_proxy_;
};

}  // namespace blink

#endif  // DedicatedWorker_h
