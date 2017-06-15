// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "components/exo/wayland/clients/client_base.h"
#include "components/exo/wayland/clients/client_helper.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gl/gl_bindings.h"

namespace exo {
namespace wayland {
namespace clients {
namespace {

void FrameCallback(void* data, wl_callback* callback, uint32_t time) {
  bool* callback_pending = static_cast<bool*>(data);
  *callback_pending = false;
}

}  // namespace

class SimpleClient : public ClientBase {
 public:
  SimpleClient() {}
  void Run(const ClientBase::InitParams& params);

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleClient);
};

void SimpleClient::Run(const ClientBase::InitParams& params) {
  if (!ClientBase::Init(params))
    return;

  std::unique_ptr<wl_surface> subsurface(static_cast<wl_surface*>(
      wl_compositor_create_surface(globals_.compositor.get())));

  std::unique_ptr<wl_subsurface> sub(
      static_cast<wl_subsurface*>(wl_subcompositor_get_subsurface(
          globals_.subcompositor.get(), subsurface.get(), surface_.get())));

  if (!subsurface || !sub) {
    LOG(ERROR) << "Can't create subsurface";
    return;
  }

  constexpr int32_t kSubsurfaceWidth = 128;
  constexpr int32_t kSubsurfaceHeight = 128;
  auto subbuffer =
      CreateBuffer(kSubsurfaceWidth, kSubsurfaceHeight, params.drm_format);
  if (!subbuffer) {
    LOG(ERROR) << "Failed to create subbuffer";
    return;
  }

  bool callback_pending = false;
  std::unique_ptr<wl_callback> frame_callback;
  wl_callback_listener frame_listener = {FrameCallback};

  size_t frame_count = 0;
  do {
    if (callback_pending && frame_count < 600)
      continue;

    // Only generate frames to subsurface for the first 200 frames.
    if (frame_count < 200) {
      SkScalar half_width = SkScalarHalf(kSubsurfaceWidth);
      SkScalar half_height = SkScalarHalf(kSubsurfaceHeight);
      SkIRect rect = SkIRect::MakeXYWH(-SkScalarHalf(half_width),
                                       -SkScalarHalf(half_height), half_width,
                                       half_height);
      // Rotation speed (degrees/frame).
      const double kRotationSpeed = 5.;
      SkScalar rotation = frame_count * kRotationSpeed;
      SkCanvas* canvas = subbuffer->sk_surface->getCanvas();
      canvas->save();
      canvas->clear(SK_ColorBLACK);
      SkPaint paint;
      paint.setColor(SkColorSetA(SK_ColorYELLOW, 0xA0));
      canvas->translate(half_width, half_height);
      canvas->rotate(rotation);
      canvas->drawIRect(rect, paint);
      canvas->restore();
      if (gr_context_) {
        gr_context_->flush();
        glFinish();
      }
      wl_surface_set_buffer_scale(subsurface.get(), scale_);
      wl_surface_damage(subsurface.get(), 0, 0, kSubsurfaceWidth / scale_,
                        kSubsurfaceHeight / scale_);
      wl_surface_attach(subsurface.get(), subbuffer->buffer.get(), 0, 0);
      wl_surface_commit(subsurface.get());
    }

    // Only generate frames to parent surface for the first 400 frames.
    if (frame_count < 400) {
      Buffer* buffer = buffers_.front().get();
      SkCanvas* canvas = buffer->sk_surface->getCanvas();
      static const SkColor kColors[] = {SK_ColorRED, SK_ColorBLACK};
      canvas->clear(kColors[frame_count % arraysize(kColors)]);
      if (gr_context_) {
        gr_context_->flush();
        glFinish();
      }
      wl_surface_set_buffer_scale(surface_.get(), scale_);
      wl_surface_damage(surface_.get(), 0, 0, width_ / scale_,
                        height_ / scale_);
      wl_surface_attach(surface_.get(), buffer->buffer.get(), 0, 0);

      callback_pending = true;
      frame_callback.reset(wl_surface_frame(surface_.get()));
      wl_callback_add_listener(frame_callback.get(), &frame_listener,
                               &callback_pending);
    }

    // Always animiate subsurface position.
    wl_subsurface_set_position(sub.get(), frame_count % 50, frame_count % 50);

    // Only commit changes for the first 600 frames.
    if (frame_count < 600)
      wl_surface_commit(surface_.get());

    wl_display_flush(display_.get());
    ++frame_count;
  } while (wl_display_dispatch(display_.get()) != -1);
}

}  // namespace clients
}  // namespace wayland
}  // namespace exo

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  exo::wayland::clients::ClientBase::InitParams params;
  if (!params.FromCommandLine(*command_line))
    return 1;

  exo::wayland::clients::SimpleClient client;
  client.Run(params);
  return 1;
}
