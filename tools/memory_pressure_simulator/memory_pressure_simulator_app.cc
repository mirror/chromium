// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Simple test application to put the system in a state of high memory pressure
// and CPU usage.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"

namespace memory_pressure_simulator {

namespace {

// The default amount of memory that should be kept 'free'.
const size_t kDefaultFreeMemoryMb = 2048;

// The default size of the blocks to allocate.
const size_t kBlockSizeInBytes = 1024 * 1024;

// The size of a memory page.
const size_t kPageSize = 4 * 1024;

// Do some dummy work on a set of memory blocks, every array in |arrays|
// has a length of |kBlockSizeInBytes| bytes.
void DummyWork(uint32_t** arrays, size_t arrays_count) {
  while (true) {
    // TODO(sebmarchand): Should the order be randomized? if |array_count| is
    // high then this loop might take a while to execute and some memory blocks
    // might be evicted from the resident memory while this is running.
    for (size_t i = 0; i < arrays_count; ++i) {
      // Touch the first byte of every memory page so they stay in the resident
      // memory.
      for (size_t j = 0; j < kBlockSizeInBytes; j += kPageSize)
        base::RandBytes(reinterpret_cast<uint8_t*>(arrays[i]) + j, 1);
    }
  }
}

}  // namespace

class MemoryPressureSimulatorApp {
 public:
  MemoryPressureSimulatorApp() : free_memory_mb_(kDefaultFreeMemoryMb) {}

  ~MemoryPressureSimulatorApp() {
    // Don't bother freeing the memory here, as this destructor will never be
    // called anyway (this app has to be Ctrl-C killed).
  }

  int Run(int argc, char* argv[]) {
    // TODO(sebmarchand): Add a command line argument to specify the value of
    // |free_memory_mb_|.
    const int64_t kBlocksToAllocate =
        (base::SysInfo::AmountOfPhysicalMemory() / kBlockSizeInBytes) -
        (free_memory_mb_ * (1024 * 1024) / kBlockSizeInBytes);

    buffers_.reserve(kBlocksToAllocate);
    // Allocate the memory blocks.
    // TODO(sebmarchand): Make this work on !Windows platforms.
    for (int64_t i = 0; i < kBlocksToAllocate; ++i) {
      uint32_t* array = reinterpret_cast<uint32_t*>(
          ::VirtualAlloc(nullptr, kBlockSizeInBytes, MEM_COMMIT | MEM_RESERVE,
                         PAGE_READWRITE));
      buffers_.push_back(array);
    }

    // Use all the CPUs!
    const int kProcessorToUse =
        std::max(1, base::SysInfo::NumberOfProcessors());

    // Start |kProcessorToUse| high priority threads and make them each
    // continuously touch a set of memory blocks.
    const size_t kChunksSize = kBlocksToAllocate / kProcessorToUse;
    for (int i = 0; i < kProcessorToUse; ++i) {
      scoped_refptr<base::SequencedTaskRunner> task_runner =
          base::CreateSequencedTaskRunnerWithTraits(
              {base::TaskShutdownBehavior::BLOCK_SHUTDOWN,
               base::TaskPriority::USER_BLOCKING});
      size_t chunk_size = kChunksSize;
      if (i == (kProcessorToUse - 1))
        chunk_size = kBlocksToAllocate - (i * kChunksSize);
      task_runner->PostTask(
          FROM_HERE,
          base::BindOnce(&DummyWork, &buffers_[i * kChunksSize], chunk_size));
      threads_.push_back(std::move(task_runner));
    }
    return 0;
  }

 private:
  // The amount of memory to keep "free" for the other processes. This program
  // will do its best to keep |[Physical memory in MB] - free_memory_mb_| in its
  // resident memory.
  size_t free_memory_mb_;

  // The threads that do the dummy work.
  std::vector<scoped_refptr<base::SequencedTaskRunner>> threads_;

  // The memory buffers.
  std::vector<uint32_t*> buffers_;
};

}  // namespace memory_pressure_simulator

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  memory_pressure_simulator::MemoryPressureSimulatorApp app;
  base::TaskScheduler::CreateAndStartWithDefaultParams(
      "memory_pressure_simulator");
  int ret = app.Run(argc, argv);
  // TODO: Add a logging function to MemoryPressureSimulatorApp to continuously
  // show some system metrics (resident memory of each process, commit charge,
  // rate of hard page faults of each processes...)  and call it from here.
  base::TaskScheduler::GetInstance()->Shutdown();
  return ret;
}
