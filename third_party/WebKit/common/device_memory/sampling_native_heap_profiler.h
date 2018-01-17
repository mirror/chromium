// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_DEVICE_MEMORY_SAMPLING_NATIVE_HEAP_PROFILER_H_
#define THIRD_PARTY_WEBKIT_COMMON_DEVICE_MEMORY_SAMPLING_NATIVE_HEAP_PROFILER_H_

#include <mutex>
#include <unordered_map>
#include <vector>

#include "base/allocator/allocator_shim.h"
#include "base/atomicops.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace blink {

class SamplingNativeHeapProfiler {
 public:
  struct Sample {
    size_t size;
    size_t count;
    unsigned offset;
    std::vector<void*> stack;
  };

  SamplingNativeHeapProfiler() = default;

  void Start();
  void Stop();
  void SetSamplingInterval(unsigned sampling_interval);
  void SuppressRandomnessForTest() { deterministic_ = true; }

  std::vector<Sample> GetSamples();

  static SamplingNativeHeapProfiler* GetInstance();

 private:
  void InstallAllocatorDispatchOnce();
  static inline bool CreateAllocSample(size_t size, Sample*);
  static intptr_t GetNextSampleInterval(uint64_t base_interval);

  void* RecordAlloc(Sample& sample,
                    void* address,
                    unsigned offset,
                    unsigned skip_frames,
                    bool preserve_data = false);
  void* RecordFree(void* address);
  void RecordStackTrace(Sample* sample, unsigned skip_frames);

  std::once_flag initialized_;
  static base::subtle::AtomicWord cumulative_counter_;
  static base::subtle::AtomicWord threshold_;
  static base::subtle::AtomicWord sampling_interval_;
  static bool running_;
  static bool entered_;
  static bool deterministic_;
  static const unsigned magic_signature_ = 0x14690ca5;
  static const unsigned default_alignment_ = 16;
  static const unsigned skip_allocator_frames_ = 4;

  base::Lock mutex_;
  std::unordered_map<void*, Sample> samples_;

  static void* AllocFn(const base::allocator::AllocatorDispatch* self,
                       size_t size,
                       void* context);
  static void* AllocZeroInitializedFn(
      const base::allocator::AllocatorDispatch* self,
      size_t n,
      size_t size,
      void* context);
  static void* AllocAlignedFn(const base::allocator::AllocatorDispatch* self,
                              size_t alignment,
                              size_t size,
                              void* context);
  static void* ReallocFn(const base::allocator::AllocatorDispatch* self,
                         void* address,
                         size_t size,
                         void* context);
  static void FreeFn(const base::allocator::AllocatorDispatch* self,
                     void* address,
                     void* context);
  static size_t GetSizeEstimateFn(
      const base::allocator::AllocatorDispatch* self,
      void* address,
      void* context);
  static unsigned BatchMallocFn(const base::allocator::AllocatorDispatch* self,
                                size_t size,
                                void** results,
                                unsigned num_requested,
                                void* context);
  static void BatchFreeFn(const base::allocator::AllocatorDispatch* self,
                          void** to_be_freed,
                          unsigned num_to_be_freed,
                          void* context);
  static void FreeDefiniteSizeFn(const base::allocator::AllocatorDispatch* self,
                                 void* ptr,
                                 size_t size,
                                 void* context);

  // The allocator dispatch used to intercept heap operations.
  static base::allocator::AllocatorDispatch allocator_dispatch_;

  friend struct base::DefaultSingletonTraits<SamplingNativeHeapProfiler>;

  DISALLOW_COPY_AND_ASSIGN(SamplingNativeHeapProfiler);
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_DEVICE_MEMORY_SAMPLING_NATIVE_HEAP_PROFILER_H_
