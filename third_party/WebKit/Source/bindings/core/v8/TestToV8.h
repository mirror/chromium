// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TestToV8_h
#define TestToV8_h

#include "bindings/core/v8/V8BindingForTesting.h"

namespace blink {

template <typename T>
v8::Local<v8::Value> ToV8(V8TestingScope* scope, T value) {
  return blink::ToV8(value, scope->GetContext()->Global(), scope->GetIsolate());
}

}  // namespace blink

#endif  // TestToV8_h
