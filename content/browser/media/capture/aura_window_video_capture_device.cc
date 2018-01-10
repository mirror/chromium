// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/aura_window_video_capture_device.h"

#include <iostream> // REMOVE
#include <sstream> // REMOVE

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/media/capture/desktop_capture_device_uma_types.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"

namespace content {

namespace {

void PrintWindowTreeRecursive(aura::Window* window, int level, std::ostringstream* oss) {
  std::ostringstream& out = *oss;
  for (int i = 0; i < level; ++i)
    out << "  ";
  const viz::FrameSinkId frame_sink_id = window->GetFrameSinkId();
  out << window << " <--> " << frame_sink_id;
  if (auto* layer = window->layer()) {
    if (auto* surface_id = layer->GetPrimarySurfaceId()) {
      out << " <--> primary surface " << surface_id->frame_sink_id();
    }
    if (auto* surface_id = layer->GetFallbackSurfaceId()) {
      out << " <--> fallback surface " << surface_id->frame_sink_id();
    }
  }
  out << std::endl;
  ++level;
  for (aura::Window* child : window->children()) {
    PrintWindowTreeRecursive(child, level, oss);
  }
}

void PrintWindowTree(aura::Window* window) {
  std::ostringstream oss;
  PrintWindowTreeRecursive(window->GetRootWindow(), 0, &oss);
  LOG(ERROR) << __func__ << "Window tree containing " << window << " is:\n" << oss.str();
}

void FindFrameSink(const aura::Window* window, viz::FrameSinkId* frame_sink_id) {
  while (window) {
    if (auto* layer = window->layer()) {
      if (auto* surface_id = layer->GetPrimarySurfaceId()) {
        if (surface_id->frame_sink_id().is_valid()) {
          *frame_sink_id = surface_id->frame_sink_id();
          return;
        }
      }
    }

    if (window->children().size() == 1) {
      window = window->children()[0];
    } else {
      break;
    }
  }

  *frame_sink_id = viz::FrameSinkId();
}

}  // namespace

// Threading note: This is constructed on the device thread, while the
// destructor and the rest of the class will run exclusively on the UI thread.
class AuraWindowVideoCaptureDevice::Targeter : public aura::WindowObserver {
 public:
  Targeter(base::WeakPtr<FrameSinkVideoCaptureDevice> device, const DesktopMediaID& source)
      : device_(std::move(device)),
        device_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
    DCHECK(device_task_runner_);

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce([](Targeter* self, const DesktopMediaID& source) {
          LOG(ERROR) << __func__ << ": source=" << source.ToString();
          if (auto* window = DesktopMediaID::GetAuraWindowById(source)) {
            PrintWindowTree(window);

            viz::FrameSinkId frame_sink_id = window->GetFrameSinkId();
            LOG(ERROR) << __func__ << ": found aura window=" << window << ", frame sink id=" << frame_sink_id;
            if (!frame_sink_id.is_valid()) {
              // TODO: clean this up!!!
              // |window| may be the content window within a RootWindow. In this
              // case, the RootWindow should have the frame sink to be targeted.
              window = window->parent();
              if (window) {
                frame_sink_id = window->GetFrameSinkId();
              }
              LOG(ERROR) << __func__ << ": found aura window=" << window << ", frame sink id=" << frame_sink_id;
            }

            if (frame_sink_id.is_valid()) {
              self->target_window_ = window;
              window->AddObserver(self);

              self->device_task_runner_->PostTask(FROM_HERE, base::BindOnce(&FrameSinkVideoCaptureDevice::OnTargetChanged, self->device_, frame_sink_id, window));

              if (source.type == DesktopMediaID::TYPE_SCREEN) {
                IncrementDesktopCaptureCounter(source.audio_share ? SCREEN_CAPTURER_CREATED_WITH_AUDIO : SCREEN_CAPTURER_CREATED_WITHOUT_AUDIO);
              }

              IncrementDesktopCaptureCounter(window->IsRootWindow() ? SCREEN_CAPTURER_CREATED : WINDOW_CAPTURER_CREATED);
              return;
            }
          }

          LOG(ERROR) << __func__ << ": did NOT find aura window, or invalid frame sink";
          self->device_task_runner_->PostTask(FROM_HERE, base::BindOnce(&FrameSinkVideoCaptureDevice::OnTargetPermanentlyLost, self->device_));

        },
        // Unretained is safe since the destructor must run in some later task.
        base::Unretained(this),
        source));
  }

  ~Targeter() final {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (target_window_) {
      target_window_->RemoveObserver(this);
    }
  }

  // aura::WindowObserver override.
  void OnWindowDestroying(aura::Window* window) final {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (window != target_window_) {
      return;
    }

    target_window_->RemoveObserver(this);
    target_window_ = nullptr;

    LOG(ERROR) << __func__ << ": aura window destroyed";
    device_task_runner_->PostTask(FROM_HERE, base::BindOnce(&FrameSinkVideoCaptureDevice::OnTargetPermanentlyLost, device_));
  }

 private:
  // |device_| may be dereferenced only by tasks run by |device_task_runner_|.
  const base::WeakPtr<FrameSinkVideoCaptureDevice> device_;
  const scoped_refptr<base::SingleThreadTaskRunner> device_task_runner_;

  aura::Window* target_window_ = nullptr;
};

AuraWindowVideoCaptureDevice::AuraWindowVideoCaptureDevice(const DesktopMediaID& source)
    : targeter_(new Targeter(FrameSinkVideoCaptureDevice::GetWeakPtr(), source)) {}

AuraWindowVideoCaptureDevice::~AuraWindowVideoCaptureDevice() = default;

// static
std::unique_ptr<AuraWindowVideoCaptureDevice>
AuraWindowVideoCaptureDevice::Create(const DesktopMediaID& source) {
  if (source.aura_id == DesktopMediaID::kNullId)
    return nullptr;
  return std::make_unique<AuraWindowVideoCaptureDevice>(source);
}

}  // namespace content
