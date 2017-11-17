// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {
class BrowserContext;
}

namespace net {
class URLRequestContextGetter;
}

namespace media_router {

class DialMediaSinkServiceImpl;

using OnDialSinkAddedCallback =
    base::RepeatingCallback<void(const MediaSinkInternal&)>;

// Delegates to DialMediaSinkServiceImpl by posting tasks to the
// SequencedTaskRunner that it runs on. All methods must be invoked on the UI
// thread.
class DialMediaSinkService {
 public:
  explicit DialMediaSinkService(content::BrowserContext* context);
  virtual ~DialMediaSinkService();

  // Starts discovery of DIAL sinks. No-ops if already started.
  // |sink_discovery_cb|: Callback to invoke when the list of discovered sinks
  // has been updated.
  // |dial_sink_added_cb|: Callback to invoke when a new DIAL sink has been
  // discovered.
  // Both callbacks are invoked on the UI thread.
  void Start(const OnSinksDiscoveredCallback& sink_discovery_cb,
             const OnDialSinkAddedCallback& dial_sink_added_cb);

  // Stops discovery of DIAL sinks. No-ops if already stopped.
  // Caller is responsible for calling Stop() before destroying this object.
  void Stop();

  // Delegates to |impl_|.
  void ForceSinkDiscoveryCallback();
  void OnUserGesture();

 private:
  friend class DialMediaSinkServiceTest;

  // Marked virtual for tests.
  virtual std::unique_ptr<DialMediaSinkServiceImpl> CreateImpl(
      const OnSinksDiscoveredCallback& sink_discovery_cb,
      const OnDialSinkAddedCallback& dial_sink_added_cb,
      const scoped_refptr<net::URLRequestContextGetter>& request_context);

  // Created on the UI thread, used and destroyed on its SequencedTaskRunner.
  std::unique_ptr<DialMediaSinkServiceImpl> impl_;

  // Passed to |impl_| when |Start| is called.
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  DISALLOW_COPY_AND_ASSIGN(DialMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_H_
