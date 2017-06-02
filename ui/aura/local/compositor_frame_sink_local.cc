// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/local/compositor_frame_sink_local.h"

#include "cc/output/compositor_frame_sink_client.h"
#include "cc/surfaces/compositor_frame_sink_support.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

#include "base/logging.h"
#include "cc/ipc/begin_frame_args_struct_traits.h"
#include "cc/ipc/compositor_frame_metadata_struct_traits.h"
#include "cc/ipc/compositor_frame_struct_traits.h"
#include "cc/ipc/copy_output_request_struct_traits.h"
#include "cc/ipc/filter_operation_struct_traits.h"
#include "cc/ipc/filter_operations_struct_traits.h"
#include "cc/ipc/frame_sink_id_struct_traits.h"
#include "cc/ipc/local_surface_id_struct_traits.h"
#include "cc/ipc/mojo_compositor_frame_sink.mojom-shared-internal.h"
#include "cc/ipc/mojo_compositor_frame_sink.mojom-shared.h"
#include "cc/ipc/quads_struct_traits.h"
#include "cc/ipc/render_pass_struct_traits.h"
#include "cc/ipc/returned_resource_struct_traits.h"
#include "cc/ipc/selection_struct_traits.h"
#include "cc/ipc/shared_quad_state_struct_traits.h"
#include "cc/ipc/surface_id_struct_traits.h"
#include "cc/ipc/texture_mailbox_struct_traits.h"
#include "cc/ipc/transferable_resource_struct_traits.h"
#include "gpu/ipc/common/mailbox_holder_struct_traits.h"
#include "gpu/ipc/common/mailbox_struct_traits.h"
#include "gpu/ipc/common/sync_token_struct_traits.h"
#include "ipc/ipc_message_utils.h"
#include "mojo/common/common_custom_types_struct_traits.h"
#include "mojo/public/cpp/bindings/lib/message_builder.h"
#include "mojo/public/cpp/bindings/lib/serialization_util.h"
#include "mojo/public/cpp/bindings/lib/validate_params.h"
#include "mojo/public/cpp/bindings/lib/validation_context.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/interfaces/bindings/interface_control_messages.mojom.h"
#include "skia/public/interfaces/image_filter_struct_traits.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"
#include "ui/gfx/ipc/color/gfx_param_traits.h"
#include "ui/gfx/mojo/buffer_types_struct_traits.h"
#include "ui/gfx/mojo/selection_bound_struct_traits.h"
#include "ui/gfx/mojo/transform_struct_traits.h"
#include "ui/latency/mojo/latency_info_struct_traits.h"

namespace aura {

CompositorFrameSinkLocal::CompositorFrameSinkLocal(
    const cc::FrameSinkId& frame_sink_id,
    cc::SurfaceManager* surface_manager)
    : cc::CompositorFrameSink(nullptr, nullptr, nullptr, nullptr),
      frame_sink_id_(frame_sink_id),
      surface_manager_(surface_manager) {}

CompositorFrameSinkLocal::~CompositorFrameSinkLocal() {}

bool CompositorFrameSinkLocal::BindToClient(
    cc::CompositorFrameSinkClient* client) {
  if (!cc::CompositorFrameSink::BindToClient(client))
    return false;
  DCHECK(!thread_checker_);
  thread_checker_ = base::MakeUnique<base::ThreadChecker>();

  support_ = cc::CompositorFrameSinkSupport::Create(
      this, surface_manager_, frame_sink_id_, false /* is_root */,
      true /* handles_frame_sink_id_invalidation */,
      true /* needs_sync_points */);
  begin_frame_source_ = base::MakeUnique<cc::ExternalBeginFrameSource>(this);
  client->SetBeginFrameSource(begin_frame_source_.get());
  return true;
}

void CompositorFrameSinkLocal::SetSurfaceChangedCallback(
    const SurfaceChangedCallback& callback) {
  DCHECK(!surface_changed_callback_);
  surface_changed_callback_ = callback;
}

void CompositorFrameSinkLocal::DetachFromClient() {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  client_->SetBeginFrameSource(nullptr);
  begin_frame_source_.reset();
  support_->EvictCurrentSurface();
  support_.reset();
  thread_checker_.reset();
  cc::CompositorFrameSink::DetachFromClient();
}

void CompositorFrameSinkLocal::SubmitCompositorFrame(
    cc::CompositorFrame frame) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  DCHECK(frame.metadata.begin_frame_ack.has_damage);
  DCHECK_LE(cc::BeginFrameArgs::kStartingFrameNumber,
            frame.metadata.begin_frame_ack.sequence_number);

