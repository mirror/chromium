// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACE_EVENT_MEMORY_DUMP_MANAGER_TEST_UTILS_H_
#define BASE_TRACE_EVENT_MEMORY_DUMP_MANAGER_TEST_UTILS_H_

#include "base/bind.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace base {
namespace trace_event {

// GTest matchers for MemoryDumpRequestArgs arguments.
MATCHER(IsDetailedDump, "") {
  return arg.level_of_detail == MemoryDumpLevelOfDetail::DETAILED;
}

MATCHER(IsLightDump, "") {
  return arg.level_of_detail == MemoryDumpLevelOfDetail::LIGHT;
}

MATCHER(IsBackgroundDump, "") {
  return arg.level_of_detail == MemoryDumpLevelOfDetail::BACKGROUND;
}

class MockMemoryDumpProvider : public MemoryDumpProvider {
 public:
  MOCK_METHOD0(Destructor, void());
  MOCK_METHOD2(OnMemoryDump,
               bool(const MemoryDumpArgs& args, ProcessMemoryDump* pmd));
  MOCK_METHOD1(PollFastMemoryTotal, void(uint64_t* memory_total));
  MOCK_METHOD0(SuspendFastMemoryPolling, void());

  MockMemoryDumpProvider();

  ~MockMemoryDumpProvider() override;

  bool enable_mock_destructor;
};

MockMemoryDumpProvider::MockMemoryDumpProvider()
    : enable_mock_destructor(false) {
  ON_CALL(*this, OnMemoryDump(testing::_, testing::_))
      .WillByDefault(
          testing::Invoke([](const MemoryDumpArgs& args,
                             ProcessMemoryDump* pmd) -> bool { return true; }));
  ON_CALL(*this, PollFastMemoryTotal(testing::_))
      .WillByDefault(testing::Invoke(
          [](uint64_t* memory_total) -> void { NOTREACHED(); }));
}

MockMemoryDumpProvider::~MockMemoryDumpProvider() {
  if (enable_mock_destructor)
    Destructor();
}

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

}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_MEMORY_DUMP_MANAGER_TEST_UTILS_H_
