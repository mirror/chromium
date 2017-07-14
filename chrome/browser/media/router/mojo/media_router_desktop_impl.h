// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_DESKTOP_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_DESKTOP_IMPL_H_

#include "chrome/browser/media/router/mojo/media_router_mojo_impl.h"

namespace media_router {

// MediaRouter implementation that delegates calls to the component extension.
// Also handles the suspension and wakeup of the component extension.
// Lives on the UI thread.
class MediaRouterDesktopImpl : public MediaRouterMojoImpl {
 public:
  ~MediaRouterDesktopImpl() override;

 protected:
  void DoCreateRoute(const MediaSource::Id& source_id,
                     const MediaSink::Id& sink_id,
                     const url::Origin& origin,
                     int tab_id,
                     std::vector<MediaRouteResponseCallback> callbacks,
                     base::TimeDelta timeout,
                     bool incognito) override;

 private:
  friend class MediaRouterFactory;
  explicit MediaRouterDesktopImpl(content::BrowserContext* context);

  base::WeakPtrFactory<MediaRouterDesktopImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterDesktopImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_DESKTOP_IMPL_H_
