// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_REQUESTS_TRACKER_IMPL_HOLDER_H_
#define CONTENT_BROWSER_RENDERER_HOST_REQUESTS_TRACKER_IMPL_HOLDER_H_

#include "base/memory/ref_counted.h"
#include "content/common/frame.mojom.h"

namespace content {

class RenderProcessHost;

class RequestsTrackerImplHolder final {
 public:
  RequestsTrackerImplHolder(mojom::RequestsTrackerRequest request,
                            RenderProcessHost* process_host);
  ~RequestsTrackerImplHolder();

 private:
  class RequestsTrackerImpl;
  class Context;

  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(RequestsTrackerImplHolder);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_REQUESTS_TRACKER_IMPL_HOLDER_H_
