// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIMPLE_URL_LOADER_SIMPLE_URL_LOADER_H_
#define COMPONENTS_SIMPLE_URL_LOADER_SIMPLE_URL_LOADER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace content {

struct ResourceRequest;
struct ResourceResponseHead;

namespace mojom {
class URLLoaderFactory;
}

}  // namespace content

namespace simple_url_loader {

// Creates and wraps a URLLoader, for consumers with basic needs. Consumers with
// more involved requirements should create and manage a URLLoader themselves.
//
// Deleting a SimpleURLLoader before it completes cancels the requests and frees
// any resources it is using (including any partially downloaded files).
//
// Each SimpleURLLoader can only be used for a single request.
//
// TODO(mmenke): Support the following:
// * Save to (temp) file.
// * Consumer-provided methods to deal with read data (with backpressure).
// * Monitoring (And cancelling during) redirects.
// * Uploads (Fixed strings, files, data streams (with backpressure), chunked
// uploads). ResourceRequest may already have some support, but should make it
// simple.
// * Retrying.
class SimpleURLLoader : public content::mojom::URLLoaderClient {
 public:
  // The maximum size DownloadToString will accept.
  const size_t kMaxBoundedStringDownloadSize = 1024 * 1024;

  // Callback used when downloading the response body as a std::string.
  // |simple_url_loader| is the loader associated with the request, and
  // |response_body| is the body of the response, or nullptr on failure.
  using BodyAsStringCallback =
      base::OnceCallback<void(SimpleURLLoader* simple_url_loader,
                              std::unique_ptr<std::string> response_body)>;

  explicit SimpleURLLoader();
  ~SimpleURLLoader() override;

  // Starts a request for |resource_request| using |network_context|.
  void DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      const content::ResourceRequest& resource_request,
      content::mojom::URLLoaderFactory* url_loader_factory,
      const net::NetworkTrafficAnnotationTag& annotation_tag,
      BodyAsStringCallback body_as_string_callback);

  // Downloads to a string of bounded size. If |max_body_size| is exceeded, the
  // request will fail without providing any data. |max_body_size| must be no
  // greater than 1 MiB. For anything larger, it's recommended to either save to
  // a temp file, or consume the data as it is received.
  // TODO(mmenke):  Support saving to a temp file or consuming data as it is
  // received.
  void DownloadToString(const content::ResourceRequest& resource_request,
                        content::mojom::URLLoaderFactory* url_loader_factory,
                        const net::NetworkTrafficAnnotationTag& annotation_tag,
                        BodyAsStringCallback body_as_string_callback,
                        size_t max_body_size);

  // Sets whether partially received results are allowed. Defaults to false.
  // When true, if an error is received or the max allowed body size exceeded,
  // an empty string will be returned. This covers net errors that truncate the
  // response bodies, as well as cases where the response body it too large.
  void set_allow_partial_results(bool allow_partial_results) {
    allow_partial_results_ = allow_partial_results;
  }

  // Sets whether bodies of non-2xx responses are allowed. When false, if a
  // non-2xx result is returned (Except for redirects), the request will fail
  // with net::FAILED without waiting to read the response body, though headers
  // will be readable.
  //
  // When true, non-2xx responses are treated no differently than other
  // responses, so when they complete, net_error() will return net::OK, if no
  // other problem occurs.
  //
  // Defaults to false.
  // TODO(mmenke): Consider adding a new error code for this.
  void set_allow_http_error_results(bool allow_http_error_results) {
    allow_http_error_results_ = allow_http_error_results;
  }

  // Returns the net::Error representing the final status of the request. May
  // only be called once the loader has informed the caller on completion.
  int net_error() const;

  // The ResourceResponseHead for the request. Will by nullptr if ResponseInfo
  // has been received.
  const content::ResourceResponseHead* response_info() const {
    return response_info_.get();
  }

 protected:
  bool allow_partial_results() const { return allow_partial_results_; }

 private:
  // Class to handle consuming the response body as it's received.
  class BodyHandler;
  class SaveToStringBodyHandler;

  void StartInternal(const content::ResourceRequest& resource_request,
                     content::mojom::URLLoaderFactory* url_loader_factory,
                     const net::NetworkTrafficAnnotationTag& annotation_tag);

  // content::mojom::URLLoaderClient implementation;
  void OnReceiveResponse(
      const content::ResourceResponseHead& response_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      content::mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(
      const net::RedirectInfo& redirect_info,
      const content::ResourceResponseHead& response_head) override;
  void OnDataDownloaded(int64_t data_length, int64_t encoded_length) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(
      const content::ResourceRequestCompletionStatus& status) override;

  void OnConnectionError();

  // Called by BodyHandler when the body pipe was closed. This could indicate an
  // error, concellation, or completion. To determine which case this is, the
  // size must be compared to the size received by
  // ResourceRequestCompletionStatus, if ResourceRequestCompletionStatus
  // indicates a success.
  void OnBodyComplete(int64_t received_body_size);

  // Completes the request if OnComplete() and either
  // OnStartLoadingResponseBody() was never called, or OnBodyComplete() has been
  // called.
  void MaybeComplete();

  // Finished the request with the provided error code, after freeing Mojo
  // resources.
  void FinishWithResult(int net_error);

  bool allow_partial_results_ = false;
  bool allow_http_error_results_ = false;

  content::mojom::URLLoaderPtr url_loader_;
  mojo::Binding<content::mojom::URLLoaderClient> binding_;
  std::unique_ptr<BodyHandler> body_handler_;

  bool request_completed_ = false;
  int request_net_error_ = net::OK;
  // The expected total size of the body, taken from
  // ResourceRequestCompletionStatus.
  int64_t expected_body_size_ = 0;

  bool body_started_ = false;
  bool body_completed_ = false;
  // Final size of the body. Set once the body's Mojo pipe has been closed.
  int64_t received_body_size_ = 0;

  int result_net_error_ = net::ERR_IO_PENDING;

  std::unique_ptr<content::ResourceResponseHead> response_info_;

  DISALLOW_COPY_AND_ASSIGN(SimpleURLLoader);
};

}  // namespace simple_url_loader

#endif  // COMPONENTS_SIMPLE_URL_LOADER_SIMPLE_URL_LOADER_H_
