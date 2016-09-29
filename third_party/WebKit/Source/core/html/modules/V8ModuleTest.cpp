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

    v8::Local<v8::String> v8_str(const char*);

protected:
    std::unique_ptr<v8::ArrayBuffer::Allocator> array_buffer_allocator_;
    v8::Isolate* isolate_;
};

struct Stuff
{
    Stuff(v8::Isolate* isolate)
        : handle_scope_(isolate)
        , context_(v8::Context::New(isolate))
        , scope_(context_)
    {
    }

    v8::HandleScope handle_scope_;
    v8::Local<v8::Context> context_;
    v8::Context::Scope scope_;
};

v8::Local<v8::String> V8ModuleTest::v8_str(const char* s)
{
    return v8::String::NewFromUtf8(isolate_, s, v8::NewStringType::kNormal)
        .ToLocalChecked();
}

TEST_F(V8ModuleTest, runScript)
{
    Stuff stuff(isolate_);
    v8::Local<v8::String> source = v8_str("'Hello' + ', World!'");
    v8::Local<v8::Script> script = v8::Script::Compile(stuff.context_, source)
        .ToLocalChecked();
    v8::Local<v8::Value> result = script->Run(stuff.context_).ToLocalChecked();
    v8::String::Utf8Value utf8(result);
    EXPECT_EQ("Hello, World!", std::string(*utf8))
        << "the script should have run";
}

// exports do not work yet (apparently?)
// TODO(dominicc): Do imports work?
TEST_F(V8ModuleTest, instantiateModule)
{
    Stuff stuff(isolate_);
    v8::ScriptCompiler::Source module_source(v8_str(
        "Object.greet = (name) => `Hello, ${name}!`;\n"
        "//export ... ;"));
    v8::Local<v8::Module> module =
        v8::ScriptCompiler::CompileModule(isolate_, &module_source)
        .ToLocalChecked();
    EXPECT_TRUE(module->Instantiate(stuff.context_, nullptr));
    //v8::Local<v8::Value> module_value =
        module->Evaluate(stuff.context_).ToLocalChecked();
    // v8::Local<v8::Function> f = module_value.As<v8::Object>()->Get(
    v8::Local<v8::Function> f =
        stuff.context_->Global()
        ->Get(stuff.context_, v8_str("Object"))
        .ToLocalChecked().As<v8::Object>()
        ->Get(stuff.context_, v8_str("greet"))
        .ToLocalChecked().As<v8::Function>();
    v8::Local<v8::Value> args[] = {v8_str("Mars")};
    v8::Local<v8::Value> result =
        f->Call(stuff.context_, stuff.context_->Global(), 1, args)
        .ToLocalChecked();
    v8::String::Utf8Value utf8(result);
    EXPECT_EQ("Hello, Mars!", std::string(*utf8))
        << "the module should have exported a function";
}

} // namespace blink
