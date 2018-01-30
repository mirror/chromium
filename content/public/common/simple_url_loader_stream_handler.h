// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_STREAM_HANDLER_H_
#define CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_STREAM_HANDLER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"

namespace content {

// Interface to handle streaming data from SimpleURLLoader. All methods are
// invoked on the sequence the SimpleURLLoader was started on, and all callbacks
// must be invoked on the same sequence. The SimpleURLLoader may be deleted at
// any time, including during a callback.
class CONTENT_EXPORT SimpleURLLoaderStreamHandler {
 public:
  // Called after OnResponseStart, as data is received.
  //
  // The request will not continue until |resume| is called. It's safe to call
  // |resume| synchronously, and to delete the SimpleURLLoader during the call.
  //
  // |string_piece| will only be valid for the duration of the OnDataReceived()
  // call, but does remain valid during that call even if the SimpleURLLoader
  // is destroyed.
  //
  // In the case of error, all received data will be passed to this method
  // before calling OnComplete, even if partial responses are set to be treated
  // as errors (the default behavior), as it may not yet be known if the request
  // will succeed or fail.
  virtual void OnDataReceived(base::StringPiece string_piece,
                              base::OnceClosure resume) = 0;

  // Called on successful completion, or error. May be called before or after
  // OnResponseStart. |success| is true if the request received a 2xx response
  // and the entire response body. False otherwise. Allowing partial responses
  // or HTTP errors will affect the value of |success|.
  //
  // 4xx and 5xx responses are considered successes, if
  // SimpleURLLoader::SetAllowHttpErrorResults() is set to true.
  //
  // When the SimpleURLLoader retries a request, OnComplete() is only called
  // after the final retry.
  virtual void OnComplete(bool success) = 0;

  // When retries are enabled for a request (They are not, by default), this is
  // called before the SimpleURLLoader retries. If this is called after data has
  // been received, the entire response of the retried request will be passed
  // to OnDataReceived() again, and the SimpleURLLoaderStreamHandler should
  // discard any previously received partial response.
  //
  // The request will not be retried until start_retry is invoked, which may be
  // done synchronously or asynchronously. It's safe to delete the
  // SimpleURLLoader during this call.
  virtual void OnRetry(base::OnceClosure start_retry) = 0;

 protected:
  SimpleURLLoaderStreamHandler() {}
  virtual ~SimpleURLLoaderStreamHandler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleURLLoaderStreamHandler);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_SIMPLE_URL_LOADER_STREAM_HANDLER_H_
