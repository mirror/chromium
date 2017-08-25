// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_RESOURCE_FETCHER_H_
#define CONTENT_PUBLIC_RENDERER_RESOURCE_FETCHER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "content/common/content_export.h"

class GURL;

namespace base {
class SingleThreadTaskRunner;
class TimeDelta;
}

namespace content {

struct ResourceRequest;

// Interface to download resources asynchronously.
class CONTENT_EXPORT ResourceFetcher {
 public:
  virtual ~ResourceFetcher() {}

  // This will be called asynchronously after the URL has been fetched,
  // successfully or not.  If there is a failure, |success| will be false, an
  // |http_status_code| and |final_url| may not be reliable.
  // |final_url| and |data| are both valid until the URLFetcher instance is
  // destroyed.
  typedef base::OnceCallback<void(bool success,
                                  int http_status_code,
                                  const GURL& final_url,
                                  const std::string& data)>
      Callback;

  // Creates a ResourceFetcher for the specific resource, and starts a request.
  // Deleting the ResourceFetcher will cancel the request, and |callback| will
  // never be run.
  static std::unique_ptr<ResourceFetcher> CreateAndStart(
      const ResourceRequest& request,
      base::SingleThreadTaskRunner* task_runner,
      Callback callback);

  // Sets how long to wait for the server to reply.  By default, there is no
  // timeout.  Must be called after a request is started.
  virtual void SetTimeout(const base::TimeDelta& timeout) = 0;

  // Manually cancel the request.
  virtual void Cancel() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_RESOURCE_FETCHER_H_
