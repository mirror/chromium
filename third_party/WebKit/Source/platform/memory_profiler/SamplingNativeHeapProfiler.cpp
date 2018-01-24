// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/memory_profiler/SamplingNativeHeapProfiler.h"

#include <cmath>

#include "base/allocator/features.h"
#include "base/atomicops.h"
#include "base/bits.h"
#include "base/debug/alias.h"
#include "base/debug/stack_trace.h"
#include "base/memory/singleton.h"
#include "base/rand_util.h"
#include "build/build_config.h"

namespace blink {

using base::allocator::AllocatorDispatch;
using base::subtle::Atomic32;
using base::subtle::AtomicWord;

namespace {

const unsigned kMagicSignature = 0x14690ca5;
const unsigned kDefaultAlignment = 16;
const unsigned kSkipAllocatorFrames = 4;

Atomic32 g_running;
Atomic32 g_deterministic;
AtomicWord g_bytes_left;
AtomicWord g_current_interval;
AtomicWord g_sampling_interval = 128 * 1024;
unsigned g_last_sample_ordinal = 0;

inline bool HasBeenSampledFastCheck(void* address) {
  return address && reinterpret_cast<unsigned*>(address)[-1] == kMagicSignature;
}

}  // namespace

SamplingNativeHeapProfiler::Sample::Sample(size_t size,
                                           size_t count,
                                           unsigned ordinal,
                                           unsigned offset)
    : size(size), count(count), ordinal(ordinal), offset(offset) {}

SamplingHeapProfiler* SamplingHeapProfiler::GetInstance() {
  return SamplingNativeHeapProfiler::GetInstance();
}

void* SamplingNativeHeapProfiler::AllocFn(const AllocatorDispatch* self,
                                          size_t size,
                                          void* context) {
  size_t accumulated;
  if (LIKELY(!ShouldRecordSample(size, &accumulated))) {
    return self->next->alloc_function(self->next, size, context);
  }
  void* address =
      self->next->alloc_function(self->next, size + kDefaultAlignment, context);
  return GetInstance()->RecordAlloc(accumulated, size, address,
                                    kDefaultAlignment, kSkipAllocatorFrames);
}

// static
void* SamplingNativeHeapProfiler::AllocZeroInitializedFn(
    const AllocatorDispatch* self,
    size_t n,
    size_t size,
    void* context) {
  size_t accumulated;
  if (LIKELY(!ShouldRecordSample(n * size, &accumulated))) {
    return self->next->alloc_zero_initialized_function(self->next, n, size,
                                                       context);
  }
  void* address = self->next->alloc_zero_initialized_function(
      self->next, 1, n * size + kDefaultAlignment, context);
  return GetInstance()->RecordAlloc(accumulated, n * size, address,
                                    kDefaultAlignment, kSkipAllocatorFrames);
}

// static
void* SamplingNativeHeapProfiler::AllocAlignedFn(const AllocatorDispatch* self,
                                                 size_t alignment,
                                                 size_t size,
                                                 void* context) {
  size_t accumulated;
  if (LIKELY(!ShouldRecordSample(size, &accumulated))) {
    return self->next->alloc_aligned_function(self->next, alignment, size,
                                              context);
  }
  size_t offset = base::bits::Align(sizeof(kMagicSignature), alignment);
  void* address = self->next->alloc_aligned_function(self->next, alignment,
                                                     size + offset, context);
  return GetInstance()->RecordAlloc(accumulated, size, address, offset,
                                    kSkipAllocatorFrames);
}

// static
void* SamplingNativeHeapProfiler::ReallocFn(const AllocatorDispatch* self,
                                            void* address,
                                            size_t size,
                                            void* context) {
  // Note: size == 0 actually performs free.
  size_t accumulated;
  bool will_sample = ShouldRecordSample(size, &accumulated);
  char* client_address = reinterpret_cast<char*>(address);
  if (UNLIKELY(HasBeenSampledFastCheck(address)))
    address = GetInstance()->RecordFree(address);
  intptr_t prev_offset = client_address - reinterpret_cast<char*>(address);
  bool was_sampled = prev_offset;
  if (LIKELY(!was_sampled && !will_sample))
    return self->next->realloc_function(self->next, address, size, context);
  size_t size_to_allocate = will_sample ? size + kDefaultAlignment : size;
  address = self->next->realloc_function(self->next, address, size_to_allocate,
                                         context);
  if (will_sample) {
    return GetInstance()->RecordAlloc(
        accumulated, size, address, kDefaultAlignment, kSkipAllocatorFrames,
        client_address && prev_offset != kDefaultAlignment);
  }
  DCHECK(was_sampled && !will_sample);
  memmove(address, reinterpret_cast<char*>(address) + prev_offset, size);
  return address;
}

// static
void SamplingNativeHeapProfiler::FreeFn(const AllocatorDispatch* self,
                                        void* address,
                                        void* context) {
  if (UNLIKELY(HasBeenSampledFastCheck(address)))
    address = GetInstance()->RecordFree(address);
  self->next->free_function(self->next, address, context);
}

// static
size_t SamplingNativeHeapProfiler::GetSizeEstimateFn(
    const AllocatorDispatch* self,
    void* address,
    void* context) {
  return self->next->get_size_estimate_function(self->next, address, context);
}

// static
unsigned SamplingNativeHeapProfiler::BatchMallocFn(
    const AllocatorDispatch* self,
    size_t size,
    void** results,
    unsigned num_requested,
    void* context) {
  unsigned num_allocated = self->next->batch_malloc_function(
      self->next, size + kDefaultAlignment, results, num_requested, context);
  for (unsigned i = 0; i < num_allocated; ++i) {
    size_t accumulated;
    if (UNLIKELY(ShouldRecordSample(size, &accumulated))) {
      results[i] =
          GetInstance()->RecordAlloc(accumulated, size, results[i],
                                     kDefaultAlignment, kSkipAllocatorFrames);
    }
  }
  return num_allocated;
}

// static
void SamplingNativeHeapProfiler::BatchFreeFn(const AllocatorDispatch* self,
                                             void** to_be_freed,
                                             unsigned num_to_be_freed,
                                             void* context) {
  for (unsigned i = 0; i < num_to_be_freed; ++i) {
    if (UNLIKELY(HasBeenSampledFastCheck(to_be_freed[i])))
      to_be_freed[i] = GetInstance()->RecordFree(to_be_freed[i]);
  }
  self->next->batch_free_function(self->next, to_be_freed, num_to_be_freed,
                                  context);
}

// static
void SamplingNativeHeapProfiler::FreeDefiniteSizeFn(
    const AllocatorDispatch* self,
    void* address,
    size_t size,
    void* context) {
  if (UNLIKELY(HasBeenSampledFastCheck(address))) {
    void* client_address = address;
    address = GetInstance()->RecordFree(address);
    size += reinterpret_cast<uint8_t*>(client_address) -
            reinterpret_cast<uint8_t*>(address);
  }
  self->next->free_definite_size_function(self->next, address, size, context);
}

AllocatorDispatch SamplingNativeHeapProfiler::allocator_dispatch_ = {
    &AllocFn,
    &AllocZeroInitializedFn,
    &AllocAlignedFn,
    &ReallocFn,
    &FreeFn,
    &GetSizeEstimateFn,
    &BatchMallocFn,
    &BatchFreeFn,
    &FreeDefiniteSizeFn,
    nullptr};

// static
void SamplingNativeHeapProfiler::InstallAllocatorHooksOnce() {
  static bool hook_installed = InstallAllocatorHooks();
  base::debug::Alias(&hook_installed);
}

// static
bool SamplingNativeHeapProfiler::InstallAllocatorHooks() {
#if BUILDFLAG(USE_ALLOCATOR_SHIM)
  base::allocator::InsertAllocatorDispatch(&allocator_dispatch_);
#else
  base::debug::Alias(&allocator_dispatch_);
  CHECK(false)
      << "Can't enable native sampling heap profiler without the shim.";
#endif  // BUILDFLAG(USE_ALLOCATOR_SHIM)
  return true;
}

unsigned SamplingNativeHeapProfiler::Start() {
  InstallAllocatorHooksOnce();
  intptr_t next_interval =
      GetNextSampleInterval(base::subtle::Acquire_Load(&g_sampling_interval));
  base::subtle::Release_Store(&g_current_interval, next_interval);
  base::subtle::Release_Store(&g_bytes_left, next_interval);
  base::subtle::Barrier_AtomicIncrement(&g_running, 1);
  return g_last_sample_ordinal;
}

void SamplingNativeHeapProfiler::Stop() {
  AtomicWord count = base::subtle::Barrier_AtomicIncrement(&g_running, -1);
  CHECK_GE(count, 0);
}

void SamplingNativeHeapProfiler::SetSamplingInterval(
    unsigned sampling_interval) {
  // Note: when changed dynamically, the current sample will still use
  // the previous sampling interval.
  base::subtle::Release_Store(&g_sampling_interval, sampling_interval);
}

// static
size_t SamplingNativeHeapProfiler::GetNextSampleInterval(size_t interval) {
  if (UNLIKELY(base::subtle::NoBarrier_Load(&g_deterministic)))
    return static_cast<size_t>(interval);

  // We sample with a Poisson process, with constant average sampling
  // interval. This follows the exponential probability distribution with
  // parameter λ = 1/interval where |interval| is the average number of bytes
  // between samples.
  // Let u be a uniformly distributed random number between 0 and 1, then
  // next_sample = -ln(u) / λ
  double uniform = base::RandDouble();
  double value = -log(uniform) * interval;
  size_t min_value = sizeof(intptr_t);
  // We limit the upper bound of a sample interval to make sure we don't have
  // huge gaps in the sampling stream. Probability of the upper bound gets hit
  // is exp(-20) ~ 2e-9, so it should not skew the distibution.
  size_t max_value = interval * 20;
  if (UNLIKELY(value < min_value))
    return min_value;
  if (UNLIKELY(value > max_value))
    return max_value;
  return static_cast<size_t>(value);
}

// static
bool SamplingNativeHeapProfiler::ShouldRecordSample(size_t size,
                                                    size_t* accumulated) {
  if (LIKELY(!base::subtle::NoBarrier_Load(&g_running)))
    return false;

  // Lock-free algorithm that adds the allocation size to the cumulative
  // counter. When the counter reaches threshold, it picks a single thread
  // that will record the sample and reset the counter.
  // The thread that records the sample returns true, others return false.
  AtomicWord bytes_left = base::subtle::NoBarrier_AtomicIncrement(
      &g_bytes_left, -static_cast<AtomicWord>(size));
  if (LIKELY(bytes_left > 0))
    return false;

  // Return if g_bytes_left was already zero or below before we decreased it.
  // That basically means that another thread in fact crossed the threshold.
  // That other thread is responsible for recording the sample.
  if (LIKELY(bytes_left + static_cast<intptr_t>(size) <= 0))
    return false;

  // Only one thread that crossed the threshold is running the code below.
  // It is going to be recording the sample.
  size_t next_interval =
      GetNextSampleInterval(base::subtle::NoBarrier_Load(&g_sampling_interval));
  size_t last_interval = base::subtle::NoBarrier_AtomicExchange(
      &g_current_interval, next_interval);
  // Make sure g_current_interval is updated before staring next sample.
  base::subtle::MemoryBarrier();

  // Put the next sampling interval to g_bytes_left, thus allowing threads to
  // start accumulating bytes towards the next sample.
  // Simulateneously extract the current value (which is negative or zero)
  // and take it into account when calculating the number of bytes
  // accumulated during the current sample.
  *accumulated = last_interval - base::subtle::NoBarrier_AtomicExchange(
                                     &g_bytes_left, next_interval);

  DCHECK_NE(size, 0u);
  return true;
}

void SamplingNativeHeapProfiler::RecordStackTrace(Sample* sample,
                                                  unsigned skip_frames) {
  // TODO(alph): Consider using debug::TraceStackFramePointers. It should be
  // somewhat faster than base::debug::StackTrace.
  base::debug::StackTrace trace;
  size_t count;
  void* const* addresses = const_cast<void* const*>(trace.Addresses(&count));
  // Skip SamplingNativeHeapProfiler frames.
  sample->stack.insert(
      sample->stack.end(), &addresses[skip_frames],
      &addresses[std::max(count, static_cast<size_t>(skip_frames))]);
}

void* SamplingNativeHeapProfiler::RecordAlloc(size_t total_allocated,
                                              size_t allocation_size,
                                              void* address,
                                              unsigned offset,
                                              unsigned skip_frames,
                                              bool preserve_data) {
  if (entered_.Get())
    return address;
  base::AutoLock lock(mutex_);
  entered_.Set(true);
  size_t size = allocation_size;
  size_t count = std::max<size_t>(1, (total_allocated + size / 2) / size);
  Sample sample(size, count, ++g_last_sample_ordinal, offset);
  void* client_address = reinterpret_cast<char*>(address) + offset;
  if (preserve_data)
    memmove(client_address, address, size);
  RecordStackTrace(&sample, skip_frames);
  samples_.insert(std::make_pair(client_address, std::move(sample)));
  if (offset)
    reinterpret_cast<unsigned*>(client_address)[-1] = kMagicSignature;
  entered_.Set(false);
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
SamplingNativeHeapProfiler* SamplingNativeHeapProfiler::GetInstance() {
  return base::Singleton<SamplingNativeHeapProfiler>::get();
}

// static
void SamplingNativeHeapProfiler::SuppressRandomnessForTest() {
  base::subtle::Release_Store(&g_deterministic, true);
}

std::vector<SamplingNativeHeapProfiler::Sample>
SamplingNativeHeapProfiler::GetSamples(unsigned profile_id) {
  base::AutoLock lock(mutex_);
  CHECK(!entered_.Get());
  entered_.Set(true);
  std::vector<Sample> samples;
  for (auto it = samples_.begin(); it != samples_.end(); ++it) {
    Sample& sample = it->second;
    if (sample.ordinal <= profile_id)
      continue;
    samples.push_back(sample);
  }
  entered_.Set(false);
  return samples;
}

}  // namespace blink
