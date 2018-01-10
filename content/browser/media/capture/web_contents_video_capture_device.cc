// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/web_contents_video_capture_device.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "media/capture/video_capture_types.h"
#include "ui/base/layout.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

// Threading note: This is constructed on the device thread, while the
// destructor and the rest of the class will run exclusively on the UI thread.
class WebContentsVideoCaptureDevice::Targeter : public WebContentsObserver {
 public:
  Targeter(base::WeakPtr<FrameSinkVideoCaptureDevice> device,
           int render_process_id, int main_render_frame_id)
      : device_(std::move(device)),
        device_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
    DCHECK(device_task_runner_);

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce([](Targeter* self, int render_process_id, int main_render_frame_id) {
          self->Observe(WebContents::FromRenderFrameHost(RenderFrameHost::FromID(render_process_id, main_render_frame_id)));
          self->OnPossibleTargetChange();
        },
        // Unretained is safe since the destructor must run in some later task.
        base::Unretained(this),
        render_process_id, main_render_frame_id));
  }

  ~Targeter() final {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
  }

  // Find the view associated with the entirety of displayed content of the
  // current WebContents, whether that be a fullscreen view or the regular view.
  RenderWidgetHostView* GetCurrentView() const {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    WebContents* const contents = web_contents();
    if (!contents || contents->IsCrashed()) {
      return nullptr;
    }

    RenderWidgetHostView* view = contents->GetFullscreenRenderWidgetHostView();
    if (!view) {
      view = contents->GetRenderWidgetHostView();
    }
    // Make sure the RWHV is still associated with a RWH before considering the
    // view "alive." This is because a null RWH indicates the RWHV has had its
    // Destroy() method called.
    if (view && !view->GetRenderWidgetHost()) {
      return nullptr;
    }
    return view;
  }

 private:
  void OnPossibleTargetChange() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (web_contents()) {
      // Inside content code, downcasting from the public interface class to the
      // internal class is safe.
      RenderWidgetHostViewBase* const view = static_cast<RenderWidgetHostViewBase*>(GetCurrentView());
      const viz::FrameSinkId frame_sink_id = view ? view->GetFrameSinkId() : viz::FrameSinkId();
      const gfx::NativeView native_view = view ? view->GetNativeView() : gfx::NativeView();
      if (frame_sink_id != last_target_frame_sink_id_ || native_view != last_target_native_view_) {
        last_target_frame_sink_id_ = frame_sink_id;
        last_target_native_view_ = native_view;
        device_task_runner_->PostTask(FROM_HERE, base::BindOnce(&FrameSinkVideoCaptureDevice::OnTargetChanged, device_, frame_sink_id, native_view));
      }
    } else {
      device_task_runner_->PostTask(FROM_HERE, base::BindOnce(&FrameSinkVideoCaptureDevice::OnTargetPermanentlyLost, device_));
    }
  }

  // WebContentsObserver overrides.
  void RenderFrameCreated(RenderFrameHost* render_frame_host) final {
    OnPossibleTargetChange();
  }
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) final {
    OnPossibleTargetChange();
  }
  void RenderFrameHostChanged(RenderFrameHost* old_host, RenderFrameHost* new_host) final {
    OnPossibleTargetChange();
  }
  void DidShowFullscreenWidget() final {
    OnPossibleTargetChange();
  }
  void DidDestroyFullscreenWidget() final {
    OnPossibleTargetChange();
  }
  void WebContentsDestroyed() final {
    Observe(nullptr);
    OnPossibleTargetChange();
  }

  // |device_| may be dereferenced only by tasks run by |device_task_runner_|.
  const base::WeakPtr<FrameSinkVideoCaptureDevice> device_;
  const scoped_refptr<base::SingleThreadTaskRunner> device_task_runner_;

  viz::FrameSinkId last_target_frame_sink_id_;
  gfx::NativeView last_target_native_view_ = gfx::NativeView();
};

WebContentsVideoCaptureDevice::WebContentsVideoCaptureDevice(
    int render_process_id,
    int main_render_frame_id)
    : targeter_(new Targeter(FrameSinkVideoCaptureDevice::GetWeakPtr(), render_process_id, main_render_frame_id)) {}

WebContentsVideoCaptureDevice::~WebContentsVideoCaptureDevice() = default;

// static
std::unique_ptr<WebContentsVideoCaptureDevice>
WebContentsVideoCaptureDevice::Create(const std::string& device_id) {
  // Parse device_id into render_process_id and main_render_frame_id.
  WebContentsMediaCaptureId media_id;
  if (!WebContentsMediaCaptureId::Parse(device_id, &media_id)) {
    return nullptr;
  }
  return std::make_unique<WebContentsVideoCaptureDevice>(media_id.render_process_id, media_id.main_render_frame_id);
}

void WebContentsVideoCaptureDevice::WillStart(const media::VideoCaptureParams& params) {
  // Increment the WebContents's capturer count, providing WebContents with a
  // preferred size override during its capture. The preferred size is a strong
  // suggestion to UI layout code to size the view such that its physical
  // rendering size matches the exact capture size. This helps to eliminate
  // redundant scaling operations during capture.
  //
  // TODO(crbug.com/350491): Propagate capture frame size changes as new
  // "preferred size" updates, rather than just using the max frame size. This
  // would also fix an issue where the view may move to a different screen that
  // has a different device scale factor while being captured.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce([](Targeter* targeter, const gfx::Size& capture_size) {
        gfx::Size preferred_size = capture_size;
        if (auto* view = targeter->GetCurrentView()) {
          const gfx::Size dip_size = gfx::ConvertSizeToDIP(ui::GetScaleFactorForNativeView(view->GetNativeView()), capture_size);
          if (!dip_size.IsEmpty()) {
            preferred_size = dip_size;
          }
        }
        VLOG(1) << "Computed preferred WebContents size: " << preferred_size.ToString();
        if (auto* contents = targeter->web_contents()) {
          contents->IncrementCapturerCount(preferred_size);
        }
      }, base::Unretained(targeter_.get()), params.SuggestConstraints().max_frame_size));
}

void WebContentsVideoCaptureDevice::DidStop() {
  // Decrement the WebContents's capturer count.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::BindOnce([](Targeter* targeter) {
        if (auto* contents = targeter->web_contents()) {
          contents->DecrementCapturerCount();
        }
      }, base::Unretained(targeter_.get())));
}

}  // namespace content
