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

namespace exo {
namespace wayland {
namespace clients {
namespace {

void FrameCallback(void* data, wl_callback* callback, uint32_t time) {
  bool* callback_pending = static_cast<bool*>(data);
  *callback_pending = false;
}

}  // namespace

class VulkanClient : ClientBase {
 public:
  VulkanClient() {}
  void Run(const ClientBase::InitParams& params);

 private:
  DISALLOW_COPY_AND_ASSIGN(VulkanClient);
};

void VulkanClient::Run(const ClientBase::InitParams& params) {
  if (!ClientBase::Init(params))
    return;

  bool callback_pending = false;
  std::unique_ptr<wl_callback> frame_callback;
  wl_callback_listener frame_listener = {FrameCallback};

  size_t frame_count = 0;
  do {
    if (callback_pending)
      continue;

    Buffer* buffer = buffers_.front().get();

    static const SkColor kColors[] = {SK_ColorRED, SK_ColorBLACK,
                                      SK_ColorGREEN};
    SkColor4f sk_color =
        SkColor4f::FromColor(kColors[++frame_count % arraysize(kColors)]);

    static const VkCommandBufferBeginInfo vk_command_buffer_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult result = vkBeginCommandBuffer(buffer->vk_command_buffer,
                                           &vk_command_buffer_begin_info);
    CHECK_EQ(VK_SUCCESS, result);

    VkClearValue clear_color = {
        .color =
            {
                .float32 =
                    {
                        sk_color.fR, sk_color.fG, sk_color.fB, sk_color.fA,
                    },
            },
    };
    VkRenderPassBeginInfo render_pass_begin_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk_renderpass_,
        .framebuffer = buffer->vk_framebuffer,
        .renderArea =
            (VkRect2D){
                .offset = {0, 0},
                .extent = {surface_size_.width(), surface_size_.height()},
            },
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };
    vkCmdBeginRenderPass(buffer->vk_command_buffer, &render_pass_begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(buffer->vk_command_buffer);

    result = vkEndCommandBuffer(buffer->vk_command_buffer);
    CHECK_EQ(VK_SUCCESS, result);
    VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             .commandBufferCount = 1,
                             .pCommandBuffers = &buffer->vk_command_buffer};
    result = vkQueueSubmit(vk_queue_, 1, &submit_info, VK_NULL_HANDLE);
    CHECK_EQ(VK_SUCCESS, result);

    result = vkQueueWaitIdle(vk_queue_);
    CHECK_EQ(VK_SUCCESS, result);

    ++frame_count;

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

  exo::wayland::clients::VulkanClient client;
  client.Run(params);
  return 1;
}
