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

using OnCompleteCallback =
    base::Callback<void(const ResourceRequestCompletionStatus& status)>;

class TestURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  TestURLLoaderFactory() : binding_(this) {
    binding_.Bind(mojo::MakeRequest(&factory_ptr_));
  }

  mojom::URLLoaderFactoryPtr& factory_ptr() { return factory_ptr_; }
  mojom::URLLoaderClientPtr& client_ptr() { return client_ptr_; }

  int create_loader_and_start_called() const {
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

  int create_loader_and_start_called_ = 0;

  mojo::Binding<mojom::URLLoaderFactory> binding_;
  mojom::URLLoaderFactoryPtr factory_ptr_;
  mojom::URLLoaderClientPtr client_ptr_;
  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

class TestURLLoaderClient : public mojom::URLLoaderClient {
 public:
  TestURLLoaderClient() : binding_(this) {}

  mojom::URLLoaderClientPtr CreateInterfacePtr() {
    mojom::URLLoaderClientPtr client_ptr;
    binding_.Bind(mojo::MakeRequest(&client_ptr));
    return client_ptr;
  }

  int on_received_response_called_ = 0;
  int on_received_redirect_called_ = 0;
  int on_complete_called_ = 0;

  int completion_status_error_code_ = 0;
  std::string completion_status_error_description_;
  OnCompleteCallback on_complete_callback_;

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
    completion_status_error_code_ = status.error_code;
    completion_status_error_description_ = status.error_description_;

    on_complete_called_++;
    if (on_complete_callback_)
      on_complete_callback_.Run(status);
  }

  mojo::Binding<mojom::URLLoaderClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderClient);
};

class CORSURLLoaderTest : public testing::Test {
 public:
  CORSURLLoaderTest() {}

 protected:
  void CreateLoaderAndStart(ResourceRequest& request) {
    CORSURLLoader::CreateAndStart(&factory_, mojo::MakeRequest(&url_loader), 0,
                                  0, 0, request, client_.CreateInterfacePtr(),
                                  TRAFFIC_ANNOTATION_FOR_TESTS);
    factory_.factory_ptr().FlushForTesting();
  }

  struct StatusCheckBuilder {
    StatusCheckBuilder(TestURLLoaderFactory& factory,
                       TestURLLoaderClient& client)
        : factory_(factory), client_(client){};

    StatusCheckBuilder* FactoryNotCalled() {
      EXPECT_EQ(0, factory_.create_loader_and_start_called());
      return this;
    }

    StatusCheckBuilder* FactoryCalledOnce() {
      EXPECT_EQ(1, factory_.create_loader_and_start_called());
      return this;
    }

    StatusCheckBuilder* NoRedirect() {
      EXPECT_EQ(0, client_.on_received_redirect_called_);
      return this;
    }

    StatusCheckBuilder* NoResponse() {
      EXPECT_EQ(0, client_.on_received_response_called_);
      return this;
    }

    StatusCheckBuilder* SingleResponse() {
      EXPECT_EQ(1, client_.on_received_response_called_);
      return this;
    }

    StatusCheckBuilder* SingleCompleteWithoutError() {
      EXPECT_EQ(1, client_.on_complete_called_);
      EXPECT_EQ(0, client_.completion_status_error_code_);
      return this;
    }

    StatusCheckBuilder* SingleCompleteWithError(int error) {
      EXPECT_EQ(1, client_.on_complete_called_);
      EXPECT_EQ(error, client_.completion_status_error_code_);
      return this;
    }

    TestURLLoaderFactory& factory_;
    TestURLLoaderClient& client_;
  };

  StatusCheckBuilder Expect() { return StatusCheckBuilder(factory_, client_); }

  // Be the first member so it is destroyed last.
  base::MessageLoop message_loop_;

  TestURLLoaderFactory factory_;
  TestURLLoaderClient client_;

  mojom::URLLoaderAssociatedPtr url_loader;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoaderTest);
};

OnCompleteCallback CreateOnCompleteCallback(base::RunLoop& run_loop) {
  return base::Bind(
      [](const base::Closure& quit_closure,
         const ResourceRequestCompletionStatus& status) {
        // FIXME:
        quit_closure.Run();
      },
      run_loop.QuitClosure());
}

ResourceRequest CreateResourceRequest(std::string origin,
                                      std::string method,
                                      std::string url) {
  ResourceRequest request;
  request.resource_type = RESOURCE_TYPE_IMAGE;
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
  ResourceRequest request = CreateResourceRequest("http://example.com/", "GET",
                                                  "http://example.com/");

  ResourceResponseHead response = CreateResourceResponse();

  base::RunLoop run_loop;
  client_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  factory_.NotifyClientOnReceiveResponse(response);
  factory_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  Expect()
      .FactoryCalledOnce()
      ->NoRedirect()
      ->SingleResponse()
      ->SingleCompleteWithoutError();
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestWithFetchRequestModeSameOrigin) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com/", "GET", "http://other.com/");
  request.fetch_request_mode = FETCH_REQUEST_MODE_SAME_ORIGIN;

  ResourceResponseHead response = CreateResourceResponse();

  base::RunLoop run_loop;
  client_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  // This call never hits the network URLLoader (i.e. the TestURLLoaderFactory)
  // because it is fails right away.

  Expect()
      .FactoryNotCalled()
      ->NoRedirect()
      ->NoResponse()
      ->SingleCompleteWithError(net::ERR_ACCESS_DENIED);
}

}  // namespace
}  // namespace content
