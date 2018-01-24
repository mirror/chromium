// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_

#include <string>

#include "base/callback.h"
#include "mojo/common/data_pipe_drainer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "url/gurl.h"

namespace mojo {
class StringDataPipeProducer;
}  // namespace mojo

namespace network {
struct ResourceResponseHead;
}  // namespace network

namespace content {

class SignedExchangeHandler : public mojo::common::DataPipeDrainer::Client {
 public:
  using ExchangeFoundCallback =
      base::OnceCallback<void(const GURL& request_url,
                              const network::ResourceResponseHead&,
                              mojo::ScopedDataPipeConsumerHandle body)>;
  using ExchangeFinishedCallback = base::OnceCallback<void(int)>;

  explicit SignedExchangeHandler(mojo::ScopedDataPipeConsumerHandle body);
  ~SignedExchangeHandler() override;
  void GetHTTPExchange(ExchangeFoundCallback found_callback,
                       ExchangeFinishedCallback finish_callback);

 private:
  // mojo::Common::DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

  void OnDataWritten(MojoResult result);

  mojo::ScopedDataPipeConsumerHandle body_;
  std::unique_ptr<mojo::common::DataPipeDrainer> drainer_;
  ExchangeFoundCallback found_callback_;
  ExchangeFinishedCallback finish_callback_;
  std::string pending_data_;
  std::unique_ptr<mojo::StringDataPipeProducer> data_producer_;

  base::WeakPtrFactory<SignedExchangeHandler> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(SignedExchangeHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_SIGNED_EXCHANGE_HANDLER_H_
