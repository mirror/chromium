// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/shared_worker/shared_worker_repository.h"

#include "content/child/webmessageportchannel_impl.h"
#include "content/common/view_messages.h"
#include "content/renderer/render_frame_impl.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/web/WebSharedWorkerConnectListener.h"

namespace content {

SharedWorkerRepository::SharedWorkerRepository(
    service_manager::InterfaceProvider* interface_provider)
    : interface_provider_(interface_provider) {}

SharedWorkerRepository::~SharedWorkerRepository() = default;

void SharedWorkerRepository::Connect(
    const blink::WebURL& url,
    const blink::WebString& name,
    DocumentID document_id,
    const blink::WebString& content_security_policy,
    blink::WebContentSecurityPolicyType content_security_policy_type,
    blink::WebAddressSpace creation_address_space,
    blink::mojom::SharedWorkerCreationContextType creation_context_type,
    bool data_saver_enabled,
    std::unique_ptr<blink::WebMessagePortChannel> channel,
    std::unique_ptr<blink::WebSharedWorkerConnectListener> listener) {
  // Lazy bind the connector.
  if (!connector_)
    interface_provider_->GetInterface(mojo::MakeRequest(&connector_));

  mojom::SharedWorkerInfoPtr info(mojom::SharedWorkerInfo::New(
      url, name.Utf8(), content_security_policy.Utf8(),
      content_security_policy_type, creation_address_space,
      creation_context_type, data_saver_enabled));

  mojom::SharedWorkerClientPtr client;
  AddWorker(document_id,
            mojo::MakeStrongBinding(
                std::make_unique<SharedWorkerClientImpl>(std::move(listener)),
                mojo::MakeRequest(&client)));

  connector_->Connect(std::move(info), std::move(client),
                      static_cast<WebMessagePortChannelImpl*>(channel.get())
                          ->ReleaseMessagePort()
                          .ReleaseHandle());
}

void SharedWorkerRepository::DocumentDetached(DocumentID document_id) {
  // Delete any associated SharedWorkerClientImpls, which will signal, via the
  // dropped mojo connection, disinterest in the associated shared worker.
  auto iter = worker_map_.find(document_id);
  if (iter != worker_map_.end()) {
    for (auto client : iter->second) {
      if (client)
        client->Close();
    }
    worker_map_.erase(iter);
  }
}

void SharedWorkerRepository::AddWorker(
    DocumentID document_id,
    mojo::StrongBindingPtr<mojom::SharedWorkerClient> client) {
#if 0
  // XXX Watch for errors on the client handle to cleanup map.
  client->set_connection_error_handler(
      base::Bind(&SharedWorkerRepository::OnWorkerClosed,
                 base::Unretained(this),
                 document_id,
                 client));
#endif

  std::pair<WorkerMap::iterator, bool> iter =
      worker_map_.insert(std::make_pair(document_id, WorkerList()));
  iter.first->second.push_back(client);
}

#if 0
void SharedWorkerRepository::OnWorkerClosed(
    DocumentID document_id,
    mojo::StrongBindingPtr<mojom::SharedWorkerClient> client) {
  // The client is about to be destroyed, garbage collect it from
  // the set.
  worker_map_[document_id]
}
#endif

}  // namespace content
