// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/wayland/clients/simple.h"

#include <presentation-time-client-protocol.h>

#include "base/command_line.h"
#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "components/exo/wayland/clients/client_helper.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gl/gl_bindings.h"

namespace exo {
namespace wayland {
namespace clients {
namespace {

struct Frame {
  base::TimeTicks submit_time;
  std::unique_ptr<wl_callback> callback;
  std::unique_ptr<struct wp_presentation_feedback> feedback;
};

struct Presentation {
  bool frame_callback_pending = false;

  base::circular_deque<Frame> scheduled_frames;
  Simple::Result result;
};

void FrameCallback(void* data, wl_callback* callback, uint32_t time) {
  Presentation* presentation = static_cast<Presentation*>(data);
  DCHECK(!presentation->scheduled_frames.empty());

  Frame& frame = presentation->scheduled_frames.back();
  DCHECK_EQ(frame.callback.get(), callback);
  frame.callback = nullptr;

  presentation->result.frame_latency +=
      base::TimeTicks::Now() - frame.submit_time;
  ++presentation->result.num_frames;
  presentation->frame_callback_pending = false;
  if (!frame.feedback) {
    DCHECK_EQ(1u, presentation->scheduled_frames.size());
    presentation->scheduled_frames.pop_front();
  }
}

void FeedbackSyncOutput(void* data,
                        struct wp_presentation_feedback* feedback,
                        wl_output* output) {}

void FeedbackPresented(void* data,
                       struct wp_presentation_feedback* feedback,
                       uint32_t tv_sec_hi,
                       uint32_t tv_sec_lo,
                       uint32_t tv_nsec,
                       uint32_t refresh,
                       uint32_t seq_hi,
                       uint32_t seq_lo,
                       uint32_t flags) {
  Presentation* presentation = static_cast<Presentation*>(data);
  DCHECK(!presentation->scheduled_frames.empty());

  Frame& frame = presentation->scheduled_frames.front();
  DCHECK_EQ(frame.feedback.get(), feedback);
  frame.feedback = nullptr;

  int64_t seconds = (static_cast<int64_t>(tv_sec_hi) << 32) + tv_sec_lo;
  int64_t microseconds = seconds * base::Time::kMicrosecondsPerSecond +
                         tv_nsec / base::Time::kNanosecondsPerMicrosecond;
  base::TimeTicks presentation_time =
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(microseconds);
  presentation->result.presentation_latency +=
      presentation_time - frame.submit_time;
  presentation->result.presentation_ipc_latency +=
      base::TimeTicks::Now() - presentation_time;
  ++presentation->result.num_frames_presented;

  if (!frame.callback)
    presentation->scheduled_frames.pop_front();
}

void FeedbackDiscarded(void* data, struct wp_presentation_feedback* feedback) {
  Presentation* presentation = static_cast<Presentation*>(data);
  DCHECK(!presentation->scheduled_frames.empty());

  Frame& frame = presentation->scheduled_frames.front();
  DCHECK_EQ(frame.feedback.get(), feedback);
  frame.feedback = nullptr;
  ++presentation->result.num_frames_discarded;

  if (!frame.callback)
    presentation->scheduled_frames.pop_front();
}

}  // namespace

Simple::Simple() = default;

void Simple::Run(int frames, Result* result) {
  wl_callback_listener frame_listener = {FrameCallback};
  wp_presentation_feedback_listener feedback_listener = {
      FeedbackSyncOutput, FeedbackPresented, FeedbackDiscarded};

  Presentation presentation;
  int frame_count = 0;

  do {
    if (presentation.frame_callback_pending)
      continue;

    if (frame_count == frames)
      break;

    Buffer* buffer = DequeueBuffer();
    if (!buffer)
      continue;

    SkCanvas* canvas = buffer->sk_surface->getCanvas();

    static const SkColor kColors[] = {SK_ColorRED, SK_ColorBLACK};
    canvas->clear(kColors[++frame_count % arraysize(kColors)]);

    if (gr_context_) {
      gr_context_->flush();
      glFinish();
    }

    wl_surface_set_buffer_scale(surface_.get(), scale_);
    wl_surface_set_buffer_transform(surface_.get(), transform_);
    wl_surface_damage(surface_.get(), 0, 0, surface_size_.width(),
                      surface_size_.height());
    wl_surface_attach(surface_.get(), buffer->buffer.get(), 0, 0);

    presentation.scheduled_frames.push_back(Frame());
    Frame& frame = presentation.scheduled_frames.back();

    // Set up the frame callback.
    presentation.frame_callback_pending = true;
    frame.callback.reset(wl_surface_frame(surface_.get()));
    wl_callback_add_listener(frame.callback.get(), &frame_listener,
                             &presentation);

    // Set up presentation feedback.
    frame.feedback.reset(
        wp_presentation_feedback(globals_.presentation.get(), surface_.get()));
    wp_presentation_feedback_add_listener(frame.feedback.get(),
                                          &feedback_listener, &presentation);

    frame.submit_time = base::TimeTicks::Now();
    wl_surface_commit(surface_.get());
    wl_display_flush(display_.get());
  } while (wl_display_dispatch(display_.get()) != -1);

  if (result)
    *result = presentation.result;
}

}  // namespace clients
}  // namespace wayland
}  // namespace exo
