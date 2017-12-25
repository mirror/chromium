// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerClients.h"

#include "core/frame/WebLocalFrameImpl.h"
#include "core/loader/WorkerFetchContext.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "public/platform/WebWorkerFetchContext.h"
#include "public/web/WebFrameClient.h"

namespace blink {

template class CORE_TEMPLATE_EXPORT Supplement<WorkerClients>;

WorkerClients* WorkerClients::Create(Document* document) {
  WorkerClients* worker_clients = new WorkerClients;

  // Provide WorkerFetchContext to the worker clients.
  WebLocalFrameImpl* web_frame =
      WebLocalFrameImpl::FromFrame(document->GetFrame());
  std::unique_ptr<WebWorkerFetchContext> web_worker_fetch_context =
      web_frame->Client()->CreateWorkerFetchContext();
  DCHECK(web_worker_fetch_context);
  web_worker_fetch_context->SetApplicationCacheHostID(
      document->Fetcher()->Context().ApplicationCacheHostID());
  web_worker_fetch_context->SetIsOnSubframe(document->GetFrame() !=
                                            document->GetFrame()->Tree().Top());
  ProvideWorkerFetchContextToWorker(worker_clients,
                                    std::move(web_worker_fetch_context));

  return worker_clients;
}

}  // namespace blink
