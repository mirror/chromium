// Copyright 2007-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

using namespace v8;


class LocalContext {
 public:
  inline LocalContext(v8::ExtensionConfiguration* extensions = 0,
      v8::Handle<ObjectTemplate> global_template = v8::Handle<ObjectTemplate>(),
      v8::Handle<Value> global_object = v8::Handle<Value>())
    : context_(Context::New(extensions, global_template, global_object)) {
    context_->Enter();
  }
  inline ~LocalContext() {
    context_->Exit();
    context_.Dispose();
  }
  inline Context* operator->() { return *context_; }
  inline Context* operator*() { return *context_; }
  inline bool IsReady() { return !context_.IsEmpty(); }
 private:
  Persistent<Context> context_;
};


static void ContextCreation() {
  v8::HandleScope scope;
  LocalContext env;
}


static void TestContextCreation() {
  int loop = 2;
  double start_time = v8::internal::OS::TimeCurrentMillis();
  for (int i = 0; i < loop; i++) {
    ContextCreation();
  }
  double end_time = v8::internal::OS::TimeCurrentMillis();
  v8::internal::OS::Print("total time %f ms\n", (end_time - start_time));
}


static void TestV8Initialize() {
  v8::internal::V8::Initialize();
  v8::internal::V8::TearDown();
  v8::internal::V8::Initialize();
  v8::internal::V8::TearDown();
}


static void TestHeapSetup() {
  v8::internal::Heap::Setup();
  v8::internal::Heap::TearDown();
  v8::internal::Heap::Setup();
  v8::internal::Heap::TearDown();
}
