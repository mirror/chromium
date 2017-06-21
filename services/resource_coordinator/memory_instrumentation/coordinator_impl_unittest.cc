// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/coordinator_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::ByRef;
using ::testing::Invoke;

using RequestGlobalMemoryDumpCallback =
    memory_instrumentation::CoordinatorImpl::RequestGlobalMemoryDumpCallback;
using memory_instrumentation::mojom::GlobalMemoryDumpPtr;
using memory_instrumentation::mojom::GlobalMemoryDump;
using base::trace_event::MemoryDumpRequestArgs;

namespace memory_instrumentation {

class FakeCoordinatorImpl : public CoordinatorImpl {
 public:
  FakeCoordinatorImpl() : CoordinatorImpl(false, nullptr) {}
  ~FakeCoordinatorImpl() override {}
  service_manager::Identity GetClientIdentityForCurrentRequest()
      const override {
    return service_manager::Identity();
  }
};

class CoordinatorImplTest : public testing::Test {
 public:
  CoordinatorImplTest() {}
  void SetUp() override {
    coordinator_.reset(new FakeCoordinatorImpl);
  }

  void TearDown() override { coordinator_.reset(); }

  void RegisterClientProcess(mojom::ClientProcessPtr client_process) {
    coordinator_->RegisterClientProcess(std::move(client_process));
  }

  void RequestGlobalMemoryDump(MemoryDumpRequestArgs args,
                               RequestGlobalMemoryDumpCallback callback) {
    coordinator_->RequestGlobalMemoryDump(args, callback);
  }

 private:
  std::unique_ptr<CoordinatorImpl> coordinator_;
  base::MessageLoop message_loop_;
};

class MockClientProcess : public mojom::ClientProcess {
 public:
  MockClientProcess(CoordinatorImplTest* test_coordinator) : binding_(this) {
    // Register to the coordinator.
    mojom::ClientProcessPtr client_process;
    binding_.Bind(mojo::MakeRequest(&client_process));
    test_coordinator->RegisterClientProcess(std::move(client_process));

    ON_CALL(*this, RequestProcessMemoryDump(_, _))
        .WillByDefault(
            Invoke([](const MemoryDumpRequestArgs& args,
                      const RequestProcessMemoryDumpCallback& callback) {
              callback.Run(args.dump_guid, true,
                           mojom::RawProcessMemoryDumpPtr());

            }));
  }

  ~MockClientProcess() override {}

  MOCK_METHOD2(RequestProcessMemoryDump,
               void(const MemoryDumpRequestArgs& args,
                    const RequestProcessMemoryDumpCallback& callback));

 private:
  mojo::Binding<mojom::ClientProcess> binding_;
};

class MockGlobalMemoryDumpCallback {
 public:
  MockGlobalMemoryDumpCallback() = default;
  MOCK_METHOD3(DoRun, void(uint64_t, bool, GlobalMemoryDump*));

  void Run(uint64_t dump_guid, bool success, GlobalMemoryDumpPtr ptr) {
    DoRun(dump_guid, success, ptr.get());
  }

  RequestGlobalMemoryDumpCallback Get() {
    return base::Bind(&MockGlobalMemoryDumpCallback::Run,
                      base::Unretained(this));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockGlobalMemoryDumpCallback);
};

// Tests that the global dump is acked even in absence of clients.
TEST_F(CoordinatorImplTest, NoClients) {
  base::trace_event::MemoryDumpRequestArgs args = {
      1234, base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED,
      base::trace_event::MemoryDumpLevelOfDetail::DETAILED};

  MockGlobalMemoryDumpCallback callback;
  EXPECT_CALL(callback, DoRun(1234U, true, testing::NotNull()));
  RequestGlobalMemoryDump(args, callback.Get());
}

// Nominal behavior: several clients contributing to the global dump.
TEST_F(CoordinatorImplTest, SeveralClients) {
  MockClientProcess client_process_1(this);
  MockClientProcess client_process_2(this);

  EXPECT_CALL(client_process_1, RequestProcessMemoryDump(_, _)).Times(1);
  EXPECT_CALL(client_process_2, RequestProcessMemoryDump(_, _)).Times(1);

  base::trace_event::MemoryDumpRequestArgs args = {
      2345, base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED,
      base::trace_event::MemoryDumpLevelOfDetail::DETAILED};

  MockGlobalMemoryDumpCallback callback;
  EXPECT_CALL(callback, DoRun(2345U, true, testing::NotNull()));
  RequestGlobalMemoryDump(args, callback.Get());
  base::RunLoop().RunUntilIdle();
}

// Tests that a global dump is completed even if a client disconnects (e.g. due
// to a crash) while a global dump is happening.
TEST_F(CoordinatorImplTest, ClientCrashDuringGlobalDump) {
  base::trace_event::MemoryDumpRequestArgs args = {
      3456, base::trace_event::MemoryDumpType::EXPLICITLY_TRIGGERED,
      base::trace_event::MemoryDumpLevelOfDetail::DETAILED};

  MockClientProcess client_process_1(this);
  auto client_process_2 = base::MakeUnique<MockClientProcess>(this);

  // One of the client processes dies after a global dump is requested and
  // before it receives the corresponding process dump request. The coordinator
  // should detect that one of its clients is disconnected and claim the global
  // dump attempt has failed.
  EXPECT_CALL(client_process_1, RequestProcessMemoryDump(_, _))
      .WillOnce(
          Invoke([&client_process_2](
                     const MemoryDumpRequestArgs& args,
                     const MockClientProcess::RequestProcessMemoryDumpCallback&
                         callback) {
            client_process_2.reset();
            callback.Run(args.dump_guid, true,
                         mojom::RawProcessMemoryDumpPtr());
          }));

  MockGlobalMemoryDumpCallback callback;
  EXPECT_CALL(callback, DoRun(3456U, false, testing::NotNull()));
  RequestGlobalMemoryDump(args, callback.Get());
  base::RunLoop().RunUntilIdle();
}

}  // namespace memory_instrumentation
