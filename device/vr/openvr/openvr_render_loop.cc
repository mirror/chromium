// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/openvr/openvr_render_loop.h"

#include <math.h>

#include "base/memory/ptr_util.h"
#include "base/numerics/math_constants.h"
#include "build/build_config.h"
#include "device/vr/openvr/openvr_helpers.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "third_party/openvr/src/headers/openvr.h"
#include "ui/gfx/geometry/angle_conversions.h"

#if defined(OS_WIN)
#include "device/vr/windows/d3d11_texture_helper.h"
#endif

namespace device {
OpenVRRenderLoop::OpenVRRenderLoop()
    : base::Thread("OpenVRRenderLoop"),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      binding_(this),
      weak_ptr_factory_(this) {
  DCHECK(main_thread_task_runner_);
}

OpenVRRenderLoop::~OpenVRRenderLoop() {}

void OpenVRRenderLoop::Bind(mojom::VRPresentationProviderRequest request) {
  binding_.Close();
  binding_.Bind(std::move(request));
}

void OpenVRRenderLoop::SubmitFrame(int16_t frame_index,
                                   const gpu::MailboxHolder& mailbox) {
  NOTREACHED();
}

void OpenVRRenderLoop::SubmitFrameWithTextureHandle(
    mojo::ScopedHandle texture_handle,
    int16_t frame_index) {
  TRACE_EVENT1("gpu", "SubmitFrameWithTextureHandle", "frameIndex",
               frame_index);

#if defined(OS_WIN)
  texture_helper_.SetSourceTexture(std::move(texture_handle));
  texture_helper_.AllocateBackBuffer();
  if (texture_helper_.CopyTextureToBackBuffer(true)) {
    vr::Texture_t texture;
    texture.handle = texture_helper_.GetBackbuffer().Get();
    texture.eType = vr::TextureType_DirectX;
    texture.eColorSpace = vr::ColorSpace_Auto;

    vr::VRTextureBounds_t bounds[2];
    bounds[0] = {left_bounds_.x(), left_bounds_.y(),
                 left_bounds_.width() + left_bounds_.x(),
                 left_bounds_.height() + left_bounds_.y()};
    bounds[1] = {right_bounds_.x(), right_bounds_.y(),
                 right_bounds_.width() + right_bounds_.x(),
                 right_bounds_.height() + right_bounds_.y()};

    vr::EVRCompositorError error =
        vr_compositor_->Submit(vr::EVREye::Eye_Left, &texture, &bounds[0]);
    if (error != vr::VRCompositorError_None) {
      ExitPresent();
      return;
    }
    error = vr_compositor_->Submit(vr::EVREye::Eye_Right, &texture, &bounds[1]);
    if (error != vr::VRCompositorError_None) {
      ExitPresent();
      return;
    }
    vr_compositor_->PostPresentHandoff();
  }
#endif
}

void OpenVRRenderLoop::UpdateLayerBounds(int16_t frame_id,
                                         const gfx::RectF& left_bounds,
                                         const gfx::RectF& right_bounds,
                                         const gfx::Size& source_size) {
  // Bounds are updated instantly, rather than waiting for frame_id.  This works
  // since blink always passes the current frame_id when updating the bounds.
  // Ignoring the frame_id keeps the logic simpler, so this can more easily
  // merge with vr_shell_gl eventually.
  left_bounds_ = left_bounds;
  right_bounds_ = right_bounds;
  source_size_ = source_size;
};

void OpenVRRenderLoop::RequestPresent(
    mojom::VRPresentationProviderRequest request,
    base::Callback<void(bool)> callback) {
#if defined(OS_WIN)
  if (!texture_helper_.EnsureInitialized()) {
    main_thread_task_runner_->PostTask(FROM_HERE, base::Bind(callback, false));
    return;
  }
#endif

  Bind(std::move(request));
  main_thread_task_runner_->PostTask(FROM_HERE, base::Bind(callback, true));
  is_presenting_ = true;
  vr_compositor_->SuspendRendering(false);
}

void OpenVRRenderLoop::ExitPresent() {
  is_presenting_ = false;
  vr_compositor_->SuspendRendering(true);
}

mojom::VRPosePtr OpenVRRenderLoop::GetPose() {
  vr::TrackedDevicePose_t rendering_poses[vr::k_unMaxTrackedDeviceCount];
  vr::TrackedDevicePose_t unused_poses[vr::k_unMaxTrackedDeviceCount];

  TRACE_EVENT0("gpu", "WaitGetPoses");
  vr_compositor_->WaitGetPoses(rendering_poses, vr::k_unMaxTrackedDeviceCount,
                               unused_poses, vr::k_unMaxTrackedDeviceCount);

  return OpenVRPoseToVRPosePtr(rendering_poses[vr::k_unTrackedDeviceIndex_Hmd]);
}

void OpenVRRenderLoop::Init() {
  vr::EVRInitError error;
  vr_compositor_ = reinterpret_cast<vr::IVRCompositor*>(
      vr::VR_GetGenericInterface(vr::IVRCompositor_Version, &error));
  if (error != vr::VRInitError_None) {
    DLOG(ERROR) << "Failed to initialize compositor: "
                << vr::VR_GetVRInitErrorAsEnglishDescription(error);
    return;
  }

  vr_compositor_->SuspendRendering(true);
  vr_compositor_->SetTrackingSpace(
      vr::ETrackingUniverseOrigin::TrackingUniverseSeated);
}

base::WeakPtr<OpenVRRenderLoop> OpenVRRenderLoop::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void OpenVRRenderLoop::GetVSync(
    mojom::VRPresentationProvider::GetVSyncCallback callback) {
  int16_t frame = next_frame_id_++;
  DCHECK(is_presenting_);

  vr::Compositor_FrameTiming timing;
  timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
  bool valid_time = vr_compositor_->GetFrameTiming(&timing);
  base::TimeDelta time =
      valid_time ? base::TimeDelta::FromSecondsD(timing.m_flSystemTimeInSeconds)
                 : base::TimeDelta();

  mojom::VRPosePtr pose = GetPose();
  std::move(callback).Run(std::move(pose), time, frame,
                          mojom::VRPresentationProvider::VSyncStatus::SUCCESS);
}

}  // namespace device
