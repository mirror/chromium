// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_HTTP_TRANSACTION_FACTORY_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_HTTP_TRANSACTION_FACTORY_

#include "net/base/request_priority.h"
#include "net/http/http_transaction_factory.h"
#include "net/http/http_network_session.h"
#include "content/browser/devtools/devtools_client_socket_factory.h"

namespace content {

class DevtoolsHttpTransactionFactory : public net::HttpTransactionFactory {
public:
  DevtoolsHttpTransactionFactory(
    scoped_refptr<DevToolsURLRequestInterceptor::State> state,
    net::HttpTransactionFactory* base_transaction_factory);
  ~DevtoolsHttpTransactionFactory() override;

  // Creates a HttpTransaction object. On success, saves the new
  // transaction to |*trans| and returns OK.
  int CreateTransaction(net::RequestPriority priority,
                        std::unique_ptr<net::HttpTransaction>* trans) override;

  // Returns the associated cache if any (may be NULL).
  net::HttpCache* GetCache() override;

  // Returns the associated HttpNetworkSession used by new transactions.
  net::HttpNetworkSession* GetSession() override;

  DevtoolsClientSocketFactory* GetOrCreateClientSocketFactory(
      net::ClientSocketFactory* base_client_stream_factory);

private:
  DevtoolsClientSocketFactory client_socket_factory_;
  net::HttpNetworkSession session_;

  //net::HttpTransactionFactory* base_transaction_factory_;

};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_HTTP_TRANSACTION_FACTORY_
