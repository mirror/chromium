// Copyright 2007-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <signal.h>
#include <map>
#include <string>

#include "sys/stat.h"
#include "v8.h"

#include "debug.h"
#include "ic-inl.h"
#include "runtime.h"
#include "serialize.h"
#include "scopeinfo.h"
#include "snapshot.h"

namespace v8 { namespace internal {

#ifdef WIN32
DEFINE_string(serialization_file, "C:\\Windows\\Temp\\serdes",
              "file in which to serialize heap");
#else
DEFINE_string(serialization_file, "/tmp/serdes",
              "file in which to serialize heap");
#endif
DECLARE_bool(debug_serialization);
}}  // namespace v8::internal

using namespace v8::internal;


static int local_counters[256];
static int counter_count = 0;
static std::map<std::wstring, int> counter_table;


// Callback receiver to track counters in test.
static int* counter_function(const wchar_t* name) {
  std::wstring counter(name);
  if (counter_table.find(counter) == counter_table.end()) {
    local_counters[counter_count] = 0;
    counter_table[counter] = counter_count++;
  }

  return &local_counters[counter_table[counter]];
}


template <class T>
static Address AddressOf(T id) {
  return ExternalReference(id).address();
}


template <class T>
static uint32_t Encode(const ExternalReferenceEncoder& encoder, T id) {
  return encoder.Encode(AddressOf(id));
}


static int make_code(TypeCode type, int id) {
  return static_cast<uint32_t>(type) << kReferenceTypeShift | id;
}


static int register_code(int reg) {
  return Debug::k_register_address << kDebugIdShift | reg;
}


static void TestExternalReferenceEncoder() {
  StatsTable::SetCounterFunction(counter_function);
  Heap::Setup(false);
  ExternalReferenceEncoder encoder;
  CHECK_EQ(make_code(BUILTIN, Builtins::ArrayCode),
           Encode(encoder, Builtins::ArrayCode));
  CHECK_EQ(make_code(RUNTIME_FUNCTION, Runtime::kAbort),
           Encode(encoder, Runtime::kAbort));
  CHECK_EQ(make_code(IC_UTILITY, IC::kLoadCallbackProperty),
           Encode(encoder, IC_Utility(IC::kLoadCallbackProperty)));
  CHECK_EQ(make_code(DEBUG_ADDRESS, register_code(3)),
           Encode(encoder, Debug_Address(Debug::k_register_address, 3)));
  ExternalReference keyed_load_function_prototype =
      ExternalReference(&Counters::keyed_load_function_prototype);
  CHECK_EQ(make_code(STATS_COUNTER, Counters::k_keyed_load_function_prototype),
           encoder.Encode(keyed_load_function_prototype.address()));
  ExternalReference passed_function =
      ExternalReference::builtin_passed_function();
  CHECK_EQ(make_code(UNCLASSIFIED, 1),
           encoder.Encode(passed_function.address()));
  ExternalReference the_hole_value_location =
      ExternalReference::the_hole_value_location();
  CHECK_EQ(make_code(UNCLASSIFIED, 2),
           encoder.Encode(the_hole_value_location.address()));
  ExternalReference stack_guard_limit_address =
      ExternalReference::address_of_stack_guard_limit();
  CHECK_EQ(make_code(UNCLASSIFIED, 3),
           encoder.Encode(stack_guard_limit_address.address()));
  CHECK_EQ(make_code(UNCLASSIFIED, 4),
           encoder.Encode(ExternalReference::debug_break().address()));
  CHECK_EQ(make_code(UNCLASSIFIED, 5),
           encoder.Encode(ExternalReference::new_space_start().address()));
}


