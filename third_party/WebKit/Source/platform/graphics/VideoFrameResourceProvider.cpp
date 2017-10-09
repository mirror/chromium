// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/VideoFrameResourceProvider.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "cc/resources/layer_tree_resource_provider.h"
#include "cc/resources/video_resource_updater.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "components/viz/common/quads/yuv_video_draw_quad.h"
#include "media/base/video_frame.h"
#include "media/video/gpu_video_accelerator_factories.h"
#include "platform/wtf/WeakPtr.h"

namespace cc {
class VideoFrameExternalResources;
}  // namespace cc

namespace blink {

VideoFrameResourceProvider::VideoFrameResourceProvider(
    WebContextProviderCallback context_provider_callback,
    viz::SharedBitmapManager* shared_bitmap_manager,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
    : context_provider_callback_(std::move(context_provider_callback)),
      shared_bitmap_manager_(shared_bitmap_manager),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      weak_ptr_factory_(this) {
  context_provider_callback_.Run(base::BindOnce(
      &VideoFrameResourceProvider::Initialize, weak_ptr_factory_.GetWeakPtr()));
}

void VideoFrameResourceProvider::Initialize(
    viz::ContextProvider* media_context_provider) {
  // TODO(lethalantidote): Need to handle null contexts.
  // https://crbug/768565
  CHECK(media_context_provider);

  context_provider_ = media_context_provider;

  resource_provider_ = std::make_unique<cc::LayerTreeResourceProvider>(
      media_context_provider, shared_bitmap_manager_,
      gpu_memory_buffer_manager_, false, resource_settings_);

  resource_updater_ = std::make_unique<cc::VideoResourceUpdater>(
      media_context_provider, resource_provider_.get(), false);
}

void VideoFrameResourceProvider::AppendQuads(
    viz::RenderPass& render_pass,
    scoped_refptr<media::VideoFrame> frame) {
  viz::ContextProvider::ScopedContextLock lock(context_provider_);
  cc::VideoFrameExternalResources external_resources =
      resource_updater_->CreateExternalResourcesFromVideoFrame(frame);

  // TODO(lethalantidote): Implement rotation.
  // TODO(lethalantidote): Implement visible_quad_rect.

  gfx::Rect visible_rect = frame->visible_rect();
  gfx::Size coded_size = frame->coded_size();
  // TODO(lethalantidote): Give true value;
  bool needs_blending = true;

  // FOR TEST
  gfx::Rect rect(0, 0, 960, 720);

  const float tex_width_scale =
      static_cast<float>(visible_rect.width()) / coded_size.width();
  const float tex_height_scale =
      static_cast<float>(visible_rect.height()) / coded_size.height();

  DCHECK_EQ(external_resources.mailboxes.size(),
            external_resources.release_callbacks.size());

  for (size_t i = 0; i < external_resources.mailboxes.size(); ++i) {
    unsigned resource_id = resource_provider_->CreateResourceFromTextureMailbox(
        external_resources.mailboxes[i],
        viz::SingleReleaseCallback::Create(
            external_resources.release_callbacks[i]),
        external_resources.read_lock_fences_enabled,
        external_resources.buffer_format);
    frame_resources_.push_back(FrameResource(
        resource_id, external_resources.mailboxes[i].size_in_pixels(),
        external_resources.mailboxes[i].is_overlay_candidate()));
  }

  viz::SharedQuadState* shared_quad_state =
      render_pass.CreateAndAppendSharedQuadState();

  switch (external_resources.type) {
    case cc::VideoFrameExternalResources::YUV_RESOURCE: {
      auto color_space = viz::YUVVideoDrawQuad::REC_601;
      int videoframe_color_space;
      if (frame->metadata()->GetInteger(media::VideoFrameMetadata::COLOR_SPACE,
                                        &videoframe_color_space)) {
        if (videoframe_color_space == media::COLOR_SPACE_JPEG) {
          color_space = viz::YUVVideoDrawQuad::JPEG;
        } else if (videoframe_color_space == media::COLOR_SPACE_HD_REC709) {
          color_space = viz::YUVVideoDrawQuad::REC_709;
        }
      }

      const gfx::Size ya_tex_size = coded_size;

      int u_width = media::VideoFrame::Columns(
          media::VideoFrame::kUPlane, frame->format(), coded_size.width());
      int u_height = media::VideoFrame::Rows(
          media::VideoFrame::kUPlane, frame->format(), coded_size.height());
      gfx::Size uv_tex_size(u_width, u_height);

      if (frame->HasTextures()) {
        if (frame->format() == media::PIXEL_FORMAT_NV12) {
          DCHECK_EQ(2u, frame_resources_.size());
        } else {
          DCHECK_EQ(media::PIXEL_FORMAT_I420, frame->format());
          DCHECK_EQ(3u,
                    frame_resources_.size());  // Alpha is not supported yet.
        }
      } else {
        DCHECK_GE(frame_resources_.size(), 3u);
        DCHECK(frame_resources_.size() <= 3 ||
               ya_tex_size == media::VideoFrame::PlaneSize(
                                  frame->format(), media::VideoFrame::kAPlane,
                                  coded_size));
      }

      // Compute the UV sub-sampling factor based on the ratio between
      // |ya_tex_size| and |uv_tex_size|.
      float uv_subsampling_factor_x =
          static_cast<float>(ya_tex_size.width()) / uv_tex_size.width();
      float uv_subsampling_factor_y =
          static_cast<float>(ya_tex_size.height()) / uv_tex_size.height();

      gfx::RectF ya_tex_coord_rect(visible_rect);
      gfx::RectF uv_tex_coord_rect(
          visible_rect.x() / uv_subsampling_factor_x,
          visible_rect.y() / uv_subsampling_factor_y,
          visible_rect.width() / uv_subsampling_factor_x,
          visible_rect.height() / uv_subsampling_factor_y);

      VLOG(0) << "frame_resource 0 id " << frame_resources_[0].id;
      VLOG(0) << "frame_resource 1 id " << frame_resources_[1].id;

      auto* yuv_video_quad =
          render_pass.CreateAndAppendDrawQuad<viz::YUVVideoDrawQuad>();
      yuv_video_quad->SetNew(
          //     Rect = test.
          shared_quad_state, rect, rect, needs_blending, ya_tex_coord_rect,
          uv_tex_coord_rect, ya_tex_size, uv_tex_size, frame_resources_[0].id,
          frame_resources_[1].id,
          frame_resources_.size() > 2 ? frame_resources_[2].id
                                      : frame_resources_[1].id,
          frame_resources_.size() > 3 ? frame_resources_[3].id : 0, color_space,
          frame->ColorSpace(), external_resources.offset,
          external_resources.multiplier, external_resources.bits_per_channel);
      yuv_video_quad->require_overlay =
          frame->metadata()->IsTrue(media::VideoFrameMetadata::REQUIRE_OVERLAY);
      // ValidateQuadResources(yuv_video_quad);
      for (viz::ResourceId resource_id : yuv_video_quad->resources) {
        resource_provider_->ValidateResource(resource_id);
      }
      VLOG(0) << "render pass quad size " << render_pass.quad_list.size();
      break;
    }
    case cc::VideoFrameExternalResources::RGBA_RESOURCE:
    case cc::VideoFrameExternalResources::RGBA_PREMULTIPLIED_RESOURCE:
    case cc::VideoFrameExternalResources::RGB_RESOURCE: {
      DCHECK_EQ(frame_resources_.size(), 1u);
      if (frame_resources_.size() < 1u)
        break;
      bool premultiplied_alpha =
          external_resources.type ==
          cc::VideoFrameExternalResources::RGBA_PREMULTIPLIED_RESOURCE;
      gfx::PointF uv_top_left(0.f, 0.f);
      gfx::PointF uv_bottom_right(tex_width_scale, tex_height_scale);
      float opacity[] = {1.0f, 1.0f, 1.0f, 1.0f};
      bool flipped = false;
      bool nearest_neighbor = false;
      auto* texture_quad =
          render_pass.CreateAndAppendDrawQuad<viz::TextureDrawQuad>();
      texture_quad->SetNew(shared_quad_state, visible_rect, visible_rect,
                           needs_blending, frame_resources_[0].id,
                           premultiplied_alpha, uv_top_left, uv_bottom_right,
                           SK_ColorTRANSPARENT, opacity, flipped,
                           nearest_neighbor, false);
      texture_quad->set_resource_size_in_pixels(coded_size);
      // TODO(lethalantidote): Implement.
      // ValidateQuadResources(texture_quad);
      break;
    }
    case cc::VideoFrameExternalResources::SOFTWARE_RESOURCE:
    case cc::VideoFrameExternalResources::STREAM_TEXTURE_RESOURCE: {
      VLOG(0) << "TYPE " << external_resources.type;
      gfx::Rect rect(0, 0, 10000, 10000);
      bool is_clipped = false;
      bool are_contents_opaque = true;
      viz::SolidColorDrawQuad* solid_color_quad =
          render_pass.CreateAndAppendDrawQuad<viz::SolidColorDrawQuad>();
      shared_quad_state->SetAll(gfx::Transform(), rect, rect, rect, is_clipped,
                                are_contents_opaque, 1, SkBlendMode::kSrcOver,
                                0);
      // Fluxuate colors for placeholder testing.
      static int r = 0;
      static int g = 0;
      static int b = 0;
      r++;
      g += 2;
      b += 3;
      solid_color_quad->SetNew(shared_quad_state, rect, visible_rect,
                               SkColorSetRGB(r % 255, g % 255, b % 255), false);
      break;
    }
    case cc::VideoFrameExternalResources::NONE:
      NOTIMPLEMENTED();
      break;
  }
}

void VideoFrameResourceProvider::DidDraw() {
  viz::ContextProvider::ScopedContextLock lock(context_provider_);
  for (size_t i = 0; i < frame_resources_.size(); ++i)
    resource_provider_->DeleteResource(frame_resources_[i].id);
  frame_resources_.clear();
}

}  // namespace blink
