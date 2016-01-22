// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8Debugger_h
#define V8Debugger_h

#include "core/CoreExport.h"
#include "wtf/Forward.h"
#include "wtf/PassOwnPtr.h"

#include <v8.h>

namespace blink {

class V8DebuggerClient;

class CORE_EXPORT V8Debugger {
    USING_FAST_MALLOC(V8Debugger);
public:
    static PassOwnPtr<V8Debugger> create(v8::Isolate*, V8DebuggerClient*);
    virtual ~V8Debugger() { }

    // Each v8::Context is a part of a group. The group id is used to find approapriate
    // V8DebuggerAgent to notify about events in the context.
    // |contextGroupId| must be non-0.
    static void setContextDebugData(v8::Local<v8::Context>, const String& type, int contextGroupId);
    static int contextId(v8::Local<v8::Context>);
};

} // namespace blink


#endif // V8Debugger_h