static void TestExternalReferenceDecoder() {
  StatsTable::SetCounterFunction(counter_function);
  Heap::Setup(false);
  ExternalReferenceDecoder decoder;
  CHECK_EQ(AddressOf(Builtins::ArrayCode),
           decoder.Decode(make_code(BUILTIN, Builtins::ArrayCode)));
  CHECK_EQ(AddressOf(Runtime::kAbort),
           decoder.Decode(make_code(RUNTIME_FUNCTION, Runtime::kAbort)));
  CHECK_EQ(AddressOf(IC_Utility(IC::kLoadCallbackProperty)),
           decoder.Decode(make_code(IC_UTILITY, IC::kLoadCallbackProperty)));
  CHECK_EQ(AddressOf(Debug_Address(Debug::k_register_address, 3)),
           decoder.Decode(make_code(DEBUG_ADDRESS, register_code(3))));
  ExternalReference keyed_load_function =
      ExternalReference(&Counters::keyed_load_function_prototype);
  CHECK_EQ(keyed_load_function.address(),
           decoder.Decode(
               make_code(STATS_COUNTER,
                         Counters::k_keyed_load_function_prototype)));
  CHECK_EQ(ExternalReference::builtin_passed_function().address(),
           decoder.Decode(make_code(UNCLASSIFIED, 1)));
  CHECK_EQ(ExternalReference::the_hole_value_location().address(),
           decoder.Decode(make_code(UNCLASSIFIED, 2)));
  CHECK_EQ(ExternalReference::address_of_stack_guard_limit().address(),
           decoder.Decode(make_code(UNCLASSIFIED, 3)));
  CHECK_EQ(ExternalReference::debug_break().address(),
           decoder.Decode(make_code(UNCLASSIFIED, 4)));
  CHECK_EQ(ExternalReference::new_space_start().address(),
           decoder.Decode(make_code(UNCLASSIFIED, 5)));
}


static void Serialize() {
#ifdef DEBUG
  FLAG_debug_serialization = true;
#endif
  StatsTable::SetCounterFunction(counter_function);

  v8::HandleScope scope;
  const int kExtensionCount = 5;
  const char* extension_list[kExtensionCount] =
    { "v8/print", "v8/load", "v8/quit", "v8/version", "v8/gc" };
  v8::ExtensionConfiguration extensions(kExtensionCount, extension_list);
  v8::Persistent<v8::Context> env = v8::Context::New(&extensions);
  env->Enter();

  Snapshot::WriteToFile(FLAG_serialization_file);
}


// Test that the whole heap can be serialized when running from the
// internal snapshot.
// (Smoke test.)
static void TestSerializeInternal() {
  Snapshot::Initialize(NULL);
  Serialize();
}


// Test that the whole heap can be serialized when running from a
// bootstrapped heap.
// (Smoke test.)
static void TestSerialize() {
  Snapshot::DisableInternal();
  Serialize();
}


//----------------------------------------------------------------------------
// Tests that the heap can be deserialized.

static void Deserialize() {
#ifdef DEBUG
  FLAG_debug_serialization = true;
#endif
  CHECK(Snapshot::Initialize(FLAG_serialization_file));
}


static void SanityCheck() {
  v8::HandleScope scope;
#ifdef DEBUG
  Heap::Verify();
#endif
  CHECK(Top::global()->IsJSObject());
  CHECK(Top::global_context()->IsContext());
  CHECK(Top::special_function_table()->IsFixedArray());
  CHECK(Heap::symbol_table()->IsSymbolTable());
  CHECK(!Factory::LookupAsciiSymbol("Empty")->IsFailure());
}


static void TestDeserialize() {
  v8::HandleScope scope;

  Deserialize();

  SanityCheck();
}

static void TestDeserializeAndRunScript() {
  v8::HandleScope scope;

  Deserialize();

  char* c_source = "\"1234\".length";
  v8::Local<v8::String> source = v8::String::New(c_source);
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  CHECK_EQ(4, script->Run()->Int32Value());
}


static void TestDeserializeNatives() {
  v8::HandleScope scope;

  Deserialize();

  char* c_source = "\"abcd\".charAt(2) == 'c'";
  v8::Local<v8::String> source = v8::String::New(c_source);
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  v8::Local<v8::Value> value = script->Run();
  CHECK(value->BooleanValue());
}


static void TestDeserializeExtensions() {
  v8::HandleScope scope;

  Deserialize();
  char* c_source = "print(\"abcd\");";
  v8::Local<v8::String> source = v8::String::New(c_source);
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  v8::Local<v8::Value> value = script->Run();
  CHECK(value->IsUndefined());
}
