// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "controller/OomInterventionImpl.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "platform/WebTaskRunner.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/allocator/Partitions.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

// Roughly caclculates amount of memory which is used to execute pages.
size_t BlinkMemoryWorkloadCaculator() {
  v8::Isolate* isolate = V8PerIsolateData::MainThreadIsolate();
  DCHECK(isolate);
  v8::HeapStatistics heap_statistics;
  isolate->GetHeapStatistics(&heap_statistics);
  // TODO: Add memory usage for worker threads.
  size_t v8_size =
      heap_statistics.total_heap_size() + heap_statistics.malloced_memory();
  size_t blink_gc_size = ProcessHeap::TotalAllocatedObjectSize() +
                         ProcessHeap::TotalMarkedObjectSize();
  size_t partition_alloc_size = WTF::Partitions::TotalSizeOfCommittedPages();
  return v8_size + blink_gc_size + partition_alloc_size;
}

}  // namespace

// If memory workload is above this threshold, we assume that we are in a
// near-OOM situation.
const size_t OomInterventionImpl::kMemoryWorkloadThreshold = 80 * 1024 * 1024;

constexpr WTF::TimeDelta OomInterventionImpl::kTimeout =
    WTF::TimeDelta::FromSeconds(60);

// static
void OomInterventionImpl::Create(mojom::blink::OomInterventionRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<OomInterventionImpl>(
          WTF::BindRepeating(&BlinkMemoryWorkloadCaculator)),
      std::move(request));
}

OomInterventionImpl::OomInterventionImpl(
    MemoryWorkloadCaculator workload_calculator)
    : workload_calculator_(std::move(workload_calculator)),
      weak_ptr_factory_(this) {
  DCHECK(workload_calculator_);
}

OomInterventionImpl::~OomInterventionImpl() = default;

void OomInterventionImpl::StartDetection(
    mojom::blink::OomInterventionHostPtr host,
    bool trigger_intervention) {
  host_ = std::move(host);
  timeout_ = CurrentTimeTicks() + kTimeout;
  trigger_intervention_ = trigger_intervention;

  Check();
}

void OomInterventionImpl::Check() {
  DCHECK(host_);

  if (CurrentTimeTicks() > timeout_) {
    host_->DetectionTimedOutOnRenderer();
    return;
  }

  size_t workload = workload_calculator_.Run();
  if (workload > kMemoryWorkloadThreshold) {
    host_->NearOomDetectedOnRenderer();

    if (trigger_intervention_) {
      // The ScopedPagePauser is destroyed when the intervention is declined and
      // mojo strong binding is disconnected.
      pauser_.reset(new ScopedPagePauser);
    }
    return;
  }

  Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostDelayedTask(
      FROM_HERE,
      WTF::Bind(&OomInterventionImpl::Check, weak_ptr_factory_.GetWeakPtr()),
      TimeDelta::FromSeconds(1));
}

}  // namespace blink
