// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/devtools/devtools_network_transaction_factory.h"

#include <set>
#include <string>
#include <utility>

#include "content/common/devtools/devtools_network_controller.h"
#include "content/common/devtools/devtools_network_transaction.h"
#include "net/base/net_errors.h"
#include "net/http/http_network_layer.h"
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

DevToolsNetworkTransactionFactory::DevToolsNetworkTransactionFactory(
    net::HttpNetworkSession* session)
    : session_(copyParams(base_transaction_factory->GetSession()->params()),
      copyContextForThrottling(
        base_transaction_factory->GetSession()->context(), state))
    , network_layer_(new net::HttpNetworkLayer(session_)) {}

DevToolsNetworkTransactionFactory::~DevToolsNetworkTransactionFactory() {}

int DevToolsNetworkTransactionFactory::CreateTransaction(
    net::RequestPriority priority,
    std::unique_ptr<net::HttpTransaction>* trans) {
  std::unique_ptr<net::HttpTransaction> network_transaction;
  int rv = network_layer_->CreateTransaction(priority, &network_transaction);
  if (rv != net::OK) {
    return rv;
  }
  trans->reset(new DevToolsNetworkTransaction(std::move(network_transaction)));
  return net::OK;
}

net::HttpCache* DevToolsNetworkTransactionFactory::GetCache() {
  return network_layer_->GetCache();
}

net::HttpNetworkSession* DevToolsNetworkTransactionFactory::GetSession() {
  return &session_;
}

}  // namespace content
