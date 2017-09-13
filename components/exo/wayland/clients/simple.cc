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

const size_t kWarmUpFrames = 20;
const size_t kFrameCounts = 600;

void FrameCallback(void* data, wl_callback* callback, uint32_t time) {
  bool* callback_pending = static_cast<bool*>(data);
  *callback_pending = false;
}

class SimpleClient : public wayland::clients::ClientBase {
 public:
  SimpleClient() {}
  float Run(const ClientBase::InitParams& params);

 private:
  base::TimeDelta DrawFrames(int count);

  DISALLOW_COPY_AND_ASSIGN(SimpleClient);
};

float SimpleClient::Run(const ClientBase::InitParams& params) {
  if (!ClientBase::Init(params))
    return 0.f;
  DrawFrames(kWarmUpFrames);
  auto time_delta = DrawFrames(kFrameCounts);
  return kFrameCounts / time_delta.InSecondsF();
}

base::TimeDelta SimpleClient::DrawFrames(int count) {
  bool callback_pending = false;
  std::unique_ptr<wl_callback> frame_callback;
  wl_callback_listener frame_listener = {FrameCallback};

  int frame_count = 0;
  auto start_time = base::TimeTicks::Now();
  do {
    if (callback_pending)
      continue;
    callback_pending = true;

    Buffer* buffer = buffers_.front().get();
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

    frame_callback.reset(wl_surface_frame(surface_.get()));
    wl_callback_add_listener(frame_callback.get(), &frame_listener,
                             &callback_pending);
    wl_surface_commit(surface_.get());
    wl_display_flush(display_.get());
  } while (wl_display_dispatch(display_.get()) != -1 && frame_count < count);
  return base::TimeTicks::Now() - start_time;
}

float SimpleMain() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  exo::wayland::clients::ClientBase::InitParams params;
  if (!params.FromCommandLine(*command_line))
    return 0.f;

  SimpleClient client;
  return client.Run(params);
}

}  // namespace clients
}  // namespace wayland
}  // namespace exo

#if !defined(WAYLAND_CLIENT_PERFTESTS)
int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  return exo::wayland::clients::SimpleMain() == 0.f ? 1 : 0;
}
#endif
