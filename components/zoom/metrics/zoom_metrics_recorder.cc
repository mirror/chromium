// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/metrics/zoom_metrics_recorder.h"

#include "base/optional.h"
#include "components/zoom/metrics/style_info.h"
#include "components/zoom/metrics/zoom_metrics_cache.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/gfx/geometry/size.h"

namespace zoom {
namespace metrics {

// TODO remove
void DebugPrint(const StyleInfo& info) {
  std::map<uint8_t, uint32_t> font_size_distribution =
      info.font_size_distribution;
  LOG(ERROR) << "Fonts:";
  for (const auto& font : font_size_distribution) {
    LOG(ERROR) << "font[" << static_cast<uint32_t>(font.first)
               << "] = " << font.second;
  }

  if (info.largest_image)
    LOG(ERROR) << "larget image - " << info.largest_image->width() << " "
               << info.largest_image->height();
  else
    LOG(ERROR) << "larget image - none";

  if (info.largest_object)
    LOG(ERROR) << "larget object - " << info.largest_object->width() << " "
               << info.largest_object->height();
  else
    LOG(ERROR) << "larget object - none";

  if (info.main_frame_info) {
    const gfx::Size& visible_content_size =
        info.main_frame_info->visible_content_size;
    const gfx::Size& contents_size = info.main_frame_info->contents_size;
    LOG(ERROR) << "\nvisible - " << visible_content_size.width() << " "
               << visible_content_size.height() << "\ncontent - "
               << contents_size.width() << " " << contents_size.height()
               << "\npreferred - " << info.main_frame_info->preferred_width;

    LOG(ERROR) << "\ntext - "
               << static_cast<float>(info.text_area) /
                      (contents_size.width() * contents_size.height())
               << "\nimage - "
               << static_cast<float>(info.image_area) /
                      (contents_size.width() * contents_size.height())
               << "\nobject - "
               << static_cast<float>(info.object_area) /
                      (contents_size.width() * contents_size.height());
  }
  LOG(ERROR) << "\ntext - " << info.text_area << "\nimage - " << info.image_area
             << "\nobject - " << info.object_area;
}

// static
const base::TimeDelta ZoomMetricsRecorder::kStyleRequestDelay =
    base::TimeDelta::FromSeconds(15);

ZoomMetricsRecorder::ZoomMetricsRecorder(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      pending_requests_(0),
      sent_request_(false),
      collection_complete_(false) {}

ZoomMetricsRecorder::~ZoomMetricsRecorder() {}

void ZoomMetricsRecorder::OnUserZoom(float zoom_factor,
                                     float default_zoom_factor,
                                     bool browser_handled) {
  zoom_factor_ = zoom_factor;
  default_zoom_factor_ = default_zoom_factor;

  // There's no need to do anything here if metrics recording is disabled.
  if (!ZoomMetricsCache::GetInstance()->IsRecordingEnabled())
    return;

  // For now, we don't consider pages where zooming is not done by the browser.
  if (!browser_handled)
    return;

  // The user has zoomed after we requested style information. So the zoom
  // level at which we collected the information may not be the user's
  // desired zoom level.
  if (sent_request_) {
    Cancel();
    return;
  }

  request_timer_.Stop();
  request_timer_.Start(FROM_HERE, kStyleRequestDelay,
                       base::Bind(&ZoomMetricsRecorder::RequestStyleInfo,
                                  base::Unretained(this)));
}

void ZoomMetricsRecorder::RequestStyleInfo() {
  DCHECK_EQ(0, pending_requests_);
  DCHECK(!collection_complete_);

  sent_request_ = true;
  recorded_data_ = StyleInfo();

  for (content::RenderFrameHost* frame : web_contents()->GetAllFrames()) {
    if (!frame->IsRenderFrameLive())
      continue;

    ++pending_requests_;
    GetStyleCollector(frame)->GetStyle(
        base::Bind(&ZoomMetricsRecorder::OnReceivedStyleFromFrame,
                   base::Unretained(this), frame));
  }
}

void ZoomMetricsRecorder::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted())
    return;

  request_timer_.Stop();

  if (!sent_request_)
    return;

  frame_collector_map_.clear();
  pending_requests_ = 0;

  if (navigation_handle->IsInMainFrame() &&
      !navigation_handle->IsSameDocument()) {
    Flush();
    sent_request_ = false;
  }
}

void ZoomMetricsRecorder::WebContentsDestroyed() {
  if (!sent_request_)
    return;

  frame_collector_map_.clear();

  Flush();
}

void ZoomMetricsRecorder::RenderFrameDeleted(content::RenderFrameHost* frame) {
  if (!sent_request_)
    return;

  frame_collector_map_.erase(frame);
}

void ZoomMetricsRecorder::OnReceivedStyleFromFrame(
    content::RenderFrameHost* frame,
    const base::Optional<StyleInfo>& info) {
  DCHECK(sent_request_);
  DCHECK(!collection_complete_);

  // The collection was canceled.
  if (!recorded_data_)
    return;

  DCHECK_GT(pending_requests_, 0);
  --pending_requests_;

  // The renderer was unable to provide style information. Do not proceed with
  // data collection.
  if (!info) {
    Cancel();
    return;
  }

  bool did_merge = recorded_data_->Merge(*info);

  if (!did_merge ||
      (frame == web_contents()->GetMainFrame() && !info->main_frame_info)) {
    Cancel();
    return;
  }

  if (pending_requests_ == 0)
    collection_complete_ = true;
}

void ZoomMetricsRecorder::Flush() {
  if (recorded_data_ && collection_complete_) {
    DebugPrint(*recorded_data_);
    ZoomMetricsCache::GetInstance()->AddZoomEvent(
        zoom_factor_, default_zoom_factor_, *recorded_data_);
  }

  collection_complete_ = false;
  recorded_data_ = base::nullopt;
}

void ZoomMetricsRecorder::Cancel() {
  request_timer_.Stop();
  frame_collector_map_.clear();
  pending_requests_ = 0;
  collection_complete_ = false;
  recorded_data_ = base::nullopt;
}

void ZoomMetricsRecorder::OnConnectionError(content::RenderFrameHost* frame) {
  Cancel();
}

const blink::mojom::zoom_metrics::StyleCollectorPtr&
ZoomMetricsRecorder::GetStyleCollector(content::RenderFrameHost* frame) {
  auto insertion = frame_collector_map_.insert(std::make_pair(frame, nullptr));
  if (insertion.second) {
    frame->GetRemoteInterfaces()->GetInterface(
        mojo::MakeRequest(&insertion.first->second));
  }
  insertion.first->second.set_connection_error_handler(base::Bind(
      &ZoomMetricsRecorder::OnConnectionError, base::Unretained(this), frame));
  return insertion.first->second;
}

}  // namespace metrics
}  // namespace zoom
