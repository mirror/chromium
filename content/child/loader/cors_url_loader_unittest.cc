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
#include "mojo/public/cpp/bindings/strong_binding.h"
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

  ~TestURLLoaderFactory() override = default;

  mojom::URLLoaderFactoryPtr& factory_ptr() { return factory_ptr_; }
  mojom::URLLoaderClientPtr& client_ptr() { return client_ptr_; }

  int create_loader_and_start_called() const {
    return create_loader_and_start_called_;
  }

  void NotifyClientOnReceiveResponse(ResourceResponseHead head) {
    DCHECK(create_loader_and_start_called_);

    client_ptr_->OnReceiveResponse(head, base::nullopt, nullptr);
  }

  void NotifyClientOnReceiveRedirect() {
    DCHECK(create_loader_and_start_called_);

    client_ptr_->OnReceiveRedirect(net::RedirectInfo(), ResourceResponseHead());
  }

  void NotifyClientOnComplete(int error_code) {
    DCHECK(create_loader_and_start_called_);

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
    create_loader_and_start_called_++;
    client_ptr_ = std::move(client);

    DCHECK(client_ptr_);
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

  ~TestURLLoaderClient() override = default;

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
    completion_status_error_description_ = status.error_description;

    on_complete_called_++;
    if (on_complete_callback_)
      on_complete_callback_.Run(status);
  }

  mojo::Binding<mojom::URLLoaderClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderClient);
};

class CORSURLLoaderTest : public testing::Test {
 public:
  CORSURLLoaderTest() : network_factory_binding_(&network_simulator_) {
    network_factory_binding_.Bind(mojo::MakeRequest(&network_factory_ptr_));
    network_url_loader_factory_ = std::move(network_factory_ptr_);
  }

 protected:
  void CreateLoaderAndStart(ResourceRequest& request) {
    CORSURLLoaderFactory::CreateAndBind(std::move(network_url_loader_factory_),
                                        mojo::MakeRequest(&factory_ptr_));
    factory_ptr_->CreateLoaderAndStart(
        mojo::MakeRequest(&url_loader_), 0, 0, 0, request,
        client_simulator_.CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
    factory_ptr_.FlushForTesting();
  }

  struct StatusCheckBuilder {
    StatusCheckBuilder(TestURLLoaderFactory& factory,
                       TestURLLoaderClient& client)
        : factory_(factory), client_(client){};

    StatusCheckBuilder& NetworkNotHit() {
      EXPECT_EQ(0, factory_.create_loader_and_start_called());
      return *this;
    }

    StatusCheckBuilder& NetworkHitOnce() {
      EXPECT_EQ(1, factory_.create_loader_and_start_called());
      return *this;
    }

    StatusCheckBuilder& NoRedirect() {
      EXPECT_EQ(0, client_.on_received_redirect_called_);
      return *this;
    }

    StatusCheckBuilder& NoResponse() {
      EXPECT_EQ(0, client_.on_received_response_called_);
      return *this;
    }

    StatusCheckBuilder& Response() {
      EXPECT_EQ(1, client_.on_received_response_called_);
      return *this;
    }

    StatusCheckBuilder& CompleteWithoutError() {
      EXPECT_EQ(1, client_.on_complete_called_);
      EXPECT_EQ(0, client_.completion_status_error_code_);
      EXPECT_EQ("", client_.completion_status_error_description_);
      return *this;
    }

    StatusCheckBuilder& CompleteWithError(int error,
                                          std::string error_description) {
      EXPECT_EQ(1, client_.on_complete_called_);
      EXPECT_EQ(error, client_.completion_status_error_code_);
      EXPECT_EQ(error_description,
                client_.completion_status_error_description_);

      return *this;
    }

    TestURLLoaderFactory& factory_;
    TestURLLoaderClient& client_;
  };

  StatusCheckBuilder Expect() {
    return StatusCheckBuilder(network_simulator_, client_simulator_);
  }

  // Be the first member so it is destroyed last.
  base::MessageLoop message_loop_;

  TestURLLoaderFactory network_simulator_;
  TestURLLoaderClient client_simulator_;

  mojom::URLLoaderFactoryPtr network_factory_ptr_;
  mojo::Binding<mojom::URLLoaderFactory> network_factory_binding_;

  mojom::URLLoaderPtr url_loader_;
  mojom::URLLoaderFactoryPtr factory_ptr_;

  PossiblyAssociatedInterfacePtr<mojom::URLLoaderFactory>
      network_url_loader_factory_;

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
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://example.com");
  base::RunLoop run_loop;
  client_simulator_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  network_simulator_.NotifyClientOnReceiveResponse(CreateResourceResponse());
  network_simulator_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  Expect().NetworkHitOnce().NoRedirect().Response().CompleteWithoutError();
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestFetchRequestModeSameOrigin) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://other.com/");
  request.fetch_request_mode = FETCH_REQUEST_MODE_SAME_ORIGIN;

