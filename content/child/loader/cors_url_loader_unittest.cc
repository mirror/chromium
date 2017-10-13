// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/loader/cors_url_loader.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/child/loader/cors_url_loader_factory.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

class TestURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  TestURLLoaderFactory() : binding_(this) {
    binding_.Bind(mojo::MakeRequest(&factory_ptr_));
  }
  ~TestURLLoaderFactory() override = default;

  void NotifyClientOnReceiveResponse() {
    DCHECK(client_ptr_);
    ResourceResponseHead response;
    response.headers = new net::HttpResponseHeaders(
        "HTTP/1.1 200 OK\n"
        "Content-TYPE: text/html; charset=utf-8\n");

    client_ptr_->OnReceiveResponse(response, base::nullopt, nullptr);
  }

  void NotifyClientOnComplete(int error_code) {
    DCHECK(client_ptr_);
    ResourceRequestCompletionStatus data;
    data.error_code = error_code;
    client_ptr_->OnComplete(data);
  }

 private:
  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    DCHECK(client);
    client_ptr_ = std::move(client);
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override { NOTREACHED(); }

  mojo::Binding<mojom::URLLoaderFactory> binding_;
  mojom::URLLoaderFactoryPtr factory_ptr_;
  mojom::URLLoaderClientPtr client_ptr_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

class TestURLLoaderClient : public mojom::URLLoaderClient {
 public:
  TestURLLoaderClient() : binding_(this) {}

  ~TestURLLoaderClient() override = default;

  mojom::URLLoaderClientPtr CreateInterfacePtr() {
    mojom::URLLoaderClientPtr client_ptr;
    binding_.Bind(mojo::MakeRequest(&client_ptr));
    return client_ptr;
  }

  bool IsOnReceiveResponseCalled() const {
    return is_on_receive_response_called_;
  }
  bool IsOnReceiveRedirectCalled() const {
    return is_on_receive_redirect_called_;
  }
  bool IsOnCompleteCalled() const { return is_on_complete_called_; }
  bool IsCompletionStatusErrorCodeEqualTo(int code) const {
    return completion_status_error_code_ == code;
  }

 private:
  // mojom::URLLoaderClient implementation:
  void OnReceiveResponse(
      const ResourceResponseHead& response_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      mojom::DownloadedTempFilePtr downloaded_file) override {
    is_on_receive_response_called_ = true;
  }
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& response_head) override {
    is_on_receive_redirect_called_ = true;
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
    completion_status_error_code_ = status.error_code;
    is_on_complete_called_ = true;
  }

  bool is_on_receive_response_called_ = false;
  bool is_on_receive_redirect_called_ = false;
  bool is_on_complete_called_ = false;
  int completion_status_error_code_ = 0;

  mojo::Binding<mojom::URLLoaderClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderClient);
};

class CORSURLLoaderTest : public testing::Test {
 public:
  CORSURLLoaderTest() : network_factory_binding_(&test_factory_) {
    network_factory_binding_.Bind(mojo::MakeRequest(&network_factory_ptr_));
    network_url_loader_factory_ = std::move(network_factory_ptr_);
  }

 protected:
  void CreateLoaderAndStart(const std::string& origin, const std::string& url) {
    CORSURLLoaderFactory::CreateAndBind(
        network_url_loader_factory_.PassInterface(),
        mojo::MakeRequest(&factory_ptr_));

    ResourceRequest request;
    request.resource_type = RESOURCE_TYPE_IMAGE;
    request.fetch_request_context_type = REQUEST_CONTEXT_TYPE_IMAGE;
    request.method = net::HttpRequestHeaders::kGetMethod;
    request.url = GURL(url);
    request.request_initiator = url::Origin(GURL(origin));

    factory_ptr_->CreateLoaderAndStart(
        mojo::MakeRequest(&url_loader_), 0, 0, 0, request,
        test_client_.CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
    factory_ptr_.FlushForTesting();
  }

  void RunUntilIdle() {
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
  }

  void NotifyClientOnReceiveResponse() {
    test_factory_.NotifyClientOnReceiveResponse();
  }

  void NotifyClientOnComplete(int error_code) {
    test_factory_.NotifyClientOnComplete(error_code);
  }

  const TestURLLoaderClient& client() { return test_client_; }

 private:
  // Be the first member so it is destroyed last.
  base::MessageLoop message_loop_;

  TestURLLoaderFactory test_factory_;
  TestURLLoaderClient test_client_;

  mojom::URLLoaderFactoryPtr network_factory_ptr_;
  mojo::Binding<mojom::URLLoaderFactory> network_factory_binding_;

  mojom::URLLoaderPtr url_loader_;
  mojom::URLLoaderFactoryPtr factory_ptr_;

  PossiblyAssociatedInterfacePtr<mojom::URLLoaderFactory>
      network_url_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoaderTest);
};

TEST_F(CORSURLLoaderTest, SameOriginRequest) {
  const std::string kOriginOrUrl("http://example.com");
  CreateLoaderAndStart(kOriginOrUrl, kOriginOrUrl);

  NotifyClientOnReceiveResponse();
  NotifyClientOnComplete(net::OK);

  RunUntilIdle();

  EXPECT_FALSE(client().IsOnReceiveRedirectCalled());
  EXPECT_TRUE(client().IsOnReceiveResponseCalled());
  EXPECT_TRUE(client().IsOnCompleteCalled());
  EXPECT_TRUE(client().IsCompletionStatusErrorCodeEqualTo(net::OK));
}

}  // namespace
}  // namespace content
