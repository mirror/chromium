// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_RECORDER_H_
#define COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_RECORDER_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/zoom/metrics/style_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/WebKit/public/platform/modules/zoom_metrics/style_collector.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace zoom {
namespace metrics {

// When the user zooms the page, collect style information about the page
// (e.g. font sizes used) which can be offered to UMA metrics along with
// details of the desired zoom level.
class ZoomMetricsRecorder : public content::WebContentsObserver {
 public:
  ZoomMetricsRecorder(content::WebContents* web_contents);
  ~ZoomMetricsRecorder() override;

  // Called when a user manually zooms the page.
  // |zoom_factor| is the zoom factor chosen by the user.
  // |default_zoom_factor| is the default zoom factor from the user's settings.
  // |browser_handled| is whether the zoom is performed by the browser
  // or by an extension.
  void OnUserZoom(float zoom_factor,
                  float default_zoom_factor,
                  bool browser_handled);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;
  void RenderFrameDeleted(content::RenderFrameHost* frame) override;

 private:
  // Send requests for style information to the frame(s) for the page.
  void RequestStyleInfo();

  // The response from the frame(s).
  void OnReceivedStyleFromFrame(content::RenderFrameHost* frame,
                                const base::Optional<StyleInfo>& info);
  void OnConnectionError(content::RenderFrameHost* frame);

  // Send collected data to UMA.
  void Flush();

  void Cancel();

  const blink::mojom::zoom_metrics::StyleCollectorPtr& GetStyleCollector(
      content::RenderFrameHost* frame);

  std::map<content::RenderFrameHost*,
           blink::mojom::zoom_metrics::StyleCollectorPtr>
      frame_collector_map_;

  int pending_requests_;
  bool sent_request_;

  // True once |recorded_data_| represents data from the entire page.
  bool collection_complete_;

  float zoom_factor_;
  float default_zoom_factor_;
  // The style from the page.
  base::Optional<StyleInfo> recorded_data_;

  // It is very common for a user to zoom multiple times before reaching the
  // desired zoom level. We delay sending the request for style information
  // from the page until |kStyleRequestDelay| seconds after the last zoom
  // action. The user has hopefully decided on a zoom level by that time.
  base::OneShotTimer request_timer_;
  static const base::TimeDelta kStyleRequestDelay;

  DISALLOW_COPY_AND_ASSIGN(ZoomMetricsRecorder);
};

}  // namespace metrics
}  // namespace zoom

#endif  // COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_RECORDER_H_
