// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LOADER_SIMPLE_URL_LOADER_H_
#define COMPONENTS_LOADER_SIMPLE_URL_LOADER_H_

#include <stdint.h>

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

namespace loader {

// Creates and wraps a URLLoader, and runs it to completion. It's recommended
// that consumers use this class instead of URLLoader directly, due to the
// complexity of the API.
//
// Deleting a SimpleURLLoader before it completes cancels the requests and frees
// any resources it is using (including any partially downloaded files).
//
// Each SimpleURLLoader can only be used for a single request.
//
// TODO(mmenke): Support the following:
// * Save to (temp) file.
// * Consumer-provided methods to receive streaming (with backpressure).
// * Monitoring (And cancelling during) redirects.
// * Uploads (Fixed strings, files, data streams (with backpressure), chunked
// uploads). ResourceRequest may already have some support, but should make it
// simple.
// * Retrying.
class SimpleURLLoader final : public content::mojom::URLLoaderClient {
 public:
  // The maximum size DownloadToString will accept.
  const size_t kMaxBoundedStringDownloadSize = 1024 * 1024;

  // Callback used when downloading the response body as a std::string.
  // |simple_url_loader| is the loader associated with the request, and
  // |response_body| is the body of the response, or nullptr on failure.
  using BodyAsStringCallback =
      base::OnceCallback<void(std::unique_ptr<std::string> response_body)>;

  SimpleURLLoader();
  ~SimpleURLLoader() override;

  // Starts a request for |resource_request| using |network_context|. The
  // SimpleURLLoader will accumulate all downloaded data in an in-memory string
  // of bounded size. If |max_body_size| is exceeded, the request will fail
  // without providing any body data. |max_body_size| must be no greater than 1
  // MiB. For anything larger, it's recommended to either save to a temp file,
  // or consume the data as it is received.
  //
  // Whether the request succeeds or fails (Or the body exceeds
  // |max_body_size|), |body_as_string_callback| will be invoked on completion.
  void DownloadToString(const content::ResourceRequest& resource_request,
                        content::mojom::URLLoaderFactory* url_loader_factory,
                        const net::NetworkTrafficAnnotationTag& annotation_tag,
                        BodyAsStringCallback body_as_string_callback,
                        size_t max_body_size);

  // Same as DownloadToString, but downloads to a buffer of unbounded size,
  // potentially causing a crash if the amount of addressable memory is
  // exceeded. It's recommended consumers use DownloadToString instead.
  void DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      const content::ResourceRequest& resource_request,
      content::mojom::URLLoaderFactory* url_loader_factory,
      const net::NetworkTrafficAnnotationTag& annotation_tag,
      BodyAsStringCallback body_as_string_callback);

  // Sets whether partially received results are allowed. Defaults to false.
  // When true, if an error is received after reading the body starts or the max
  // allowed body size exceeded, the partial response body that was received
  // will be provided to the BodyAsStringCallback. The partial response body may
  // be an empty string.
  void set_allow_partial_results(bool allow_partial_results) {
    allow_partial_results_ = allow_partial_results;
  }

  // Sets whether bodies of non-2xx responses are returned.
  //
  // When false, if a non-2xx result is received (Other than a redirect), the
  // request will fail with net::FAILED without waiting to read the response
  // body, though headers will be accessible through response_info().
  //
  // When true, non-2xx responses are treated no differently than other
  // responses, so their response body is returned just as with any other
  // response code, and when they complete, net_error() will return net::OK, if
  // no other problem occurs.
  //
  // Defaults to false.
  // TODO(mmenke): Consider adding a new error code for this.
  void set_allow_http_error_results(bool allow_http_error_results) {
    allow_http_error_results_ = allow_http_error_results;
  }

  // Returns the net::Error representing the final status of the request. May
  // only be called once the loader has informed the caller of completion.
  int net_error() const;

  // The ResourceResponseHead for the request. Will be nullptr if ResponseInfo
  // was never received. May only be called once the loader has informed the
  // caller of completion.
  const content::ResourceResponseHead* response_info() const;

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

  // Bound to the URLLoaderClient message pipe (|binding_|) via
  // set_connection_error_handler.
  void OnConnectionError();

  // Called by BodyHandler when the body pipe was closed. This could indicate an
  // error, concellation, or completion. To determine which case this is, the
  // size will also be compared to the size reported in
  // ResourceRequestCompletionStatus(), if ResourceRequestCompletionStatus
  // indicates a success.
  void OnBodyPipeClosed(int64_t received_body_size);

  // Completes the request by calling FinishWithResult() if OnComplete() was
  // called and either no body pipe was ever received, or the body pipe was
  // closed.
  void MaybeComplete();

  // Finished the request with the provided error code, after freeing Mojo
  // resources. Closes any open pipes, so no URLLoader or BodyHandlers callbacks
  // will be invoked after this is called.
  void FinishWithResult(int net_error);

  bool allow_partial_results() const { return allow_partial_results_; }

  bool allow_partial_results_ = false;
  bool allow_http_error_results_ = false;

  content::mojom::URLLoaderPtr url_loader_;
  mojo::Binding<content::mojom::URLLoaderClient> binding_;
  std::unique_ptr<BodyHandler> body_handler_;

  bool request_completed_ = false;
  // The expected total size of the body, taken from
  // ResourceRequestCompletionStatus.
  int64_t expected_body_size_ = 0;

  bool body_started_ = false;
  bool body_completed_ = false;
  // Final size of the body. Set once the body's Mojo pipe has been closed.
  int64_t received_body_size_ = 0;

  // Set to true when FinishWithResult() is called. Once that happens, the
  // consumer is informed of completion, and both pipes should be closed.
  bool finished_ = false;

  // Result of the request.
  int net_error_ = net::ERR_IO_PENDING;

  std::unique_ptr<content::ResourceResponseHead> response_info_;

  DISALLOW_COPY_AND_ASSIGN(SimpleURLLoader);
};

}  // namespace loader

#endif  // COMPONENTS_LOADER_SIMPLE_URL_LOADER_H_
