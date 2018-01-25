// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/CSSPropertyNames.h"
#include "core/CoreExport.h"
#include "platform/wtf/text/AtomicString.h"
#include "v8/include/v8.h"

namespace blink {

class ExceptionState;

//
CORE_EXPORT bool ParsePrototype(v8::Isolate*,
                                v8::Local<v8::Context>,
                                v8::Local<v8::Function> constructor,
                                v8::Local<v8::Object>* prototype,
                                ExceptionState*,
                                v8::TryCatch*);

CORE_EXPORT bool ParseFunction(v8::Isolate*,
                               v8::Local<v8::Context>,
                               v8::Local<v8::Object> prototype,
                               const AtomicString function_name,
                               v8::Local<v8::Function>*,
                               ExceptionState*,
                               v8::TryCatch*);

CORE_EXPORT bool ParseGeneratorFunction(v8::Isolate*,
                                        v8::Local<v8::Context>,
                                        v8::Local<v8::Object> prototype,
                                        const AtomicString function_name,
                                        v8::Local<v8::Function>*,
                                        ExceptionState*,
                                        v8::TryCatch*);

CORE_EXPORT bool ParseCSSPropertyList(v8::Isolate*,
                                      v8::Local<v8::Context>,
                                      v8::Local<v8::Function> constructor,
                                      const AtomicString list_name,
                                      Vector<CSSPropertyID>* native_properties,
                                      Vector<AtomicString>* custom_properties,
                                      ExceptionState*,
                                      v8::TryCatch*);

}  // namespace blink
