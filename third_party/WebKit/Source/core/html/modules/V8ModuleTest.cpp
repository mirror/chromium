// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/v8.h"
#include <memory>

namespace blink {

class V8ModuleTest : public ::testing::Test {
public:
    V8ModuleTest()
        : isolate_(nullptr) { }

    void SetUp()
    {
        // TODO(dominicc): I assume other tests may use V8, so not sure about
        // setting up and destroying V8 in this test.
        // TODO(dominicc): This test needs --harmony; is it set?
        //const char v8_flags[] = "--expose_gc --harmony";
        //v8::V8::SetFlagsFromString(v8_flags, sizeof(v8_flags)/sizeof(char));
        //v8::V8::Initialize();
        v8::Isolate::CreateParams create_params;
        array_buffer_allocator_.reset(
            v8::ArrayBuffer::Allocator::NewDefaultAllocator());
        create_params.array_buffer_allocator = array_buffer_allocator_.get();
        isolate_ = v8::Isolate::New(create_params);
    }

    void TearDown()
    {
        isolate_->Dispose();
        isolate_ = nullptr;
        array_buffer_allocator_.reset(nullptr);
    }

protected:
    std::unique_ptr<v8::ArrayBuffer::Allocator> array_buffer_allocator_;
    v8::Isolate* isolate_;
};

TEST_F(V8ModuleTest, testsAreRunning)
{
    v8::HandleScope handle_scope(isolate_);
    v8::Local<v8::Context> context = v8::Context::New(isolate_);
    v8::Context::Scope context_scope(context);
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(
            isolate_, "'Hello' + ', World!'", v8::NewStringType::kNormal)
        .ToLocalChecked();
    v8::Local<v8::Script> script = v8::Script::Compile(context, source)
        .ToLocalChecked();
    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
    v8::String::Utf8Value utf8(result);
    EXPECT_EQ("Hello, World!", std::string(*utf8))
        << "the script should have run";
}

} // namespace blink
