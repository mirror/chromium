// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/libfuzzer/fuzzers/javascript_parser.pb.h"
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"
// #include "v8/test/fuzzer/fuzzer-support.h"

// Silence logging from the protobuf library.
protobuf_mutator::protobuf::LogSilencer log_silencer;

bool initialized = false;
v8::Isolate* isolate = nullptr;

DEFINE_BINARY_PROTO_FUZZER(
    const javascript_parser_proto_fuzzer::Source& source_protobuf) {
  fprintf(stderr, "BINARY proto fuzzer!!!!!\n");

  // protobuf_to_source(source_protobuf);

  // Initialize V8.
  if (!initialized) {
    v8::V8::InitializeICUDefaultLocation(
        "out/libfuzzer/javascript_parser_proto_fuzzer");
    v8::V8::InitializeExternalStartupData(
        "out/libfuzzer/javascript_parser_proto_fuzzer");
    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = v8::Isolate::New(create_params);
    initialized = true;
  }
  {
    v8::Isolate::Scope isolate_scope(isolate);
    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);
    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);
    // Create a string containing the JavaScript source code.
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, "'Hello' + ', World!'",
                                v8::NewStringType::kNormal)
            .ToLocalChecked();
    // Compile the source code.
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();
    // Run the script to get the result.
    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
    // Convert the result to an UTF8 string and print it.
    v8::String::Utf8Value utf8(result);
    printf("%s\n", *utf8);
  }

  // FIXME: need to do GC here?

  // Dispose the isolate and tear down V8.
  // isolate->Dispose();
  // V8::Dispose();
  // V8::ShutdownPlatform();
  // delete platform;
  // delete create_params.array_buffer_allocator;
  // return 0;
}
