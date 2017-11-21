// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
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
// thread, with the exception being the *OnSequence methods, which run on
// |impl_->task_runner()|.
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
  // (Note that we can optimize |dial_sink_added_cb| to not require thread hops,
  // but that requires assuming about Dial and Cast impl running on the same
  // sequence. It would also complicate this API, as it would take 2 callbacks
  // that run on different sequences.)
   SequencedTaskRunner, so I left it out for now.
  void Start(const OnSinksDiscoveredCallback& sink_discovery_cb,
             const OnDialSinkAddedCallback& dial_sink_added_cb);

   // Delegates to |impl_| to force the sink discovery callback is to be
   // invoked with the current list of sinks.
   void ForceSinkDiscoveryCallback();

   // Delegates to |impl_| to initiate an out of band discovery.
   void OnUserGesture();

  private:
   friend class DialMediaSinkServiceTest;

   // Marked virtual for tests.
   virtual std::unique_ptr<DialMediaSinkServiceImpl> CreateImpl(
       const OnSinksDiscoveredCallback& sink_discovery_cb,
       const OnDialSinkAddedCallback& dial_sink_added_cb,
       const scoped_refptr<net::URLRequestContextGetter>& request_context);

   // These functions are invoked by DialMediaSinkServiceImpl on its
   // SequencedTaskRunner.
   // Invokes |sink_discovery_cb_| with |sinks| on |task_runner_|.
   void OnSinksDiscoveredOnSequence(std::vector<MediaSinkInternal> sinks);

   // If |dial_sink_added_cb_| is not null, invokes it with |sink| on
   // |task_runner_|.
   void OnDialSinkAddedOnSequence(const MediaSinkInternal& sink);

   OnSinksDiscoveredCallback sink_discovery_cb_;
   OnDialSinkAddedCallback dial_sink_added_cb_;

   // Created on the UI thread, used and destroyed on its SequencedTaskRunner.
   std::unique_ptr<DialMediaSinkServiceImpl> impl_;

   // Passed to |impl_| when |Start| is called.
   scoped_refptr<net::URLRequestContextGetter> request_context_;

   // The SequencedTaskRunner on which |this| runs.
   scoped_refptr<base::SequencedTaskRunner> task_runner_;

   SEQUENCE_CHECKER(sequence_checker_);
   base::WeakPtrFactory<DialMediaSinkService> weak_ptr_factory_;

   DISALLOW_COPY_AND_ASSIGN(DialMediaSinkService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_H_
