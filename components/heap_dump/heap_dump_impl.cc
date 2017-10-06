// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/heap_dump/heap_dump_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace heap_dump {

namespace {

void BindHeapDump(heap_dump::mojom::HeapDumpRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<heap_dump::HeapDumpImpl>(),
                          std::move(request));
}

}  // namespace

HeapDumpImpl::HeapDumpImpl() {}

HeapDumpImpl::~HeapDumpImpl() {}

// static
void HeapDumpImpl::RegisterInterface() {
  auto registry = base::MakeUnique<service_manager::BinderRegistry>();
  registry->AddInterface(base::Bind(&BindHeapDump),
                         base::ThreadTaskRunnerHandle::Get());
  if (content::ChildThread::Get()) {
    content::ChildThread::Get()
        ->GetServiceManagerConnection()
        ->AddConnectionFilter(base::MakeUnique<content::SimpleConnectionFilter>(
            std::move(registry)));
  }
}

void HeapDumpImpl::RequestHeapDump(mojo::ScopedHandle file_handle,
                                   RequestHeapDumpCallback callback) {
  DebugBreak();
  std::move(callback).Run(false);
}

}  // namespace heap_dump
