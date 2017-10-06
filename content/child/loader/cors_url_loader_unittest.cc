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

  bool IsCreateLoaderAndStartCalled() const {
    return is_create_loader_and_start_called_;
  }

  void NotifyClientOnReceiveResponse(ResourceResponseHead head) {
    DCHECK(is_create_loader_and_start_called_);
    client_ptr_->OnReceiveResponse(head, base::nullopt, nullptr);
  }

  void NotifyClientOnReceiveRedirect() {
    DCHECK(is_create_loader_and_start_called_);
    client_ptr_->OnReceiveRedirect(net::RedirectInfo(), ResourceResponseHead());
  }

  void NotifyClientOnComplete(int error_code) {
    DCHECK(is_create_loader_and_start_called_);
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

    is_create_loader_and_start_called_ = true;
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override { NOTREACHED(); }

  bool is_create_loader_and_start_called_ = false;

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

  bool IsOnReceiveResponseCalled() { return is_on_receive_response_called_; }
  bool IsOnReceiveRedirectCalled() { return is_on_receive_redirect_called_; }
  bool IsOnCompleteCalled() { return is_on_complete_called_; }
  bool IsCompletionStatusErrorCodeEqualTo(int code) {
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
  void CreateLoaderAndStart(ResourceRequest& request) {
    CORSURLLoaderFactory::CreateAndBind(
        network_url_loader_factory_.PassInterface(),
        mojo::MakeRequest(&factory_ptr_));
    factory_ptr_->CreateLoaderAndStart(
        mojo::MakeRequest(&url_loader_), 0, 0, 0, request,
        test_client_.CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
    factory_ptr_.FlushForTesting();
  }

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

ResourceRequest CreateResourceRequest(std::string origin,
                                      std::string method,
                                      std::string url) {
  ResourceRequest request;
  request.resource_type = RESOURCE_TYPE_IMAGE;
  request.fetch_request_context_type = REQUEST_CONTEXT_TYPE_IMAGE;
  request.method = method;
  request.url = GURL(url);
  request.request_initiator = url::Origin(GURL(origin));

  return request;
}

ResourceResponseHead CreateResourceResponse() {
  ResourceResponseHead response;
  scoped_refptr<net::HttpResponseHeaders> parsed(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK\n"
                                   "Content-TYPE: text/html; charset=utf-8\n"));
  response.headers = std::move(parsed);

  return response;
}

TEST_F(CORSURLLoaderTest, SameOriginRequest) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://example.com");
  base::RunLoop run_loop;

  CreateLoaderAndStart(request);

  test_factory_.NotifyClientOnReceiveResponse(CreateResourceResponse());
  test_factory_.NotifyClientOnComplete(net::OK);

  run_loop.RunUntilIdle();

  EXPECT_TRUE(test_factory_.IsCreateLoaderAndStartCalled());
  EXPECT_FALSE(test_client_.IsOnReceiveRedirectCalled());
  EXPECT_TRUE(test_client_.IsOnReceiveResponseCalled());
  EXPECT_TRUE(test_client_.IsOnCompleteCalled());
  EXPECT_TRUE(test_client_.IsCompletionStatusErrorCodeEqualTo(net::OK));
}

}  // namespace
}  // namespace content
