// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/device_memory/sampling_native_heap_profiler.h"

#include <cmath>

#include "base/allocator/allocator_shim.h"
// #include "base/allocator/features.h"
#include "base/atomicops.h"
#include "base/debug/stack_trace.h"
#include "base/memory/singleton.h"
#include "base/rand_util.h"
// #include "build/build_config.h"

namespace blink {

using base::allocator::AllocatorDispatch;
using base::subtle::AtomicWord;

namespace {
size_t RoundUp(size_t value, size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}
}  // namespace

AtomicWord SamplingNativeHeapProfiler::cumulative_counter_ = 0;
AtomicWord SamplingNativeHeapProfiler::threshold_;
AtomicWord SamplingNativeHeapProfiler::sampling_interval_ = 128 * 1024;
bool SamplingNativeHeapProfiler::running_ = false;
bool SamplingNativeHeapProfiler::entered_ = false;

AllocatorDispatch SamplingNativeHeapProfiler::allocator_dispatch_ = {
    &SamplingNativeHeapProfiler::AllocFn,
    &SamplingNativeHeapProfiler::AllocZeroInitializedFn,
    &SamplingNativeHeapProfiler::AllocAlignedFn,
    &SamplingNativeHeapProfiler::ReallocFn,
    &SamplingNativeHeapProfiler::FreeFn,
    &SamplingNativeHeapProfiler::GetSizeEstimateFn,
    &SamplingNativeHeapProfiler::BatchMallocFn,
    &SamplingNativeHeapProfiler::BatchFreeFn,
    &SamplingNativeHeapProfiler::FreeDefiniteSizeFn,
    nullptr};

void SamplingNativeHeapProfiler::InstallAllocatorDispatchOnce() {
  std::call_once(initialized_, []() {
    //#if BUILDFLAG(USE_ALLOCATOR_SHIM)
    base::allocator::InsertAllocatorDispatch(&allocator_dispatch_);
    //#else
    //    CHECK(false) << "Can't enable native sampling heap profiler without
    //    the shim.";
    //#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)
  });
}

void SamplingNativeHeapProfiler::Start() {
  InstallAllocatorDispatchOnce();
  // TODO(alph): Update the threshold, as the sampling_interval_ could have
  // been changed.
  base::subtle::Release_Store(&threshold_, sampling_interval_);
  running_ = true;
}

void SamplingNativeHeapProfiler::Stop() {
  running_ = false;
}

void SamplingNativeHeapProfiler::SetSamplingInterval(
    unsigned sampling_interval) {
  base::subtle::Release_Store(&sampling_interval_, sampling_interval);
}

// static
intptr_t SamplingNativeHeapProfiler::GetNextSampleInterval(uint64_t interval) {
  // We sample with a Poisson process, with constant average sampling
  // interval. This follows the exponential probability distribution with
  // parameter λ = 1/interval where |interval| is the average number of bytes
  // between samples.
  // Let u be a uniformly distributed random number between 0 and 1, then
  // next_sample = -ln(u) / λ
  double uniform = base::RandDouble();
  double value = -log(uniform) * interval;
  intptr_t min_value = sizeof(intptr_t);
  // We limit the upper bound of a sample interval to make sure we don't have
  // huge gaps in the sampling stream. Probability of the upper bound gets hit
  // is exp(-20) ~ 2e-9, so it should not skew the distibution.
  intptr_t max_value = interval * 20;
  if (UNLIKELY(value < min_value))
    return min_value;
  if (UNLIKELY(value > max_value))
    return max_value;
  return static_cast<intptr_t>(value);
}

// static
bool SamplingNativeHeapProfiler::CreateAllocSample(size_t size,
                                                   Sample* sample) {
  // Lock-free algorithm that adds the allocation size to the cumulative
  // counter. When the counter reaches threshold, it picks a single thread
  // that will record the sample and reset the counter.
  // The thread that records the sample returns true, others return false.
  AtomicWord accumulated = base::subtle::NoBarrier_AtomicIncrement(
      &cumulative_counter_, static_cast<AtomicWord>(size));
  if (LIKELY(accumulated < base::subtle::NoBarrier_Load(&threshold_)))
    return false;
  // On an extremely rare occasion one thread makes an allocation that triggers
  // sampling and another thread makes a zero-size allocation and then gets
  // selected to record a sample. That would cause division by zero below.
  // Skip it and make the next allocation record a sample.
  if (UNLIKELY(size == 0))
    return false;
  const AtomicWord a_big_number = std::numeric_limits<AtomicWord>::max() / 2;
  // Allow a single thread down the branch below.
  // Also temporary deposit a huge negative value to cumulative_counter_
  // so it won't trigger again until we finish recording the sample.
  if (UNLIKELY(base::subtle::Acquire_CompareAndSwap(
                   &cumulative_counter_, accumulated, -a_big_number) !=
               accumulated)) {
    return false;
  }
  intptr_t next_interval =
      GetNextSampleInterval(base::subtle::NoBarrier_Load(&sampling_interval_));
  base::subtle::NoBarrier_Store(&threshold_, next_interval);
  base::subtle::Barrier_AtomicIncrement(&cumulative_counter_, a_big_number);
  sample->size = size;
  sample->count = std::max(1ul, (accumulated + size / 2) / size);
  return true;
}

void SamplingNativeHeapProfiler::RecordStackTrace(Sample* sample,
                                                  unsigned skip_frames) {
  base::debug::StackTrace trace;
  size_t count;
  void* const* addresses = const_cast<void* const*>(trace.Addresses(&count));
  // Skip SamplingNativeHeapProfiler frames.
  sample->stack.insert(
      sample->stack.end(), &addresses[skip_frames],
      &addresses[std::max(count, static_cast<size_t>(skip_frames))]);
}

void* SamplingNativeHeapProfiler::RecordAlloc(Sample& sample,
                                              void* address,
                                              unsigned offset,
                                              unsigned skip_frames,
                                              bool preserve_data) {
  // TODO(alph): It's better to use a recursive mutex and move the check
  // inside the critical section. This way we won't skip the sample generation
  // on one thread if another thread is recording a sample.
  if (entered_)
    return address;
  base::AutoLock lock(mutex_);
  entered_ = true;
  sample.offset = offset;
  void* client_address = reinterpret_cast<char*>(address) + offset;
  if (preserve_data)
    memmove(client_address, address, sample.size);
  RecordStackTrace(&sample, skip_frames);
  samples_.insert(std::make_pair(client_address, sample));
  if (offset)
    reinterpret_cast<unsigned*>(client_address)[-1] = magic_signature_;
  entered_ = false;
  return client_address;
}

void* SamplingNativeHeapProfiler::RecordFree(void* address) {
  base::AutoLock lock(mutex_);
  auto& samples = GetInstance()->samples_;
  auto it = samples.find(address);
  if (it == samples.end())
    return address;
  void* address_to_free = reinterpret_cast<char*>(address) - it->second.offset;
  samples.erase(it);
  if (it->second.offset)
    reinterpret_cast<unsigned*>(address)[-1] = 0;
  return address_to_free;
}

// static
void* SamplingNativeHeapProfiler::AllocFn(const AllocatorDispatch* self,
                                          size_t size,
                                          void* context) {
  Sample sample;
  if (LIKELY(!running_ || !CreateAllocSample(size, &sample)))
    return self->next->alloc_function(self->next, size, context);
  void* address = self->next->alloc_function(
      self->next, size + default_alignment_, context);
  return GetInstance()->RecordAlloc(sample, address, default_alignment_,
                                    skip_allocator_frames_);
}

// static
void* SamplingNativeHeapProfiler::AllocZeroInitializedFn(
    const AllocatorDispatch* self,
    size_t n,
    size_t size,
    void* context) {
  Sample sample;
  if (LIKELY(!running_ || !CreateAllocSample(n * size, &sample)))
    return self->next->alloc_zero_initialized_function(self->next, n, size,
                                                       context);
  void* address = self->next->alloc_zero_initialized_function(
      self->next, 1, n * size + default_alignment_, context);
  return GetInstance()->RecordAlloc(sample, address, default_alignment_,
                                    skip_allocator_frames_);
}

// static
void* SamplingNativeHeapProfiler::AllocAlignedFn(const AllocatorDispatch* self,
                                                 size_t alignment,
                                                 size_t size,
                                                 void* context) {
  Sample sample;
  if (LIKELY(!running_ || !CreateAllocSample(size, &sample)))
    return self->next->alloc_aligned_function(self->next, alignment, size,
                                              context);
  size_t offset =
      static_cast<unsigned>(RoundUp(sizeof(magic_signature_), alignment));
  void* address = self->next->alloc_aligned_function(self->next, alignment,
                                                     size + offset, context);
  return GetInstance()->RecordAlloc(sample, address, offset,
                                    skip_allocator_frames_);
}

// static
void* SamplingNativeHeapProfiler::ReallocFn(const AllocatorDispatch* self,
                                            void* address,
                                            size_t size,
                                            void* context) {
  Sample sample;
  bool will_sample = running_ && CreateAllocSample(size, &sample);
  char* client_address = reinterpret_cast<char*>(address);
  if (UNLIKELY(address &&
               reinterpret_cast<unsigned*>(address)[-1] == magic_signature_))
    address = GetInstance()->RecordFree(address);
  intptr_t prev_offset = client_address - reinterpret_cast<char*>(address);
  bool was_sampled = prev_offset;
  if (LIKELY(!was_sampled && !will_sample))
    return self->next->realloc_function(self->next, address, size, context);
  size_t size_to_allocate = will_sample ? size + default_alignment_ : size;
  address = self->next->realloc_function(self->next, address, size_to_allocate,
                                         context);
  if (will_sample) {
    return GetInstance()->RecordAlloc(
        sample, address, default_alignment_, skip_allocator_frames_,
        client_address != nullptr && prev_offset != default_alignment_);
  }
  DCHECK(was_sampled && !will_sample);
  memmove(address, reinterpret_cast<char*>(address) + prev_offset, size);
  return address;
}

// static
void SamplingNativeHeapProfiler::FreeFn(const AllocatorDispatch* self,
                                        void* address,
                                        void* context) {
  if (UNLIKELY(address &&
               reinterpret_cast<unsigned*>(address)[-1] == magic_signature_)) {
    address = GetInstance()->RecordFree(address);
  }
  self->next->free_function(self->next, address, context);
}

// static
size_t SamplingNativeHeapProfiler::GetSizeEstimateFn(
    const AllocatorDispatch* self,
    void* address,
    void* context) {
  size_t ret =
      self->next->get_size_estimate_function(self->next, address, context);
  return ret;
}

// static
unsigned SamplingNativeHeapProfiler::BatchMallocFn(
    const AllocatorDispatch* self,
    size_t size,
    void** results,
    unsigned num_requested,
    void* context) {
  CHECK(false) << "Not implemented.";
  return 0;
}

// static
void SamplingNativeHeapProfiler::BatchFreeFn(const AllocatorDispatch* self,
                                             void** to_be_freed,
                                             unsigned num_to_be_freed,
                                             void* context) {
  CHECK(false) << "Not implemented.";
}

// static
void SamplingNativeHeapProfiler::FreeDefiniteSizeFn(
    const AllocatorDispatch* self,
    void* ptr,
    size_t size,
    void* context) {
  CHECK(false) << "Not implemented.";
}

// static
SamplingNativeHeapProfiler* SamplingNativeHeapProfiler::GetInstance() {
  return base::Singleton<SamplingNativeHeapProfiler>::get();
}

std::vector<SamplingNativeHeapProfiler::Sample>
SamplingNativeHeapProfiler::GetSamples() {
  base::AutoLock lock(mutex_);
  CHECK(!entered_);
  entered_ = true;
  std::vector<SamplingNativeHeapProfiler::Sample> samples;
  for (auto it = samples_.begin(); it != samples_.end(); ++it)
    samples.push_back(it->second);
  entered_ = false;
  return samples;
}

}  // namespace blink
