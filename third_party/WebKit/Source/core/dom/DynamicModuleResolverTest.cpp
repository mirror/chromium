// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/DynamicModuleResolver.h"

#include "bindings/core/v8/ScriptFunction.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/core/v8/ScriptValue.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8BindingForTesting.h"
#include "core/dom/ModuleScript.h"
#include "core/loader/modulescript/ModuleScriptFetchRequest.h"
#include "core/testing/DummyModulator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/v8.h"

namespace blink {

namespace {}  // namespace

class DynamicModuleResolverTestModulator final : public DummyModulator {
 public:
  DynamicModuleResolverTestModulator(ScriptState* script_state)
      : script_state_(script_state) {}
  virtual ~DynamicModuleResolverTestModulator() {}

  void ResolveTreeFetch(ModuleScript* module_script) {
    ASSERT_TRUE(pending_client_);
    pending_client_->NotifyModuleTreeLoadFinished(module_script);
    pending_client_ = nullptr;
  }

  DECLARE_TRACE();

 private:
  // Implements Modulator:
  ScriptState* GetScriptState() override { return script_state_.Get(); }

  ModuleScript* GetFetchedModuleScript(const KURL& url) override {
    EXPECT_EQ(url.GetString(), "https://example.com/referrer.js");
    ModuleScript* module_script = ModuleScript::CreateForTest(
        this, ScriptModule(), url, "nonce", kParserInserted,
        WebURLRequest::kFetchCredentialsModeOmit);
    return module_script;
  }

  void FetchTree(const ModuleScriptFetchRequest& request,
                 ModuleTreeClient* client) override {
    EXPECT_EQ(request.Url().GetString(), "https://example.com/dependency.js");

    pending_client_ = client;
  }

  void ExecuteModule(const ModuleScript* module_script) override {
    ScriptState::Scope scope(script_state_.Get());
    module_script->Record().Evaluate(script_state_.Get());
  }

  ScriptModuleState GetRecordStatus(ScriptModule script_module) override {
    ScriptState::Scope scope(script_state_.Get());
    return script_module.Status(script_state_.Get());
  }

  RefPtr<ScriptState> script_state_;
  Member<ModuleTreeClient> pending_client_;
};

DEFINE_TRACE(DynamicModuleResolverTestModulator) {
  visitor->Trace(pending_client_);
  DummyModulator::Trace(visitor);
}

class CaptureExportedStringFunction : public ScriptFunction {
 public:
  CaptureExportedStringFunction(ScriptState* script_state,
                                const String& export_name)
      : ScriptFunction(script_state), export_name_(export_name) {}

  v8::Local<v8::Function> Bind() { return BindToV8Function(); }
  bool WasCalled() { return was_called_; }
  String GetCapturedValue() { return captured_value_; }

 private:
  ScriptValue Call(ScriptValue value) override {
    was_called_ = true;

    v8::Isolate* isolate = GetScriptState()->GetIsolate();
    v8::Local<v8::Context> context = GetScriptState()->GetContext();

    v8::Local<v8::Object> module_namespace =
        value.V8Value()->ToObject(context).ToLocalChecked();
    v8::Local<v8::Value> exported_value =
        module_namespace->Get(context, V8String(isolate, export_name_))
            .ToLocalChecked();
    captured_value_ = ToCoreString(exported_value->ToString());

    return ScriptValue();
  }

  const String export_name_;
  bool was_called_ = false;
  String captured_value_;
};

class NotReached : public ScriptFunction {
 public:
  static v8::Local<v8::Function> CreateFunction(ScriptState* script_state) {
    NotReached* not_reached = new NotReached(script_state);
    return not_reached->BindToV8Function();
  }

 private:
  explicit NotReached(ScriptState* script_state)
      : ScriptFunction(script_state) {}

  ScriptValue Call(ScriptValue) override {
    ADD_FAILURE();
    return ScriptValue();
  }
};

TEST(DynamicModuleResolverTest, ResolveSuccess) {
  V8TestingScope scope;
  DynamicModuleResolverTestModulator* modulator =
      new DynamicModuleResolverTestModulator(scope.GetScriptState());

  ScriptPromiseResolver* promise_resolver =
      ScriptPromiseResolver::Create(scope.GetScriptState());
  ScriptPromise promise = promise_resolver->Promise();

  auto capture =
      new CaptureExportedStringFunction(scope.GetScriptState(), "foo");
  promise.Then(capture->Bind(),
               NotReached::CreateFunction(scope.GetScriptState()));

  DynamicModuleResolver* resolver = DynamicModuleResolver::Create(modulator);
  resolver->ResolveDynamically(
      "./dependency.js", "https://example.com/referrer.js", promise_resolver);

  EXPECT_FALSE(capture->WasCalled());

  KURL url(kParsedURLString, "http://example.com/dependency.js");
  ScriptModule record = ScriptModule::Compile(
      scope.GetIsolate(), "export const foo = 'hello';", url.GetString(),
      kSharableCrossOrigin, TextPosition::MinimumPosition(),
      ASSERT_NO_EXCEPTION);
  ModuleScript* module_script = ModuleScript::CreateForTest(
      modulator, record, url, "nonce", kNotParserInserted,
      WebURLRequest::kFetchCredentialsModeOmit);
  record.Instantiate(scope.GetScriptState());
  EXPECT_FALSE(module_script->IsErrored());
  modulator->ResolveTreeFetch(module_script);

  v8::MicrotasksScope::PerformCheckpoint(scope.GetIsolate());
  EXPECT_TRUE(capture->WasCalled());
  EXPECT_EQ("hello", capture->GetCapturedValue());
}

}  // namespace blink
