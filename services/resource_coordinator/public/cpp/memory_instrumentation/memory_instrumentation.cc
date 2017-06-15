// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"

#include "base/threading/thread_local_storage.h"

namespace memory_instrumentation {

namespace {

MemoryInstrumentation* g_instance = nullptr;
base::ThreadLocalStorage::StaticSlot g_tls = TLS_INITIALIZER;

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
    : connector_(connector), service_name_(service_name) {
  g_tls.Initialize([](void* tls_object) {
    delete reinterpret_cast<mojom::CoordinatorPtr*>(tls_object);
  });
}

MemoryInstrumentation::~MemoryInstrumentation() {
  g_instance = nullptr;
}

void MemoryInstrumentation::RequestGlobalDumpAndAppendToTrace(
    MemoryDumpType dump_type,
    MemoryDumpLevelOfDetail level_of_detail,
    RequestGlobalDumpAndAppendToTraceCallback callback) {
  const auto& coordinator = GetCoordinatorBindingForCurrentThread();
  auto callback_adapter = [](RequestGlobalDumpAndAppendToTraceCallback callback,
                             uint64_t dump_id, bool success,
                             mojom::GlobalMemoryDumpPtr) {
    if (callback)
      callback.Run(success, dump_id);
  };
  // TODO(primiano): get rid of dump_id. It should be a return-only argument.
  static uint64_t dump_id = 0;
  base::trace_event::MemoryDumpRequestArgs args = {++dump_id, dump_type,
                                                   level_of_detail};
  coordinator->RequestGlobalMemoryDump(args,
                                       base::Bind(callback_adapter, callback));
}

const mojom::CoordinatorPtr&
MemoryInstrumentation::GetCoordinatorBindingForCurrentThread() {
  auto* coordinator = reinterpret_cast<mojom::CoordinatorPtr*>(g_tls.Get());
  if (!coordinator) {
    coordinator = new mojom::CoordinatorPtr();
    connector_->BindInterface(service_name_, mojo::MakeRequest(coordinator));
    g_tls.Set(coordinator);
  }
  DCHECK(*coordinator);
  return *coordinator;
}

}  // namespace memory_instrumentation
