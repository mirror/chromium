// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HEAP_PROFILER_SAMPLING_HEAP_PROFILER_H
#define BASE_HEAP_PROFILER_SAMPLING_HEAP_PROFILER_H

#include <unordered_map>
#include <vector>

#include "base/base_export.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_local.h"

namespace base {

template <typename T>
struct DefaultSingletonTraits;

namespace heap_profiler {

class BASE_EXPORT SamplingHeapProfiler {
 public:
  class Sample {
   public:
    size_t size;
    size_t count;
    std::vector<void*> stack;

    Sample(const Sample& other);
    ~Sample();

   private:
    friend class SamplingHeapProfiler;

    Sample(size_t, size_t count, uint32_t ordinal);

    uint32_t ordinal;
  };

  uint32_t Start();
  void Stop();
  void SetSamplingInterval(size_t sampling_interval);
  void SuppressRandomnessForTest();

  std::vector<Sample> GetSamples(uint32_t profile_id);

  static inline void MaybeRecordAlloc(void* address, size_t, uint32_t);
  static inline void MaybeRecordFree(void* address);

  static SamplingHeapProfiler* GetInstance();

 private:
  SamplingHeapProfiler();
  ~SamplingHeapProfiler();

  static void InstallAllocatorHooksOnce();
  static bool InstallAllocatorHooks();
  static size_t GetNextSampleInterval(size_t base_interval);

  void RecordAlloc(size_t total_allocated,
                   size_t allocation_size,
                   void* address,
                   uint32_t skip_frames);
  void RecordFree(void* address);
  void RecordStackTrace(Sample*, uint32_t skip_frames);

  base::ThreadLocalBoolean entered_;
  base::Lock mutex_;
  std::unordered_map<void*, Sample> samples_;

  friend struct base::DefaultSingletonTraits<SamplingHeapProfiler>;

  DISALLOW_COPY_AND_ASSIGN(SamplingHeapProfiler);
};

}  // namespace heap_profiler
}  // namespace base

#endif  // BASE_HEAP_PROFILER_SAMPLING_HEAP_PROFILER_H