  cc::LocalSurfaceId old_local_surface_id = local_surface_id_;
  const auto& frame_size = frame.render_pass_list.back()->output_rect.size();
  if (frame_size != surface_size_ ||
      frame.metadata.device_scale_factor != device_scale_factor_ ||
      !local_surface_id_.is_valid()) {
    surface_size_ = frame_size;
    device_scale_factor_ = frame.metadata.device_scale_factor;
    local_surface_id_ = id_allocator_.GenerateId();
  }
  const auto& in_local_surface_id = local_surface_id_;
  const auto& in_frame = frame;
  {
    mojo::internal::SerializationContext serialization_context;
    size_t size =
        sizeof(::cc::mojom::internal::
                   MojoCompositorFrameSink_SubmitCompositorFrame_Params_Data);
    constexpr uint32_t kFlags = 0;
    size +=
        mojo::internal::PrepareToSerialize<::cc::mojom::LocalSurfaceIdDataView>(
            in_local_surface_id, &serialization_context);
    size += mojo::internal::PrepareToSerialize<
        ::cc::mojom::CompositorFrameDataView>(in_frame, &serialization_context);
    mojo::internal::MessageBuilder builder(
        cc::mojom::internal::
            kMojoCompositorFrameSink_SubmitCompositorFrame_Name,
        kFlags, size, serialization_context.associated_endpoint_count);

    auto params = ::cc::mojom::internal::
        MojoCompositorFrameSink_SubmitCompositorFrame_Params_Data::New(
            builder.buffer());
    ALLOW_UNUSED_LOCAL(params);
    typename decltype(params->local_surface_id)::BaseType* local_surface_id_ptr;
    mojo::internal::Serialize<::cc::mojom::LocalSurfaceIdDataView>(
        in_local_surface_id, builder.buffer(), &local_surface_id_ptr,
        &serialization_context);
    params->local_surface_id.Set(local_surface_id_ptr);
    MOJO_INTERNAL_DLOG_SERIALIZATION_WARNING(
        params->local_surface_id.is_null(),
        mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
        "null local_surface_id in "
        "MojoCompositorFrameSink.SubmitCompositorFrame request");
    typename decltype(params->frame)::BaseType* frame_ptr;
    mojo::internal::Serialize<::cc::mojom::CompositorFrameDataView>(
        in_frame, builder.buffer(), &frame_ptr, &serialization_context);
    params->frame.Set(frame_ptr);
    MOJO_INTERNAL_DLOG_SERIALIZATION_WARNING(
        params->frame.is_null(),
        mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
        "null frame in MojoCompositorFrameSink.SubmitCompositorFrame request");
    (&serialization_context)
        ->handles.Swap(builder.message()->mutable_handles());
    (&serialization_context)
        ->associated_endpoint_handles.swap(
            *builder.message()->mutable_associated_endpoint_handles());
    // This return value may be ignored as false implies the Connector has
    // encountered an error, which will be visible through other means.

    auto message = builder.message();
    if (buffer_.size() < message->data_num_bytes())
      buffer_.resize(message->data_num_bytes());

    // Copy buffer to kernel
    memcpy(buffer_.data(), message->data(), message->data_num_bytes());

    // Copy buffer from kernel
    memcpy(buffer_.data(), message->data(), message->data_num_bytes());

    {
      cc::mojom::internal::
          MojoCompositorFrameSink_SubmitCompositorFrame_Params_Data* params =
              reinterpret_cast<
                  cc::mojom::internal::
                      MojoCompositorFrameSink_SubmitCompositorFrame_Params_Data*>(
                  message->mutable_payload());
      mojo::internal::SerializationContext serialization_context;
      serialization_context.handles.Swap((message)->mutable_handles());
      serialization_context.associated_endpoint_handles.swap(
          *(message)->mutable_associated_endpoint_handles());
      bool success = true;
      cc::LocalSurfaceId p_local_surface_id{};
      cc::CompositorFrame p_frame{};
      cc::mojom::MojoCompositorFrameSink_SubmitCompositorFrame_ParamsDataView
          input_data_view(params, &serialization_context);

      if (!input_data_view.ReadLocalSurfaceId(&p_local_surface_id))
        success = false;
      if (!input_data_view.ReadFrame(&p_frame))
        success = false;
      if (!success) {
        ReportValidationErrorForMessage(
            message, mojo::internal::VALIDATION_ERROR_DESERIALIZATION_FAILED,
            "MojoCompositorFrameSink::SubmitCompositorFrame deserializer");
        return;
      }
      // A null |impl| means no implementation was bound.
      assert(impl);
      mojo::internal::MessageDispatchContext context(message);
      bool result = support_->SubmitCompositorFrame(
          std::move(p_local_surface_id), std::move(p_frame));
      DCHECK(result);
    }
  }

#if 0
  bool result =
      support_->SubmitCompositorFrame(local_surface_id_, std::move(frame));
  DCHECK(result);
#endif

  if (local_surface_id_ != old_local_surface_id) {
    surface_changed_callback_.Run(
        cc::SurfaceId(frame_sink_id_, local_surface_id_), surface_size_);
  }
}

void CompositorFrameSinkLocal::DidNotProduceFrame(
    const cc::BeginFrameAck& ack) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  DCHECK(!ack.has_damage);
  DCHECK_LE(cc::BeginFrameArgs::kStartingFrameNumber, ack.sequence_number);
  support_->DidNotProduceFrame(ack);
}

void CompositorFrameSinkLocal::DidReceiveCompositorFrameAck(
    const cc::ReturnedResourceArray& resources) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  if (!client_)
    return;
  if (!resources.empty())
    client_->ReclaimResources(resources);
  client_->DidReceiveCompositorFrameAck();
}

void CompositorFrameSinkLocal::OnBeginFrame(const cc::BeginFrameArgs& args) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  begin_frame_source_->OnBeginFrame(args);
}

void CompositorFrameSinkLocal::ReclaimResources(
    const cc::ReturnedResourceArray& resources) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  if (!client_)
    return;
  client_->ReclaimResources(resources);
}

void CompositorFrameSinkLocal::OnNeedsBeginFrames(bool needs_begin_frames) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  support_->SetNeedsBeginFrame(needs_begin_frames);
}

}  // namespace aura
