// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/client_process_impl.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/coordinator.h"
#include "services/resource_coordinator/public/interfaces/memory_instrumentation/memory_instrumentation.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::trace_event::MemoryDumpLevelOfDetail;
using base::trace_event::MemoryDumpManager;
using base::trace_event::MemoryDumpType;

namespace memory_instrumentation {

class MockCoordinator : public Coordinator, public mojom::Coordinator {
 public:
  void BindCoordinatorRequest(
      const service_manager::BindSourceInfo& source_info,
      mojom::CoordinatorRequest request) override {
    bindings_.AddBinding(this, std::move(request));
  }

  void RegisterClientProcess(mojom::ClientProcessPtr) override {}

  void RequestGlobalMemoryDump(
      const base::trace_event::MemoryDumpRequestArgs& args,
      const RequestGlobalMemoryDumpCallback& callback) override {
    callback.Run(args.dump_guid, true, mojom::GlobalMemoryDumpPtr());
  }

 private:
  mojo::BindingSet<mojom::Coordinator> bindings_;
};

class ClientProcessImplTest : public testing::Test {
 public:
  void SetUp() override {
    message_loop_ = base::MakeUnique<base::MessageLoop>();
    coordinator_ = base::MakeUnique<MockCoordinator>();
    mdm_ = MemoryDumpManager::CreateInstanceForTesting();
    auto process_type = mojom::ProcessType::OTHER;
    ClientProcessImpl::Config config(coordinator_.get(), process_type);
    client_process_.reset(new ClientProcessImpl(config));
    client_process_->SetAsNonCoordinatorForTesting();

    // Reset the counters.
    expected_callbacks_left_ = 0;
    dump_requests_received_by_coordinator_ = 0;
    quit_closure_.Reset();
  }

  void TearDown() override {
    mdm_.reset();
    client_process_.reset();
    coordinator_.reset();
    message_loop_.reset();
  }

  void OnGlobalMemoryDumpDone(int num_requests_left,
                              uint64_t dump_guid,
                              bool success,
                              mojom::GlobalMemoryDumpPtr result) {
    EXPECT_GT(expected_callbacks_left_, 0);
    EXPECT_FALSE(quit_closure_.is_null());

    dump_requests_received_by_coordinator_ += success ? 1 : 0;
    expected_callbacks_left_--;
    if (expected_callbacks_left_ == 0)
      quit_closure_.Run();

    if (num_requests_left > 0)
      SequentiallyRequestGlobalDumps(num_requests_left);
  }

  void SequentiallyRequestGlobalDumps(int num_requests) {
    base::trace_event::MemoryDumpRequestArgs args{
        num_requests /* dump_guid */, MemoryDumpType::SUMMARY_ONLY,
        MemoryDumpLevelOfDetail::BACKGROUND};
    client_process_->RequestGlobalMemoryDump(
        args, base::Bind(&ClientProcessImplTest::OnGlobalMemoryDumpDone,
                         base::Unretained(this), num_requests - 1));
  }

  int expected_callbacks_left_;
  int dump_requests_received_by_coordinator_;
  base::Closure quit_closure_;

 private:
  std::unique_ptr<base::MessageLoop> message_loop_;
  std::unique_ptr<MockCoordinator> coordinator_;
  std::unique_ptr<MemoryDumpManager> mdm_;
  std::unique_ptr<ClientProcessImpl> client_process_;
};

// Makes several global dump requests each after receiving the ACK for the
// previous one. There should be no throttling and all requests should be
// forwarded to the coordinator.
TEST_F(ClientProcessImplTest, NonOverlappingMemoryDumpRequests) {
  const int kNumRequests = 3;
  expected_callbacks_left_ = kNumRequests;
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();
  SequentiallyRequestGlobalDumps(kNumRequests);
  run_loop.Run();
  EXPECT_EQ(kNumRequests, dump_requests_received_by_coordinator_);
}

// Makes several global dump requests without waiting for previous requests to
// finish. They should be queued by the coordinator and served in sequence if
// the type != MemoryDumpType::PERIODIC_INTERVAL.
TEST_F(ClientProcessImplTest, OverlappingMemoryDumpRequests) {
  const int kNumRequests = 3;
  expected_callbacks_left_ = kNumRequests;
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();
  for (int i = 0; i < kNumRequests; i++)
    SequentiallyRequestGlobalDumps(1);
  run_loop.Run();
  EXPECT_EQ(kNumRequests, dump_requests_received_by_coordinator_);
}

}  // namespace memory_instrumentation
