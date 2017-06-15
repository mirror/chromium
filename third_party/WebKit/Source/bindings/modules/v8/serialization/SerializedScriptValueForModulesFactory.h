// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SerializedScriptValueForModulesFactory_h
#define SerializedScriptValueForModulesFactory_h

#include "bindings/core/v8/serialization/SerializedScriptValueFactory.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class SerializedScriptValueForModulesFactory final
    : public SerializedScriptValueFactory {
  WTF_MAKE_NONCOPYABLE(SerializedScriptValueForModulesFactory);

 public:
  constexpr SerializedScriptValueForModulesFactory() {}

 protected:
  PassRefPtr<SerializedScriptValue> Create(
      v8::Isolate*,
      v8::Local<v8::Value>,
      const SerializedScriptValue::SerializeOptions&,
      ExceptionState&) const override;

  v8::Local<v8::Value> Deserialize(
      SerializedScriptValue*,
      v8::Isolate*,
      const SerializedScriptValue::DeserializeOptions&) const override;
};

}  // namespace blink

#endif  // SerializedScriptValueForModulesFactory_h
