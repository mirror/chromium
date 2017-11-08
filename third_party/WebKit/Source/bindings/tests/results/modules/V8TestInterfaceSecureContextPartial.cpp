// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/partial_interface.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "V8TestInterfaceSecureContextPartial.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8DOMConfiguration.h"
#include "bindings/core/v8/V8TestInterfaceSecureContext.h"
#include "bindings/tests/idls/modules/PartialSecureContext.h"
#include "core/dom/ExecutionContext.h"
#include "platform/bindings/RuntimeCallStats.h"
#include "platform/bindings/V8ObjectConstructor.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/GetPtr.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

namespace TestInterfaceSecureContextPartialV8Internal {

static void partialSecureContextAttributeAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();

  TestInterfaceSecureContext* impl = V8TestInterfaceSecureContext::ToImpl(holder);

  V8SetReturnValueFast(info, PartialSecureContext::partialSecureContextAttribute(*impl), impl);
}

static void partialSecureContextAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  ALLOW_UNUSED_LOCAL(isolate);

  v8::Local<v8::Object> holder = info.Holder();
  ALLOW_UNUSED_LOCAL(holder);

  TestInterfaceSecureContext* impl = V8TestInterfaceSecureContext::ToImpl(holder);

  ExceptionState exceptionState(isolate, ExceptionState::kSetterContext, "TestInterfaceSecureContext", "partialSecureContextAttribute");

  // Prepare the value to be set.
  bool* cppValue = V8bool::ToImplWithTypeCheck(info.GetIsolate(), v8Value);

  // Type check per: http://heycam.github.io/webidl/#es-interface
  if (!cppValue) {
    exceptionState.ThrowTypeError("The provided value is not of type 'bool'.");
    return;
  }

  PartialSecureContext::setPartialSecureContextAttribute(*impl, cppValue);
}

static void staticPartialSecureContextAttributeAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  V8SetReturnValue(info, WTF::GetPtr(PartialSecureContext::staticPartialSecureContextAttribute()), info.GetIsolate()->GetCurrentContext()->Global());
}

static void staticPartialSecureContextAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  ALLOW_UNUSED_LOCAL(isolate);

  v8::Local<v8::Object> holder = info.Holder();
  ALLOW_UNUSED_LOCAL(holder);

  ExceptionState exceptionState(isolate, ExceptionState::kSetterContext, "TestInterfaceSecureContext", "staticPartialSecureContextAttribute");

  // Prepare the value to be set.
  bool* cppValue = V8bool::ToImplWithTypeCheck(info.GetIsolate(), v8Value);

  // Type check per: http://heycam.github.io/webidl/#es-interface
  if (!cppValue) {
    exceptionState.ThrowTypeError("The provided value is not of type 'bool'.");
    return;
  }

  PartialSecureContext::setStaticPartialSecureContextAttribute(cppValue);
}

static void partialSecureContextRuntimeEnabledAttributeAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();

  TestInterfaceSecureContext* impl = V8TestInterfaceSecureContext::ToImpl(holder);

  V8SetReturnValueFast(info, PartialSecureContext::partialSecureContextRuntimeEnabledAttribute(*impl), impl);
}

static void partialSecureContextRuntimeEnabledAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  ALLOW_UNUSED_LOCAL(isolate);

  v8::Local<v8::Object> holder = info.Holder();
  ALLOW_UNUSED_LOCAL(holder);

  TestInterfaceSecureContext* impl = V8TestInterfaceSecureContext::ToImpl(holder);

  ExceptionState exceptionState(isolate, ExceptionState::kSetterContext, "TestInterfaceSecureContext", "partialSecureContextRuntimeEnabledAttribute");

  // Prepare the value to be set.
  bool* cppValue = V8bool::ToImplWithTypeCheck(info.GetIsolate(), v8Value);

  // Type check per: http://heycam.github.io/webidl/#es-interface
  if (!cppValue) {
    exceptionState.ThrowTypeError("The provided value is not of type 'bool'.");
    return;
  }

  PartialSecureContext::setPartialSecureContextRuntimeEnabledAttribute(*impl, cppValue);
}

static void staticPartialSecureContextRuntimeEnabledAttributeAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  V8SetReturnValue(info, WTF::GetPtr(PartialSecureContext::staticPartialSecureContextRuntimeEnabledAttribute()), info.GetIsolate()->GetCurrentContext()->Global());
}

