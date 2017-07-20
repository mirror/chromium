// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACE_EVENT_MEMORY_DUMP_MANAGER_TEST_UTILS_H_
#define BASE_TRACE_EVENT_MEMORY_DUMP_MANAGER_TEST_UTILS_H_

#include "base/bind.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "base/trace_event/trace_event_argument.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace trace_event {

void RequestGlobalDumpForInProcessTesting(const MemoryDumpRequestArgs& args) {
  MemoryDumpManager::GetInstance()->CreateProcessDump(
      args, ProcessMemoryDumpCallback());
};

// Short circuits the RequestGlobalDumpFunction() to CreateProcessDump(),
// effectively allowing to use both in unittests with the same behavior.
// Unittests are in-process only and don't require all the multi-process
// dump handshaking (which would require bits outside of base).
void InitializeMemoryDumpManagerForInProcessTesting(bool is_coordinator) {
  MemoryDumpManager* instance = MemoryDumpManager::GetInstance();
  instance->set_dumper_registrations_ignored_for_testing(true);
  instance->Initialize(BindRepeating(&RequestGlobalDumpForInProcessTesting),
                       is_coordinator);
}

// Check if the MemoryAllocatorDump contains an attribute with the expected
// name, type and units. The value of attribute is returned.
std::unique_ptr<Value> CheckExpectedAttributeInDump(
    const MemoryAllocatorDump* dump,
    const std::string& name,
    const char* expected_type,
    const char* expected_units) {
  std::unique_ptr<Value> raw_attrs =
      dump->attributes_for_testing()->ToBaseValue();
  DictionaryValue* args = nullptr;
  DictionaryValue* arg = nullptr;
  std::string arg_value;
  const Value* out_value = nullptr;
  EXPECT_TRUE(raw_attrs->GetAsDictionary(&args));
  EXPECT_TRUE(args->GetDictionary(name, &arg));
  EXPECT_TRUE(arg->GetString("type", &arg_value));
  EXPECT_EQ(expected_type, arg_value);
  EXPECT_TRUE(arg->GetString("units", &arg_value));
  EXPECT_EQ(expected_units, arg_value);
  EXPECT_TRUE(arg->Get("value", &out_value));
  return out_value ? out_value->CreateDeepCopy() : std::unique_ptr<Value>();
}

}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_MEMORY_DUMP_MANAGER_TEST_UTILS_H_
