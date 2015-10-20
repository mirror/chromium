// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mus/ws/server_window_surface.h"

#include "cc/output/compositor_frame.h"
#include "cc/quads/shared_quad_state.h"
#include "cc/quads/surface_draw_quad.h"
#include "components/mus/surfaces/surfaces_state.h"
#include "components/mus/ws/server_window.h"
#include "components/mus/ws/server_window_delegate.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"

namespace mus {

namespace ws {

namespace {

void CallCallback(const mojo::Closure& callback, cc::SurfaceDrawStatus status) {
  callback.Run();
}

}  // namespace

ServerWindowSurface::ServerWindowSurface(ServerWindow* window)
    : window_(window),
      surface_id_allocator_(WindowIdToTransportId(window->id())),
      surface_factory_(window_->delegate()->GetSurfacesState()->manager(),
                       this),
      binding_(this) {
  surface_id_ = surface_id_allocator_.GenerateId();
  surface_factory_.Create(surface_id_);
}

ServerWindowSurface::~ServerWindowSurface() {
  // SurfaceFactory's destructor will attempt to return resources which will
  // call back into here and access |client_| so we should destroy
  // |surface_factory_|'s resources early on.
  surface_factory_.DestroyAll();
}

void ServerWindowSurface::Bind(mojo::InterfaceRequest<Surface> request,
                               mojom::SurfaceClientPtr client) {
  if (binding_.is_bound()) {
    // Destroy frame surfaces submitted by the old client before replacing
    // client_, so those surfaces will be returned to the old client.
    surface_factory_.DestroyAll();
    binding_.Close();
  }
  binding_.Bind(request.Pass());
  client_ = client.Pass();
}

void ServerWindowSurface::SubmitCompositorFrame(
    mojom::CompositorFramePtr frame,
    const SubmitCompositorFrameCallback& callback) {
  gfx::Size frame_size = frame->passes[0]->output_rect.To<gfx::Rect>().size();
  if (!surface_id_.is_null()) {
    // If the size of the CompostiorFrame has changed then destroy the existing
    // Surface and create a new one of the appropriate size.
    if (frame_size != last_submitted_frame_size_) {
      surface_factory_.Destroy(surface_id_);
      surface_id_ = surface_id_allocator_.GenerateId();
      surface_factory_.Create(surface_id_);
    }
  }
  surface_factory_.SubmitCompositorFrame(surface_id_,
                                         ConvertCompositorFrame(frame),
                                         base::Bind(&CallCallback, callback));
  window_->delegate()->GetSurfacesState()->scheduler()->SetNeedsDraw();
  window_->delegate()->OnScheduleWindowPaint(window_);
  last_submitted_frame_size_ = frame_size;
}

scoped_ptr<cc::CompositorFrame> ServerWindowSurface::ConvertCompositorFrame(
    const mojom::CompositorFramePtr& input) {
  referenced_window_ids_.clear();
  return ConvertToCompositorFrame(input, this);
}

bool ServerWindowSurface::ConvertSurfaceDrawQuad(
    const mojom::QuadPtr& input,
    const mojom::CompositorFrameMetadataPtr& metadata,
    cc::SharedQuadState* sqs,
    cc::RenderPass* render_pass) {
  Id id = static_cast<Id>(
      input->surface_quad_state->surface.To<cc::SurfaceId>().id);
  WindowId window_id = WindowIdFromTransportId(id);
  ServerWindow* window = window_->GetChildWindow(window_id);
  if (!window)
    return false;

  referenced_window_ids_.insert(window_id);
  cc::SurfaceDrawQuad* surface_quad =
      render_pass->CreateAndAppendDrawQuad<cc::SurfaceDrawQuad>();
  surface_quad->SetAll(
      sqs, input->rect.To<gfx::Rect>(), input->opaque_rect.To<gfx::Rect>(),
      input->visible_rect.To<gfx::Rect>(), input->needs_blending,
      window->GetOrCreateSurface()->id());
  return true;
}

void ServerWindowSurface::ReturnResources(
    const cc::ReturnedResourceArray& resources) {
  if (!client_)
    return;
  client_->ReturnResources(
      mojo::Array<mojom::ReturnedResourcePtr>::From(resources));
}

void ServerWindowSurface::SetBeginFrameSource(
    cc::SurfaceId surface_id,
    cc::BeginFrameSource* begin_frame_source) {
  // TODO(tansell): Implement this.
  NOTIMPLEMENTED();
}

}  // namespace ws

}  // namespace mus
