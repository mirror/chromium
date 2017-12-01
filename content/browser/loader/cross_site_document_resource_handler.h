// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_CROSS_SITE_DOCUMENT_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_CROSS_SITE_DOCUMENT_RESOURCE_HANDLER_H_

#include "base/macros.h"
#include "content/browser/loader/layered_resource_handler.h"
#include "content/common/cross_site_document_classifier.h"
#include "content/public/common/resource_type.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

// A ResourceHandler that prevents the renderer process from receiving network
// responses that contain cross-site documents, with appropriate exceptions to
// preserve compatibility.
class CONTENT_EXPORT CrossSiteDocumentResourceHandler
    : public LayeredResourceHandler {
 public:
  CrossSiteDocumentResourceHandler(
      std::unique_ptr<ResourceHandler> next_handler,
      net::URLRequest* request,
      ResourceType resource_type);
  ~CrossSiteDocumentResourceHandler() override;

  // LayeredResourceHandler overrides:
  void OnResponseStarted(
      ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                  int* buf_size,
                  std::unique_ptr<ResourceController> controller) override;
  void OnReadCompleted(int bytes_read,
                       std::unique_ptr<ResourceController> controller) override;
  void OnResponseCompleted(
      const net::URLRequestStatus& status,
      std::unique_ptr<ResourceController> controller) override;

 private:
  // Computes whether this response contains a cross-site document that needs to
  // be blocked from the renderer process.
  bool ShouldBlockResponse();

  // The type of the request, as claimed by the renderer process.
  ResourceType resource_type_;

  // A local buffer for sniffing content and using for throwaway reads, which is
  // not shared with the renderer process.
  scoped_refptr<net::IOBuffer> local_buffer_;

  // The buffer allocated by the next ResourceHandler for reads, which is used
  // if sniffing determines that we should proceed with the response.
  scoped_refptr<net::IOBuffer> next_handler_buffer_;

  // The size of |next_handler_buffer_|.
  int next_handler_buffer_size_;

  // Whether this response is a cross-site document that should be blocked,
  // pending the outcome of sniffing the content.
  bool should_block_response_;

  // A canonicalization of the specified MIME type, to determine if blocking the
  // response is needed, as well as which type of sniffing to perform.
  CrossSiteDocumentMimeType canonical_mime_type_;

  // Whether a "nosniff" header was present and thus sniffing should be skipped.
  bool no_sniff_;

  // Whether the next ResourceHandler has already been told that the read has
  // completed, and thus it is safe to cancel or detach on the next read.
  bool blocked_read_completed_;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_CROSS_SITE_DOCUMENT_RESOURCE_HANDLER_H_