static void staticPartialSecureContextRuntimeEnabledAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  ALLOW_UNUSED_LOCAL(isolate);

  v8::Local<v8::Object> holder = info.Holder();
  ALLOW_UNUSED_LOCAL(holder);

  ExceptionState exceptionState(isolate, ExceptionState::kSetterContext, "TestInterfaceSecureContext", "staticPartialSecureContextRuntimeEnabledAttribute");

  // Prepare the value to be set.
  bool* cppValue = V8bool::ToImplWithTypeCheck(info.GetIsolate(), v8Value);

  // Type check per: http://heycam.github.io/webidl/#es-interface
  if (!cppValue) {
    exceptionState.ThrowTypeError("The provided value is not of type 'bool'.");
    return;
  }

  PartialSecureContext::setStaticPartialSecureContextRuntimeEnabledAttribute(cppValue);
}

} // namespace TestInterfaceSecureContextPartialV8Internal

void V8TestInterfaceSecureContextPartial::partialSecureContextAttributeAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_partialSecureContextAttribute_Getter");

  TestInterfaceSecureContextPartialV8Internal::partialSecureContextAttributeAttributeGetter(info);
}

void V8TestInterfaceSecureContextPartial::partialSecureContextAttributeAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_partialSecureContextAttribute_Setter");

  v8::Local<v8::Value> v8Value = info[0];

  TestInterfaceSecureContextPartialV8Internal::partialSecureContextAttributeAttributeSetter(v8Value, info);
}

void V8TestInterfaceSecureContextPartial::staticPartialSecureContextAttributeAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_staticPartialSecureContextAttribute_Getter");

  TestInterfaceSecureContextPartialV8Internal::staticPartialSecureContextAttributeAttributeGetter(info);
}

void V8TestInterfaceSecureContextPartial::staticPartialSecureContextAttributeAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_staticPartialSecureContextAttribute_Setter");

  v8::Local<v8::Value> v8Value = info[0];

  TestInterfaceSecureContextPartialV8Internal::staticPartialSecureContextAttributeAttributeSetter(v8Value, info);
}

void V8TestInterfaceSecureContextPartial::partialSecureContextRuntimeEnabledAttributeAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_partialSecureContextRuntimeEnabledAttribute_Getter");

  TestInterfaceSecureContextPartialV8Internal::partialSecureContextRuntimeEnabledAttributeAttributeGetter(info);
}

void V8TestInterfaceSecureContextPartial::partialSecureContextRuntimeEnabledAttributeAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_partialSecureContextRuntimeEnabledAttribute_Setter");

  v8::Local<v8::Value> v8Value = info[0];

  TestInterfaceSecureContextPartialV8Internal::partialSecureContextRuntimeEnabledAttributeAttributeSetter(v8Value, info);
}

void V8TestInterfaceSecureContextPartial::staticPartialSecureContextRuntimeEnabledAttributeAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_staticPartialSecureContextRuntimeEnabledAttribute_Getter");

  TestInterfaceSecureContextPartialV8Internal::staticPartialSecureContextRuntimeEnabledAttributeAttributeGetter(info);
}

void V8TestInterfaceSecureContextPartial::staticPartialSecureContextRuntimeEnabledAttributeAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterfaceSecureContext_staticPartialSecureContextRuntimeEnabledAttribute_Setter");

  v8::Local<v8::Value> v8Value = info[0];

  TestInterfaceSecureContextPartialV8Internal::staticPartialSecureContextRuntimeEnabledAttributeAttributeSetter(v8Value, info);
}

void V8TestInterfaceSecureContextPartial::installV8TestInterfaceSecureContextTemplate(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    v8::Local<v8::FunctionTemplate> interfaceTemplate) {
  // Initialize the interface object's template.
  V8TestInterfaceSecureContext::installV8TestInterfaceSecureContextTemplate(isolate, world, interfaceTemplate);

  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interfaceTemplate);
  ALLOW_UNUSED_LOCAL(signature);
  v8::Local<v8::ObjectTemplate> instanceTemplate = interfaceTemplate->InstanceTemplate();
  ALLOW_UNUSED_LOCAL(instanceTemplate);
  v8::Local<v8::ObjectTemplate> prototypeTemplate = interfaceTemplate->PrototypeTemplate();
  ALLOW_UNUSED_LOCAL(prototypeTemplate);

  // Register IDL constants, attributes and operations.

  // Custom signature

  V8TestInterfaceSecureContextPartial::InstallRuntimeEnabledFeaturesOnTemplate(
      isolate, world, interfaceTemplate);
}

