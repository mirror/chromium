// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/libfuzzer/fuzzers/javascript_parser.pb.h"
#include "testing/libfuzzer/fuzzers/javascript_parser_proto_to_string.h"
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

// Silence logging from the protobuf library.
protobuf_mutator::protobuf::LogSilencer log_silencer;

bool initialized = false;
v8::Isolate* isolate = nullptr;

std::string protobuf_to_string(const javascript_parser_proto_fuzzer::Source& source_protobuf) {
  std::string source;
  for (const auto& token : source_protobuf.tokens()) {
    source += token_to_string(token);
  }
  return source;
}

DEFINE_BINARY_PROTO_FUZZER(
    const javascript_parser_proto_fuzzer::Source& source_protobuf) {
  fprintf(stderr, "BINARY proto fuzzer!!!!!\n");

  if (!initialized) {
    v8::V8::InitializeICUDefaultLocation(
        "out/libfuzzer/javascript_parser_proto_fuzzer");
    v8::V8::InitializeExternalStartupData(
        "out/libfuzzer/javascript_parser_proto_fuzzer");
    v8::Platform* platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();

    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = v8::Isolate::New(create_params);
    initialized = true;
  }

  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    std::string source_string = protobuf_to_string(source_protobuf);
    fprintf(stderr, "Case: %s\n", source_string.c_str());

    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, source_string.c_str(),
                                v8::NewStringType::kNormal)
            .ToLocalChecked();

    {
      v8::TryCatch try_catch(isolate);

      v8::MaybeLocal<v8::Script> script =
          v8::Script::Compile(context, source);
      v8::Local<v8::Script> successfully_compiled;
      if (script.ToLocal(&successfully_compiled)) {
        v8::MaybeLocal<v8::Value> result = successfully_compiled->Run(context);
        // FIXME: What's the Chromium macro for this?
        // USE(result);
        result.IsEmpty();
      }
    }
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
