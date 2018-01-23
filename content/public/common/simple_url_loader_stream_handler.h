// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_STREAM_HANDLER_H_
#define CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_STREAM_HANDLER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/string/string_piece.h"
#include "content/common/content_export.h"

namespace content {

class ResourceResponseHead;

// Interface to handle streaming data from SimpleURLLoader. All methods are
// invoked on the sequence the SimpleURLLoader was started on, and all callbacks
// must be invoked on the same sequence. The SimpleURLLoader may be deleted at
// any time, including during a callback.
class CONTENT_EXPORT SimpleURLLoaderStreamHandler {
 public:
  // Called when the response headers are successfully received. If an HTTP
  // error response is received, and error codes are being treated as failures,
  // OnComplete(false) will be invoked instead (See
  // SimpleURLLoader::SetAllowHttpErrorResults).
  //
  // The request will not continue until |resume| is called. It's safe to call
  // |resume| synchronously.
  virtual void OnResponseStart(const ResourceResponseHead& response_info,
                               base::OnceClosure& resume);

  // Called after OnResponseStart, as data is received. |string_piece| is only
  // valid for the duration of the call. even in the case of error, all received
  // data will be passed to this method before calling OnComplete, even if
  // partial responses are set to be treated as errors (the default behavior),
  // as it may not yet be known if the request will succeed or fail.
  //
  // The request will not continue until |resume| is called. It's safe to call
  // |resume| synchronously.
  virtual void OnDataReceived(base::StringPiece string_piece,
                              base::OnceClosure& resume) = 0;

  // Called on successful completion, or error. May be called before or after
  // OnResponseStart. |success| is true if the request received a 2xx response
  // and the entire response body. False otherwise. Allowing partial responses
  // or HTTP errors will affect the value of |success|.
  virtual void OnComplete(bool success) = 0;

  // Called instead of OnComplete in cases where the SimpleURLFetcher is going
  // to retry, based on its configuration. If this is called after data has
  // been received, the entire response of the retried request will be passed
  // to OnDataReceived() again.
  virtual void OnRetry(base::OnceClosure& resume);

 protected:
  SimpleURLLoaderStreamHandler();
  virtual ~SimpleURLLoaderStreamHandler();

  DISALLOW_COPY_AND_ASSIGN(SimpleURLLoaderStreamHandler);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_STREAM_HANDLER_H_
