// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ActiveScriptWrappableAdapter_h
#define ActiveScriptWrappableAdapter_h

#include "core/CoreExport.h"
#include "core/dom/ExecutionContext.h"
#include "platform/bindings/ActiveScriptWrappable.h"
#include "platform/bindings/ScriptWrappable.h"

namespace blink {

template <typename T>
class CORE_EXPORT ActiveScriptWrappableAdapter : public ActiveScriptWrappable {
  WTF_MAKE_NONCOPYABLE(ActiveScriptWrappableAdapter);

 public:
  ActiveScriptWrappableAdapter() {}

 protected:
  bool IsContextDestroyed() const final {
    const auto* execution_context =
        static_cast<const T*>(this)->GetExecutionContext();
    return !execution_context || execution_context->IsContextDestroyed();
  }

  bool DispatchHasPendingActivity() const final {
    return static_cast<const T*>(this)->HasPendingActivity();
  }

  ScriptWrappable* ToScriptWrappable() final { return static_cast<T*>(this); }
};

}  // namespace blink

#endif  // ActiveScriptWrappableAdapter_h
