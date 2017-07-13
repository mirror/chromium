// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cors_url_loader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/common/throttling_url_loader.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class TestURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  TestURLLoaderFactory() : binding_(this) {
    binding_.Bind(mojo::MakeRequest(&factory_ptr_));
  }

  mojom::URLLoaderFactoryPtr& factory_ptr() { return factory_ptr_; }
  mojom::URLLoaderClientPtr& client_ptr() { return client_ptr_; }

  size_t create_loader_and_start_called() const {
    return create_loader_and_start_called_;
  }

  void NotifyClientOnReceiveResponse(ResourceResponseHead& head) {
    client_ptr_->OnReceiveResponse(head, base::nullopt, nullptr);
  }

  void NotifyClientOnReceiveRedirect() {
    client_ptr_->OnReceiveRedirect(net::RedirectInfo(), ResourceResponseHead());
  }

  void NotifyClientOnComplete(int error_code) {
    ResourceRequestCompletionStatus data;
    data.error_code = error_code;
    client_ptr_->OnComplete(data);
  }

 private:
  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderAssociatedRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    create_loader_and_start_called_++;

    client_ptr_ = std::move(client);
  }

  void SyncLoad(int32_t routing_id,
                int32_t request_id,
                const ResourceRequest& request,
                SyncLoadCallback callback) override {
    NOTREACHED();
  }

  size_t create_loader_and_start_called_ = 0;

  mojo::Binding<mojom::URLLoaderFactory> binding_;
  mojom::URLLoaderFactoryPtr factory_ptr_;
  mojom::URLLoaderClientPtr client_ptr_;
  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

class TestURLLoaderClient : public mojom::URLLoaderClient {
 public:
  TestURLLoaderClient() : binding_(this) {}

  size_t on_received_response_called() const {
    return on_received_response_called_;
  }

  size_t on_received_redirect_called() const {
    return on_received_redirect_called_;
  }

  size_t on_complete_called() const { return on_complete_called_; }

  using OnCompleteCallback =
      base::Callback<void(const ResourceRequestCompletionStatus& status)>;
  void set_on_complete_callback(const OnCompleteCallback& callback) {
    on_complete_callback_ = callback;
  }

  mojom::URLLoaderClientPtr CreateInterfacePtr() {
    mojom::URLLoaderClientPtr client_ptr;
    binding_.Bind(mojo::MakeRequest(&client_ptr));
    return client_ptr;
  }

 private:
  // mojom::URLLoaderClient implementation:
  void OnReceiveResponse(
      const ResourceResponseHead& response_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      mojom::DownloadedTempFilePtr downloaded_file) override {
    on_received_response_called_++;
  }
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& response_head) override {
    on_received_redirect_called_++;
  }
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override {}
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override {}
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {}
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {}
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {}
  void OnComplete(const ResourceRequestCompletionStatus& status) override {
    on_complete_called_++;
    if (on_complete_callback_)
      on_complete_callback_.Run(status);
  }

  size_t on_received_response_called_ = 0;
  size_t on_received_redirect_called_ = 0;
  size_t on_complete_called_ = 0;

  mojo::Binding<mojom::URLLoaderClient> binding_;
  OnCompleteCallback on_complete_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderClient);
};

class CORSURLLoaderTest : public testing::Test {
 public:
  CORSURLLoaderTest() {}

 protected:
  // testing::Test implementation.
  void SetUp() override {}

  void CreateLoaderAndStart(ResourceRequest& request) {
    CORSURLLoader::CreateAndStart(&factory_, mojo::MakeRequest(&url_loader), 0,
                                  0, 0, request, client_.CreateInterfacePtr(),
                                  TRAFFIC_ANNOTATION_FOR_TESTS);
    factory_.factory_ptr().FlushForTesting();
  }

  // Be the first member so it is destroyed last.
  base::MessageLoop message_loop_;

  //  std::unique_ptr<ThrottlingURLLoader> loader_;

  TestURLLoaderFactory factory_;
  TestURLLoaderClient client_;

  mojom::URLLoaderAssociatedPtr url_loader;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoaderTest);
};

TEST_F(CORSURLLoaderTest, SameOriginRequest) {
  ResourceRequest request;
  request.method = "GET";
  request.url = GURL("http://www.example.com/");
  request.request_initiator = url::Origin(GURL("http://www.example.com/"));

  ResourceResponseHead response;
  scoped_refptr<net::HttpResponseHeaders> parsed(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK\n"
                                   "Content-TYPE: text/html; charset=utf-8\n"));
  response.headers = std::move(parsed);

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure,
         const ResourceRequestCompletionStatus& status) {
        EXPECT_EQ("", status.error_description_);
        EXPECT_EQ(0, status.error_code);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart(request);
  factory_.NotifyClientOnReceiveResponse(response);
  factory_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  EXPECT_EQ(1u, factory_.create_loader_and_start_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestNoCORS) {
  ResourceRequest request;
  request.method = "DELETE";
  request.url = GURL("http://www.example.com/");
  request.request_initiator = url::Origin(GURL("http://www.domain.de/"));

  ResourceResponseHead response;
  scoped_refptr<net::HttpResponseHeaders> parsed(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK\n"
                                   "Content-TYPE: text/html; charset=utf-8\n"));
  response.headers = std::move(parsed);

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure,
         const ResourceRequestCompletionStatus& status) {
        EXPECT_EQ("", status.error_description_);
        EXPECT_EQ(0, status.error_code);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart(request);
  factory_.NotifyClientOnReceiveResponse(response);
  factory_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  EXPECT_EQ(1u, factory_.create_loader_and_start_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

}  // namespace
}  // namespace content
