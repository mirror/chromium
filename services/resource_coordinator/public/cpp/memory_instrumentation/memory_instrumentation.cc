// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

namespace memory_instrumentation {

namespace {
MemoryInstrumentation* g_instance = nullptr;

void DestroyCoordinatorTLS(void* tls_object) {
  delete reinterpret_cast<mojom::CoordinatorPtr*>(tls_object);
};
}  // namespace

// static
void MemoryInstrumentation::CreateInstance(
    service_manager::Connector* connector,
    const std::string& service_name) {
  DCHECK(!g_instance);
  g_instance = new MemoryInstrumentation(connector, service_name);
}

// static
MemoryInstrumentation* MemoryInstrumentation::GetInstance() {
  return g_instance;
}

MemoryInstrumentation::MemoryInstrumentation(
    service_manager::Connector* connector,
    const std::string& service_name)
    : connector_(connector),
      connector_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      tls_coordinator_(&DestroyCoordinatorTLS),
      service_name_(service_name) {
  DCHECK(connector_task_runner_);
}

MemoryInstrumentation::~MemoryInstrumentation() {
  g_instance = nullptr;
}

void MemoryInstrumentation::RequestGlobalDump(
    RequestGlobalDumpCallback callback) {
  const auto& coordinator = GetCoordinatorBindingForCurrentThread();
  coordinator->RequestGlobalMemoryDump(MemoryDumpType::SUMMARY_ONLY,
                                       MemoryDumpLevelOfDetail::BACKGROUND,
                                       callback);
}

void MemoryInstrumentation::RequestGlobalDumpAndAppendToTrace(
    MemoryDumpType dump_type,
    MemoryDumpLevelOfDetail level_of_detail,
    RequestGlobalMemoryDumpAndAppendToTraceCallback callback) {
  const auto& coordinator = GetCoordinatorBindingForCurrentThread();
  coordinator->RequestGlobalMemoryDumpAndAppendToTrace(
      dump_type, level_of_detail, callback);
}

void MemoryInstrumentation::GetVmRegionsForHeapProfiler(
    RequestGlobalDumpCallback callback) {
  const auto& heap_profiling = GetHeapProfilingBindingForCurrentThread();
  heap_profiling->GetVmRegionsForHeapProfiler(callback);
}

const mojom::CoordinatorPtr&
MemoryInstrumentation::GetCoordinatorBindingForCurrentThread() {
  mojom::CoordinatorPtr* coordinator =
      reinterpret_cast<mojom::CoordinatorPtr*>(tls_coordinator_.Get());
  if (!coordinator) {
    coordinator = new mojom::CoordinatorPtr();
    tls_coordinator_.Set(coordinator);
    mojom::CoordinatorRequest coordinator_req = mojo::MakeRequest(coordinator);

    // The connector is not thread safe and BindInterface must be called on its
    // own thread. Thankfully, the binding can happen _after_ having started
    // invoking methods on the |coordinator| proxy objects.
    connector_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(
            &MemoryInstrumentation::BindCoordinatorRequestOnConnectorThread,
            base::Unretained(this), base::Passed(std::move(coordinator_req))));
  }
  DCHECK(*coordinator);
  return *coordinator;
}

void MemoryInstrumentation::BindCoordinatorRequestOnConnectorThread(
    mojom::CoordinatorRequest coordinator_request) {
  connector_->BindInterface(service_name_, std::move(coordinator_request));
}

const mojom::HeapProfilingPtr&
MemoryInstrumentation::GetHeapProfilingBindingForCurrentThread() {
  mojom::HeapProfilingPtr* heap_profiling =
      reinterpret_cast<mojom::HeapProfilingPtr*>(tls_heap_profiling_.Get());
  if (!heap_profiling) {
    heap_profiling = new mojom::HeapProfilingPtr();
    tls_heap_profiling_.Set(heap_profiling);
    mojom::HeapProfilingRequest heap_profiling_req =
        mojo::MakeRequest(heap_profiling);

    // The connector is not thread safe and BindInterface must be called on its
    // own thread. Thankfully, the binding can happen _after_ having started
    // invoking methods on the |heap_profiling| proxy objects.
    connector_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(
            &MemoryInstrumentation::BindHeapProfilingRequestOnConnectorThread,
            base::Unretained(this),
            base::Passed(std::move(heap_profiling_req))));
  }
  DCHECK(*heap_profiling);
  return *heap_profiling;
}

void MemoryInstrumentation::BindHeapProfilingRequestOnConnectorThread(
    mojom::HeapProfilingRequest heap_profiling_request) {
  connector_->BindInterface(service_name_, std::move(heap_profiling_request));
}

}  // namespace memory_instrumentation
