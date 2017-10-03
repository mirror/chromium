// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/DedicatedWorkerMessagingProxy.h"

#include <memory>

#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/workers/DedicatedWorkerThread.h"
#include "core/workers/WorkerClients.h"
#include "services/service_manager/public/interfaces/interface_provider.mojom-blink.h"
#include "third_party/WebKit/public/platform/dedicated_worker_factory.mojom-blink.h"

namespace blink {

DedicatedWorkerMessagingProxy::DedicatedWorkerMessagingProxy(
    InProcessWorkerBase* worker_object,
    WorkerClients* worker_clients)
    : InProcessWorkerMessagingProxy(worker_object, worker_clients) {}

DedicatedWorkerMessagingProxy::~DedicatedWorkerMessagingProxy() {}

bool DedicatedWorkerMessagingProxy::IsAtomicsWaitAllowed() {
  return true;
}

WTF::Optional<WorkerBackingThreadStartupData>
DedicatedWorkerMessagingProxy::CreateBackingThreadStartupData(
    v8::Isolate* isolate) {
  using HeapLimitMode = WorkerBackingThreadStartupData::HeapLimitMode;
  using AtomicsWaitMode = WorkerBackingThreadStartupData::AtomicsWaitMode;
  return WorkerBackingThreadStartupData(
      isolate->IsHeapLimitIncreasedForDebugging()
          ? HeapLimitMode::kIncreasedForDebugging
          : HeapLimitMode::kDefault,
      IsAtomicsWaitAllowed() ? AtomicsWaitMode::kAllow
                             : AtomicsWaitMode::kDisallow);
}

std::unique_ptr<WorkerThread>
DedicatedWorkerMessagingProxy::CreateWorkerThread() {
  return DedicatedWorkerThread::Create(CreateThreadableLoadingContext(),
                                       WorkerObjectProxy());
}

service_manager::mojom::blink::InterfaceProviderPtrInfo
DedicatedWorkerMessagingProxy::ConnectToWorkerInterfaceProvider(
    const RefPtr<SecurityOrigin>& script_origin) {
  auto* frame = ToDocument(GetExecutionContext())->GetFrame();
  if (!frame)
    return {};

  mojom::blink::DedicatedWorkerFactoryPtr worker_factory;
  frame->GetInterfaceProvider().GetInterface(&worker_factory);
  service_manager::mojom::blink::InterfaceProviderPtrInfo interface_provider;
  worker_factory->CreateDedicatedWorker(script_origin->IsUnique(),
                                        mojo::MakeRequest(&interface_provider));
  return interface_provider;
}

}  // namespace blink