void V8TestInterfaceSecureContextPartial::InstallRuntimeEnabledFeaturesOnTemplate(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    v8::Local<v8::FunctionTemplate> interface_template) {
  V8TestInterfaceSecureContext::InstallRuntimeEnabledFeaturesOnTemplate(isolate, world, interface_template);

  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interface_template);
  ALLOW_UNUSED_LOCAL(signature);
  v8::Local<v8::ObjectTemplate> instance_template = interface_template->InstanceTemplate();
  ALLOW_UNUSED_LOCAL(instance_template);
  v8::Local<v8::ObjectTemplate> prototype_template = interface_template->PrototypeTemplate();
  ALLOW_UNUSED_LOCAL(prototype_template);

  // Register IDL constants, attributes and operations.

  // Custom signature
}

void V8TestInterfaceSecureContextPartial::preparePrototypeAndInterfaceObject(v8::Local<v8::Context> context, const DOMWrapperWorld& world, v8::Local<v8::Object> prototypeObject, v8::Local<v8::Function> interfaceObject, v8::Local<v8::FunctionTemplate> interfaceTemplate) {
  V8TestInterfaceSecureContext::preparePrototypeAndInterfaceObject(context, world, prototypeObject, interfaceObject, interfaceTemplate);

  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interfaceTemplate);
  ExecutionContext* executionContext = ToExecutionContext(context);
  DCHECK(executionContext);
  bool isSecureContext = (executionContext && executionContext->IsSecureContext());

  if (isSecureContext) {
    static const V8DOMConfiguration::AccessorConfiguration accessor_configurations[] = {
        { "partialSecureContextAttribute", V8TestInterfaceSecureContextPartial::partialSecureContextAttributeAttributeGetterCallback, V8TestInterfaceSecureContextPartial::partialSecureContextAttributeAttributeSetterCallback, V8PrivateProperty::kNoCachedAccessor, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::kOnPrototype, V8DOMConfiguration::kCheckHolder, V8DOMConfiguration::kAllWorlds },

        { "staticPartialSecureContextAttribute", V8TestInterfaceSecureContextPartial::staticPartialSecureContextAttributeAttributeGetterCallback, V8TestInterfaceSecureContextPartial::staticPartialSecureContextAttributeAttributeSetterCallback, V8PrivateProperty::kNoCachedAccessor, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::kOnInterface, V8DOMConfiguration::kCheckHolder, V8DOMConfiguration::kAllWorlds },
    };
    V8DOMConfiguration::InstallAccessors(
        isolate, world, v8::Local<v8::Object>(), prototypeObject, interfaceObject,
        signature, accessor_configurations,
        WTF_ARRAY_LENGTH(accessor_configurations));
    if (RuntimeEnabledFeatures::SecureFeatureEnabled()) {
      static const V8DOMConfiguration::AccessorConfiguration accessor_configurations[] = {
          { "partialSecureContextRuntimeEnabledAttribute", V8TestInterfaceSecureContextPartial::partialSecureContextRuntimeEnabledAttributeAttributeGetterCallback, V8TestInterfaceSecureContextPartial::partialSecureContextRuntimeEnabledAttributeAttributeSetterCallback, V8PrivateProperty::kNoCachedAccessor, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::kOnPrototype, V8DOMConfiguration::kCheckHolder, V8DOMConfiguration::kAllWorlds },

          { "staticPartialSecureContextRuntimeEnabledAttribute", V8TestInterfaceSecureContextPartial::staticPartialSecureContextRuntimeEnabledAttributeAttributeGetterCallback, V8TestInterfaceSecureContextPartial::staticPartialSecureContextRuntimeEnabledAttributeAttributeSetterCallback, V8PrivateProperty::kNoCachedAccessor, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::kOnInterface, V8DOMConfiguration::kCheckHolder, V8DOMConfiguration::kAllWorlds },
      };
      V8DOMConfiguration::InstallAccessors(
          isolate, world, v8::Local<v8::Object>(), prototypeObject, interfaceObject,
          signature, accessor_configurations,
          WTF_ARRAY_LENGTH(accessor_configurations));
    }
  }
}

void V8TestInterfaceSecureContextPartial::initialize() {
  // Should be invoked from ModulesInitializer.
  V8TestInterfaceSecureContext::UpdateWrapperTypeInfo(
      &V8TestInterfaceSecureContextPartial::installV8TestInterfaceSecureContextTemplate,
      nullptr,
      &V8TestInterfaceSecureContextPartial::InstallRuntimeEnabledFeaturesOnTemplate,
      V8TestInterfaceSecureContextPartial::preparePrototypeAndInterfaceObject);
}

}  // namespace blink
