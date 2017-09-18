// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerMediaPlayerFactory.h"

#include "core/workers/WorkerClients.h"
#include "platform/Supplementable.h"
#include "public/platform/WebMediaPlayerFactory.h"

namespace blink {

namespace {
const char kSupplementName[] = "WorkerMediaPlayerFactoryHolder";

class WorkerMediaPlayerFactoryHolder final
    : public GarbageCollectedFinalized<WorkerMediaPlayerFactoryHolder>,
      public Supplement<WorkerClients> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerMediaPlayerFactoryHolder);

 public:
  explicit WorkerMediaPlayerFactoryHolder(
      std::unique_ptr<WebMediaPlayerFactory> factory)
      : factory_(std::move(factory)) {}

  WebMediaPlayerFactory* GetFactory() const { return factory_.get(); }

  DEFINE_INLINE_VIRTUAL_TRACE() { Supplement<WorkerClients>::Trace(visitor); }

 private:
  std::unique_ptr<WebMediaPlayerFactory> factory_;
};

}  // namespace

// static
void WorkerMediaPlayerFactory::ProvideTo(
    WorkerClients* clients,
    std::unique_ptr<WebMediaPlayerFactory> factory) {
  WorkerMediaPlayerFactoryHolder::ProvideTo(
      *clients, kSupplementName,
      new WorkerMediaPlayerFactoryHolder(std::move(factory)));
}

// static
WebMediaPlayerFactory* WorkerMediaPlayerFactory::GetFrom(
    WorkerClients& clients) {
  WorkerMediaPlayerFactoryHolder* holder =
      static_cast<WorkerMediaPlayerFactoryHolder*>(
          WorkerMediaPlayerFactoryHolder::From(clients, kSupplementName));
  return holder ? holder->GetFactory() : nullptr;
}

}  // namespace blink
