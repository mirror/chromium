// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_http_transaction_factory.h"

#include "net/http/http_network_transaction.h"

namespace content {

net::HttpNetworkSession::Params copyParams(
    const net::HttpNetworkSession::Params& params) {
  return net::HttpNetworkSession::Params(params);
}

net::HttpNetworkSession::Context copyContextForThrottling(
    const net::HttpNetworkSession::Context& context,
    scoped_refptr<DevToolsURLRequestInterceptor::State> state) {
  auto copiedContext = net::HttpNetworkSession::Context(context);
  copiedContext.client_socket_factory = new DevtoolsClientSocketFactory(
    state, net::ClientSocketFactory::GetDefaultFactory());
  return copiedContext;
}

DevtoolsHttpTransactionFactory::DevtoolsHttpTransactionFactory(
scoped_refptr<DevToolsURLRequestInterceptor::State> state,
net::HttpTransactionFactory* base_transaction_factory)
: client_socket_factory_(state,
    net::ClientSocketFactory::GetDefaultFactory())
, session_(copyParams(base_transaction_factory->GetSession()->params()),
    copyContextForThrottling(
        base_transaction_factory->GetSession()->context(), state))
//, base_transaction_factory_(base_transaction_factory)
{

}
DevtoolsHttpTransactionFactory::~DevtoolsHttpTransactionFactory() {}

// Creates a HttpTransaction object. On success, saves the new
// transaction to |*trans| and returns OK.
int DevtoolsHttpTransactionFactory::CreateTransaction(net::RequestPriority priority,
                    std::unique_ptr<net::HttpTransaction>* trans) {
  //TODO(allada) suspended?
  trans->reset(new net::HttpNetworkTransaction(priority, GetSession()));
  return net::OK;
};

// Returns the associated cache if any (may be NULL).
net::HttpCache* DevtoolsHttpTransactionFactory::GetCache() {
  return nullptr;
};

// Returns the associated HttpNetworkSession used by new transactions.
net::HttpNetworkSession* DevtoolsHttpTransactionFactory::GetSession() {
  return &session_;
};

DevtoolsClientSocketFactory* DevtoolsHttpTransactionFactory::GetOrCreateClientSocketFactory(
  net::ClientSocketFactory* base_client_stream_factory) {
  return &client_socket_factory_;
}

} // namespace content
