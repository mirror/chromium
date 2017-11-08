// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/partial_interface.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef V8TestInterfaceSecureContextPartial_h
#define V8TestInterfaceSecureContextPartial_h

#include "bindings/core/v8/GeneratedCodeHelper.h"
#include "bindings/core/v8/NativeValueTraits.h"
#include "bindings/core/v8/ToV8ForCore.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/tests/idls/core/TestInterfaceSecureContext.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/V8DOMWrapper.h"
#include "platform/bindings/WrapperTypeInfo.h"
#include "platform/heap/Handle.h"

namespace blink {

class V8TestInterfaceSecureContextPartial {
  STATIC_ONLY(V8TestInterfaceSecureContextPartial);
 public:
  static void initialize();
  static void preparePrototypeAndInterfaceObject(v8::Local<v8::Context>, const DOMWrapperWorld&, v8::Local<v8::Object>, v8::Local<v8::Function>, v8::Local<v8::FunctionTemplate>);

  static void InstallRuntimeEnabledFeaturesOnTemplate(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::FunctionTemplate> interface_template);

  // Callback functions
  static void partialSecureContextAttributeAttributeGetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void partialSecureContextAttributeAttributeSetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void staticPartialSecureContextAttributeAttributeGetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void staticPartialSecureContextAttributeAttributeSetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void partialSecureContextRuntimeEnabledAttributeAttributeGetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void partialSecureContextRuntimeEnabledAttributeAttributeSetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void staticPartialSecureContextRuntimeEnabledAttributeAttributeGetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);
  static void staticPartialSecureContextRuntimeEnabledAttributeAttributeSetterCallback(    const v8::FunctionCallbackInfo<v8::Value>& info);

 private:
  static void installV8TestInterfaceSecureContextTemplate(v8::Isolate*, const DOMWrapperWorld&, v8::Local<v8::FunctionTemplate> interfaceTemplate);
};

}  // namespace blink

#endif  // V8TestInterfaceSecureContextPartial_h
