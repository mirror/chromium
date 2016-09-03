// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8ScriptValueDeserializer_h
#define V8ScriptValueDeserializer_h

#include "bindings/core/v8/ScriptState.h"
#include "bindings/core/v8/SerializedScriptValue.h"
#include "wtf/Allocator.h"
#include "wtf/Noncopyable.h"
#include "wtf/RefPtr.h"
#include <v8.h>

namespace blink {

// Deserializes V8 values serialized using V8ScriptValueSerializer (or its
// predecessor, ScriptValueSerializer).
//
// Supports only basic JavaScript objects and core DOM types. Support for
// modules types is implemented in a subclass.
//
// A deserializer cannot be used multiple times; it is expected that its
// deserialize method will be invoked exactly once.
class V8ScriptValueDeserializer {
    STACK_ALLOCATED();
    WTF_MAKE_NONCOPYABLE(V8ScriptValueDeserializer);
public:
    V8ScriptValueDeserializer(RefPtr<ScriptState>, RefPtr<SerializedScriptValue>);
    v8::Local<v8::Value> deserialize();

protected:
    uint32_t version() const { return m_version; }

private:
    void transfer();

    RefPtr<ScriptState> m_scriptState;
    RefPtr<SerializedScriptValue> m_serializedScriptValue;
    v8::ValueDeserializer m_deserializer;

    // Set during deserialize after the header is read.
    uint32_t m_version = 0;
#if DCHECK_IS_ON()
    bool m_deserializeInvoked = false;
#endif
};

} // namespace blink

#endif // V8ScriptValueDeserializer_h
