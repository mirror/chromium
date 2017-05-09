// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/sequenced_worker_pool_owner.h"
#include "base/threading/thread.h"
#include "mojo/public/cpp/bindings/sync_handle_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

void StoreSyncHandleRegistry(scoped_refptr<SyncHandleRegistry>* out) {
  *out = SyncHandleRegistry::current();
}

scoped_refptr<SyncHandleRegistry> GetSyncHandleRegistryForTaskRunner(
    base::SequencedTaskRunner* task_runner) {
  base::RunLoop run_loop;
  scoped_refptr<SyncHandleRegistry> registry;
  task_runner->PostTaskAndReply(
      FROM_HERE, base::BindOnce(&StoreSyncHandleRegistry, &registry),
      run_loop.QuitClosure());
  run_loop.Run();
  return registry;
}

void ReleaseSyncHandleRegistry(scoped_refptr<SyncHandleRegistry> registry) {}

void ReleaseSyncHandleRegistryOnTaskRunner(
    base::SequencedTaskRunner* task_runner,
    scoped_refptr<SyncHandleRegistry> registry) {
  base::RunLoop run_loop;
  task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&ReleaseSyncHandleRegistry, base::Passed(&registry)),
      run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace

TEST(SequenceLocalSyncHandleRegistryTest, SequenceOnSingleThread) {
  base::test::ScopedTaskEnvironment task_environment;

  scoped_refptr<SyncHandleRegistry> main_thread_registry =
      SyncHandleRegistry::current();
  scoped_refptr<SyncHandleRegistry> sequence_registry, other_sequence_registry;
  EXPECT_EQ(main_thread_registry, SyncHandleRegistry::current());
  auto sequence_token = base::SequenceToken::Create();
  {
    base::ScopedSetSequenceTokenForCurrentThread scoped_set_sequence_token(
        sequence_token);
    sequence_registry = SyncHandleRegistry::current();
    EXPECT_NE(main_thread_registry, sequence_registry);
    EXPECT_EQ(sequence_registry, SyncHandleRegistry::current());
  }
  EXPECT_EQ(main_thread_registry, SyncHandleRegistry::current());

  auto other_sequence_token = base::SequenceToken::Create();
  {
    base::ScopedSetSequenceTokenForCurrentThread
        other_scoped_set_sequence_token(other_sequence_token);
    other_sequence_registry = SyncHandleRegistry::current();
    EXPECT_NE(main_thread_registry, other_sequence_registry);
    EXPECT_NE(sequence_registry, other_sequence_registry);
    EXPECT_EQ(other_sequence_registry, SyncHandleRegistry::current());
  }
  EXPECT_EQ(main_thread_registry, SyncHandleRegistry::current());
  {
    base::ScopedSetSequenceTokenForCurrentThread scoped_set_sequence_token(
        sequence_token);
    sequence_registry = nullptr;
  }
  {
    base::ScopedSetSequenceTokenForCurrentThread scoped_set_sequence_token(
        other_sequence_token);
    other_sequence_registry = nullptr;
  }
}

TEST(SequenceLocalSyncHandleRegistryTest, RealSequencesAndThreads) {
  base::test::ScopedTaskEnvironment task_environment;
  base::SequencedWorkerPoolOwner pool_owner(2u, "SyncHandleRegistryTest");
  base::Thread dedicated_thread("SyncHandleRegistryTest thread");
  dedicated_thread.Start();

  auto task_scheduler_sequence_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto task_scheduler_thread_runner =
      base::CreateSingleThreadTaskRunnerWithTraits(base::TaskTraits());
  auto worker_pool_runner = pool_owner.pool()->GetSequencedTaskRunner(
      base::SequencedWorkerPool::GetSequenceToken());
  auto dedicated_thread_runner = dedicated_thread.task_runner();

  scoped_refptr<SyncHandleRegistry> main_thread_registry =
      SyncHandleRegistry::current();
  EXPECT_EQ(main_thread_registry, SyncHandleRegistry::current());

  scoped_refptr<SyncHandleRegistry> task_scheduler_sequence_registry =
      GetSyncHandleRegistryForTaskRunner(task_scheduler_sequence_runner.get());
  EXPECT_EQ(
      task_scheduler_sequence_registry,
      GetSyncHandleRegistryForTaskRunner(task_scheduler_sequence_runner.get()));
  scoped_refptr<SyncHandleRegistry> task_scheduler_thread_registry =
      GetSyncHandleRegistryForTaskRunner(task_scheduler_thread_runner.get());
  EXPECT_EQ(
      task_scheduler_thread_registry,
      GetSyncHandleRegistryForTaskRunner(task_scheduler_thread_runner.get()));
  scoped_refptr<SyncHandleRegistry> worker_pool_registry =
      GetSyncHandleRegistryForTaskRunner(worker_pool_runner.get());
  EXPECT_EQ(worker_pool_registry,
            GetSyncHandleRegistryForTaskRunner(worker_pool_runner.get()));

  scoped_refptr<SyncHandleRegistry> dedicated_thread_registry =
      GetSyncHandleRegistryForTaskRunner(dedicated_thread_runner.get());
  EXPECT_EQ(dedicated_thread_registry,
            GetSyncHandleRegistryForTaskRunner(dedicated_thread_runner.get()));

  EXPECT_EQ(5u, (std::set<scoped_refptr<SyncHandleRegistry>>{
                     main_thread_registry, task_scheduler_sequence_registry,
                     task_scheduler_thread_registry, worker_pool_registry,
                     dedicated_thread_registry})
                    .size());
  ReleaseSyncHandleRegistryOnTaskRunner(
      task_scheduler_sequence_runner.get(),
      std::move(task_scheduler_sequence_registry));
  ReleaseSyncHandleRegistryOnTaskRunner(
      task_scheduler_thread_runner.get(),
      std::move(task_scheduler_thread_registry));
  ReleaseSyncHandleRegistryOnTaskRunner(worker_pool_runner.get(),
                                        std::move(worker_pool_registry));
  ReleaseSyncHandleRegistryOnTaskRunner(dedicated_thread_runner.get(),
                                        std::move(dedicated_thread_registry));
}

}  // namespace test
}  // namespace mojo
