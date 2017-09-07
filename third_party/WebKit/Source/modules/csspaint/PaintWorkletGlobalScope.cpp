// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/PaintWorkletGlobalScope.h"

#include "bindings/core/v8/IDLTypes.h"
#include "bindings/core/v8/NativeValueTraitsImpl.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/CSSPropertyNames.h"
#include "core/css/CSSSyntaxDescriptor.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/inspector/MainThreadDebugger.h"
#include "modules/csspaint/CSSPaintDefinition.h"
#include "modules/csspaint/CSSPaintImageGeneratorImpl.h"
#include "modules/csspaint/CSSPaintWorklet.h"
#include "modules/csspaint/PaintWorklet.h"
#include "platform/bindings/V8BindingMacros.h"

namespace blink {

namespace {

bool ParseInputProperties(v8::Isolate* isolate,
                          v8::Local<v8::Context>& context,
                          v8::Local<v8::Function>& constructor,
                          Vector<CSSPropertyID>& native_invalidation_properties,
                          Vector<AtomicString>& custom_invalidation_properties,
                          ExceptionState& exception_state) {
  v8::Local<v8::Value> input_properties_value;
  if (!constructor->Get(context, V8AtomicString(isolate, "inputProperties"))
           .ToLocal(&input_properties_value))
    return false;

  if (!IsUndefinedOrNull(input_properties_value)) {
    Vector<String> properties =
        NativeValueTraits<IDLSequence<IDLString>>::NativeValue(
            isolate, input_properties_value, exception_state);

    if (exception_state.HadException())
      return false;

    for (const auto& property : properties) {
      CSSPropertyID property_id = cssPropertyID(property);
      if (property_id == CSSPropertyVariable) {
        custom_invalidation_properties.push_back(property);
      } else if (property_id != CSSPropertyInvalid) {
        native_invalidation_properties.push_back(property_id);
      }
    }
  }
  return true;
}

bool ParseInputArguments(v8::Isolate* isolate,
                         v8::Local<v8::Context>& context,
                         v8::Local<v8::Function>& constructor,
                         Vector<CSSSyntaxDescriptor>& input_argument_types,
                         ExceptionState& exception_state) {
  if (RuntimeEnabledFeatures::CSSPaintAPIArgumentsEnabled()) {
    v8::Local<v8::Value> input_argument_type_values;
    if (!constructor->Get(context, V8AtomicString(isolate, "inputArguments"))
             .ToLocal(&input_argument_type_values))
      return false;

    if (!IsUndefinedOrNull(input_argument_type_values)) {
      Vector<String> argument_types =
          NativeValueTraits<IDLSequence<IDLString>>::NativeValue(
              isolate, input_argument_type_values, exception_state);

      if (exception_state.HadException())
        return false;

      for (const auto& type : argument_types) {
        CSSSyntaxDescriptor syntax_descriptor(type);
        if (!syntax_descriptor.IsValid()) {
          exception_state.ThrowTypeError("Invalid argument types.");
          return false;
        }
        input_argument_types.push_back(syntax_descriptor);
      }
    }
  }
  return true;
}

bool ParsePaintRenderingContext2DSettings(v8::Isolate* isolate,
                                          v8::Local<v8::Context>& context,
                                          v8::Local<v8::Function>& constructor,
                                          bool& has_alpha,
                                          ExceptionState& exception_state) {
  v8::Local<v8::Value> context_settings_value;
  if (!constructor
           ->Get(context,
                 V8AtomicString(isolate, "paintRenderingContext2DSettings"))
           .ToLocal(&context_settings_value))
    return false;
  if (!IsUndefinedOrNull(context_settings_value) &&
      !context_settings_value->IsObject()) {
    exception_state.ThrowTypeError(
        "The 'paintRenderingContext2DSettings' property on the class is not an "
        "object.");
    return false;
  }
  if (context_settings_value->IsObject()) {
    v8::Local<v8::Object> context_settings_object =
        context_settings_value.As<v8::Object>();
    v8::Local<v8::Value> alpha_value;
    static const char* const kKeys[] = {"alpha"};
    const v8::Eternal<v8::Name>* keys =
        V8PerIsolateData::From(isolate)->FindOrCreateEternalNameCache(
            kKeys, kKeys, WTF_ARRAY_LENGTH(kKeys));
    if (!context_settings_object->Get(context, keys[0].Get(isolate))
             .ToLocal(&alpha_value)) {
      exception_state.ThrowTypeError(
          "The 'PaintRenderingContext2DSettings' property on the class has no "
          "alpha property.");
      return false;
    }
    if (alpha_value.IsEmpty() || alpha_value->IsUndefined()) {
      // Fall back to default
    } else {
      if (!alpha_value->IsBoolean()) {
        exception_state.ThrowTypeError(
            "The 'PaintRenderingContext2DSettings' property on the class has "
            "no alpha property.");
        return false;
      }
      has_alpha = NativeValueTraits<IDLBoolean>::NativeValue(
          isolate, alpha_value, exception_state);
    }
  }
  return true;
}

bool ParsePaintFunction(v8::Isolate* isolate,
                        v8::Local<v8::Context>& context,
                        v8::Local<v8::Function>& constructor,
                        v8::Local<v8::Function>& paint,
                        ExceptionState& exception_state) {
  v8::Local<v8::Value> prototype_value;
  if (!constructor->Get(context, V8AtomicString(isolate, "prototype"))
           .ToLocal(&prototype_value))
    return false;

  if (IsUndefinedOrNull(prototype_value)) {
    exception_state.ThrowTypeError(
        "The 'prototype' object on the class does not exist.");
    return false;
  }

  if (!prototype_value->IsObject()) {
    exception_state.ThrowTypeError(
        "The 'prototype' property on the class is not an object.");
    return false;
  }

  v8::Local<v8::Object> prototype =
      v8::Local<v8::Object>::Cast(prototype_value);

  v8::Local<v8::Value> paint_value;
  if (!prototype->Get(context, V8AtomicString(isolate, "paint"))
           .ToLocal(&paint_value))
    return false;

  if (IsUndefinedOrNull(paint_value)) {
    exception_state.ThrowTypeError(
        "The 'paint' function on the prototype does not exist.");
    return false;
  }

  if (!paint_value->IsFunction()) {
    exception_state.ThrowTypeError(
        "The 'paint' property on the prototype is not a function.");
    return false;
  }

  paint = v8::Local<v8::Function>::Cast(paint_value);
  return true;
}

}  // namespace

// static
PaintWorkletGlobalScope* PaintWorkletGlobalScope::Create(
    LocalFrame* frame,
    const KURL& url,
    const String& user_agent,
    PassRefPtr<SecurityOrigin> security_origin,
    v8::Isolate* isolate,
    WorkerReportingProxy& reporting_proxy,
    PaintWorkletPendingGeneratorRegistry* pending_generator_registry,
    size_t global_scope_number) {
  PaintWorkletGlobalScope* paint_worklet_global_scope =
      new PaintWorkletGlobalScope(frame, url, user_agent,
                                  std::move(security_origin), isolate,
                                  reporting_proxy, pending_generator_registry);
  String context_name("PaintWorklet #");
  context_name.append(String::Number(global_scope_number));
  paint_worklet_global_scope->ScriptController()->InitializeContextIfNeeded(
      context_name);
  MainThreadDebugger::Instance()->ContextCreated(
      paint_worklet_global_scope->ScriptController()->GetScriptState(),
      paint_worklet_global_scope->GetFrame(),
      paint_worklet_global_scope->GetSecurityOrigin());
  return paint_worklet_global_scope;
}

PaintWorkletGlobalScope::PaintWorkletGlobalScope(
    LocalFrame* frame,
    const KURL& url,
    const String& user_agent,
    PassRefPtr<SecurityOrigin> security_origin,
    v8::Isolate* isolate,
    WorkerReportingProxy& reporting_proxy,
    PaintWorkletPendingGeneratorRegistry* pending_generator_registry)
    : MainThreadWorkletGlobalScope(frame,
                                   url,
                                   user_agent,
                                   std::move(security_origin),
                                   isolate,
                                   reporting_proxy),
      pending_generator_registry_(pending_generator_registry) {}

PaintWorkletGlobalScope::~PaintWorkletGlobalScope() {}

void PaintWorkletGlobalScope::Dispose() {
  MainThreadDebugger::Instance()->ContextWillBeDestroyed(
      ScriptController()->GetScriptState());

  pending_generator_registry_ = nullptr;
  WorkletGlobalScope::Dispose();
}

void PaintWorkletGlobalScope::registerPaint(const String& name,
                                            const ScriptValue& ctor_value,
                                            ExceptionState& exception_state) {
  if (name.IsEmpty()) {
    exception_state.ThrowTypeError("The empty string is not a valid name.");
    return;
  }

  if (paint_definitions_.Contains(name)) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "A class with name:'" + name + "' is already registered.");
    return;
  }

  v8::Isolate* isolate = ScriptController()->GetScriptState()->GetIsolate();
  v8::Local<v8::Context> context = ScriptController()->GetContext();
  DCHECK(ctor_value.V8Value()->IsFunction());
  v8::Local<v8::Function> constructor =
      v8::Local<v8::Function>::Cast(ctor_value.V8Value());

  Vector<CSSPropertyID> native_invalidation_properties;
  Vector<AtomicString> custom_invalidation_properties;

  if (!ParseInputProperties(isolate, context, constructor,
                            native_invalidation_properties,
                            custom_invalidation_properties, exception_state))
    return;

  // Get input argument types. Parse the argument type values only when
  // cssPaintAPIArguments is enabled.
  Vector<CSSSyntaxDescriptor> input_argument_types;
  if (!ParseInputArguments(isolate, context, constructor, input_argument_types,
                           exception_state))
    return;

  bool has_alpha = true;
  if (!ParsePaintRenderingContext2DSettings(isolate, context, constructor,
                                            has_alpha, exception_state))
    return;

  v8::Local<v8::Function> paint;
  if (!ParsePaintFunction(isolate, context, constructor, paint,
                          exception_state))
    return;

  PaintRenderingContext2DSettings context_settings;
  context_settings.setAlpha(has_alpha);
  CSSPaintDefinition* definition = CSSPaintDefinition::Create(
      ScriptController()->GetScriptState(), constructor, paint,
      native_invalidation_properties, custom_invalidation_properties,
      input_argument_types, context_settings);
  paint_definitions_.Set(name, definition);

  // TODO(xidachen): the following steps should be done with a postTask when
  // we move PaintWorklet off main thread.
  PaintWorklet* paint_worklet =
      PaintWorklet::From(*GetFrame()->GetDocument()->domWindow());
  PaintWorklet::DocumentDefinitionMap& document_definition_map =
      paint_worklet->GetDocumentDefinitionMap();
  if (document_definition_map.Contains(name)) {
    DocumentPaintDefinition* existing_document_definition =
        document_definition_map.at(name);
    if (existing_document_definition == kInvalidDocumentDefinition)
      return;
    if (!existing_document_definition->RegisterAdditionalPaintDefinition(
            *definition)) {
      document_definition_map.Set(name, kInvalidDocumentDefinition);
      exception_state.ThrowDOMException(
          kNotSupportedError,
          "A class with name:'" + name +
              "' was registered with a different definition.");
      return;
    }
    // Notify the generator ready only when register paint is called the second
    // time with the same |name| (i.e. there is already a document definition
    // associated with |name|
    if (existing_document_definition->GetRegisteredDefinitionCount() ==
        PaintWorklet::kNumGlobalScopes)
      pending_generator_registry_->NotifyGeneratorReady(name);
  } else {
    DocumentPaintDefinition* document_definition =
        new DocumentPaintDefinition(definition);
    document_definition_map.Set(name, document_definition);
  }
}

CSSPaintDefinition* PaintWorkletGlobalScope::FindDefinition(
    const String& name) {
  return paint_definitions_.at(name);
}

double PaintWorkletGlobalScope::devicePixelRatio() const {
  return GetFrame()->DevicePixelRatio();
}

DEFINE_TRACE(PaintWorkletGlobalScope) {
  visitor->Trace(paint_definitions_);
  visitor->Trace(pending_generator_registry_);
  MainThreadWorkletGlobalScope::Trace(visitor);
}

DEFINE_TRACE_WRAPPERS(PaintWorkletGlobalScope) {
  for (auto definition : paint_definitions_)
    visitor->TraceWrappers(definition.value);
  MainThreadWorkletGlobalScope::TraceWrappers(visitor);
}

}  // namespace blink