  base::RunLoop run_loop;
  client_simulator_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  // This call never hits the network URLLoader (i.e. the TestURLLoaderFactory)
  // because it is fails right away.

  Expect().NetworkNotHit().NoRedirect().NoResponse().CompleteWithError(
      net::ERR_ACCESS_DENIED, "Cross origin requests are not supported.");
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestFetchRequestModeNoCORS) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://other.com/");
  request.fetch_request_mode = FETCH_REQUEST_MODE_NO_CORS;

  base::RunLoop run_loop;
  client_simulator_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  network_simulator_.NotifyClientOnReceiveResponse(CreateResourceResponse());
  network_simulator_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  Expect().NetworkHitOnce().NoRedirect().Response().CompleteWithoutError();
}

TEST_F(CORSURLLoaderTest, CrossOriginRequestFetchRequestModeCORSNoCORSHeader) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://other.com/");
  request.fetch_request_mode = FETCH_REQUEST_MODE_CORS;

  base::RunLoop run_loop;
  client_simulator_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  network_simulator_.NotifyClientOnReceiveResponse(CreateResourceResponse());
  network_simulator_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  Expect().NetworkHitOnce().NoRedirect().NoResponse().CompleteWithError(
      net::ERR_ACCESS_DENIED,
      "No 'Access-Control-Allow-Origin' header is present on the requested "
      "resource. Origin 'http://example.com' is therefore not allowed access.");
}

TEST_F(CORSURLLoaderTest,
       CrossOriginRequestFetchRequestModeCORSWithCORSHeader) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://other.com/");
  request.fetch_request_mode = FETCH_REQUEST_MODE_CORS;

  base::RunLoop run_loop;
  client_simulator_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  ResourceResponseHead response = CreateResourceResponse();
  response.headers->AddHeader(
      "Access-Control-Allow-Origin: http://example.com");

  network_simulator_.NotifyClientOnReceiveResponse(response);
  network_simulator_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  Expect().NetworkHitOnce().NoRedirect().Response().CompleteWithoutError();
}

TEST_F(CORSURLLoaderTest,
       CrossOriginRequestFetchRequestModeCORSWithCORSHeaderMismatch) {
  ResourceRequest request =
      CreateResourceRequest("http://example.com", "GET", "http://other.com/");
  request.fetch_request_mode = FETCH_REQUEST_MODE_CORS;

  base::RunLoop run_loop;
  client_simulator_.on_complete_callback_ = CreateOnCompleteCallback(run_loop);

  CreateLoaderAndStart(request);

  ResourceResponseHead response = CreateResourceResponse();
  response.headers->AddHeader(
      "Access-Control-Allow-Origin: http://some-other-domain.com/");

  network_simulator_.NotifyClientOnReceiveResponse(response);
  network_simulator_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  Expect().NetworkHitOnce().NoRedirect().NoResponse().CompleteWithError(
      net::ERR_ACCESS_DENIED,
      "The 'Access-Control-Allow-Origin' header has a value "
      "'http://some-other-domain.com/' that is not equal to the supplied "
      "origin. Origin 'http://example.com' is therefore not allowed access.");
}

}  // namespace
}  // namespace content
