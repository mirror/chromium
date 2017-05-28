// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MojoInterfaceInterceptor_h
#define MojoInterfaceInterceptor_h

#include "core/dom/ContextLifecycleObserver.h"
#include "core/events/EventListener.h"
#include "core/events/EventTarget.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "platform/bindings/ActiveScriptWrappable.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ExceptionState;
class ExecutionContext;
class ScriptState;

class MojoInterfaceInterceptor final
    : public EventTargetWithInlineData,
      public ActiveScriptWrappable<MojoInterfaceInterceptor>,
      public ContextLifecycleObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(MojoInterfaceInterceptor);

 public:
  static MojoInterfaceInterceptor* Create(ScriptState*,
                                          const String& interface_name);
  ~MojoInterfaceInterceptor();

  void start(ExceptionState&);
  void stop();

  DEFINE_ATTRIBUTE_EVENT_LISTENER(interfacerequest);

  DECLARE_VIRTUAL_TRACE();

  // EventTargetWithInlineData
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;

  // ActiveScriptWrappable
  bool HasPendingActivity() const final;

  // ContextLifecycleObserver
  void ContextDestroyed(ExecutionContext*) final;

 private:
  friend class V8MojoInterfaceInterceptor;

  MojoInterfaceInterceptor(ScriptState*, const String& interface_name);

  void OnInterfaceRequest(mojo::ScopedMessagePipeHandle);

  const String interface_name_;
  bool started_ = false;
};

}  // namespace blink

#endif  // MojoInterfaceInterceptor_h
