// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/openvr/openvr_device.h"
#include <math.h>
#include "base/memory/ptr_util.h"
#include "base/numerics/math_constants.h"
#include "build/build_config.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "third_party/openvr/src/headers/openvr.h"

#if defined(OS_WIN)
#include "device/vr/windows/d3d11_texture_helper.h"
#endif

namespace device {

namespace {

constexpr float kRadToDeg = static_cast<float>(180 / base::kPiFloat);
constexpr float kDefaultIPD = 0.06f;  // Default average IPD

mojom::VRFieldOfViewPtr OpenVRFovToWebVRFov(vr::IVRSystem* vr_system,
                                            vr::Hmd_Eye eye) {
  auto out = mojom::VRFieldOfView::New();
  float up_tan, down_tan, left_tan, right_tan;
  vr_system->GetProjectionRaw(eye, &left_tan, &right_tan, &up_tan, &down_tan);
  out->upDegrees = -(atanf(up_tan) * kRadToDeg);
  out->downDegrees = atanf(down_tan) * kRadToDeg;
  out->leftDegrees = -(atanf(left_tan) * kRadToDeg);
  out->rightDegrees = atanf(right_tan) * kRadToDeg;
  return out;
}

std::vector<float> HmdVector3ToWebVR(const vr::HmdVector3_t& vec) {
  std::vector<float> out;
  out.resize(3);
  out[0] = vec.v[0];
  out[1] = vec.v[1];
  out[2] = vec.v[2];
  return out;
}

std::string GetOpenVRString(vr::IVRSystem* vr_system,
                            vr::TrackedDeviceProperty prop) {
  std::string out;

  vr::TrackedPropertyError error = vr::TrackedProp_Success;
  char openvr_string[vr::k_unMaxPropertyStringSize];
  vr_system->GetStringTrackedDeviceProperty(
      vr::k_unTrackedDeviceIndex_Hmd, prop, openvr_string,
      vr::k_unMaxPropertyStringSize, &error);

  if (error == vr::TrackedProp_Success)
    out = openvr_string;

  return out;
}

std::vector<float> HmdMatrix34ToWebVRTransformMatrix(
    const vr::HmdMatrix34_t& mat) {
  std::vector<float> transform;
  transform.resize(16);
  transform[0] = mat.m[0][0];
  transform[1] = mat.m[1][0];
  transform[2] = mat.m[2][0];
  transform[3] = 0.0f;
  transform[4] = mat.m[0][1];
  transform[5] = mat.m[1][1];
  transform[6] = mat.m[2][1];
  transform[7] = 0.0f;
  transform[8] = mat.m[0][2];
  transform[9] = mat.m[1][2];
  transform[10] = mat.m[2][2];
  transform[11] = 0.0f;
  transform[12] = mat.m[0][3];
  transform[13] = mat.m[1][3];
  transform[14] = mat.m[2][3];
  transform[15] = 1.0f;
  return transform;
}

mojom::VRDisplayInfoPtr CreateVRDisplayInfo(vr::IVRSystem* vr_system,
                                            unsigned int id) {
  mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
  display_info->index = id;
  display_info->displayName =
      GetOpenVRString(vr_system, vr::Prop_ManufacturerName_String) + " " +
      GetOpenVRString(vr_system, vr::Prop_ModelNumber_String);
  display_info->capabilities = mojom::VRDisplayCapabilities::New();
  display_info->capabilities->hasPosition = true;
  display_info->capabilities->hasExternalDisplay = true;
  display_info->capabilities->canPresent = true;

  display_info->leftEye = mojom::VREyeParameters::New();
  display_info->rightEye = mojom::VREyeParameters::New();
  mojom::VREyeParametersPtr& left_eye = display_info->leftEye;
  mojom::VREyeParametersPtr& right_eye = display_info->rightEye;

  left_eye->fieldOfView = OpenVRFovToWebVRFov(vr_system, vr::Eye_Left);
  right_eye->fieldOfView = OpenVRFovToWebVRFov(vr_system, vr::Eye_Right);

  vr::TrackedPropertyError error = vr::TrackedProp_Success;
  float ipd = vr_system->GetFloatTrackedDeviceProperty(
      vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_UserIpdMeters_Float, &error);

  if (error != vr::TrackedProp_Success)
    ipd = kDefaultIPD;

  left_eye->offset.resize(3);
  left_eye->offset[0] = -ipd * 0.5;
  left_eye->offset[1] = 0.0f;
  left_eye->offset[2] = 0.0f;
  right_eye->offset.resize(3);
  right_eye->offset[0] = ipd * 0.5;
  right_eye->offset[1] = 0.0;
  right_eye->offset[2] = 0.0;

  uint32_t width, height;
  vr_system->GetRecommendedRenderTargetSize(&width, &height);
  left_eye->renderWidth = width;
  left_eye->renderHeight = height;
  right_eye->renderWidth = left_eye->renderWidth;
  right_eye->renderHeight = left_eye->renderHeight;

  display_info->stageParameters = mojom::VRStageParameters::New();
  vr::HmdMatrix34_t mat =
      vr_system->GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
  display_info->stageParameters->standingTransform =
      HmdMatrix34ToWebVRTransformMatrix(mat);

  vr::IVRChaperone* chaperone = vr::VRChaperone();
  if (chaperone) {
    chaperone->GetPlayAreaSize(&display_info->stageParameters->sizeX,
                               &display_info->stageParameters->sizeZ);
  } else {
    display_info->stageParameters->sizeX = 0.0f;
    display_info->stageParameters->sizeZ = 0.0f;
  }

  return display_info;
}

class OpenVRRenderLoop : public base::Thread, mojom::VRPresentationProvider {
 public:
  OpenVRRenderLoop(vr::IVRSystem* vr);

  void RegisterPollingEventCallback(
      const base::Callback<void()>& on_polling_events);
  void UnregisterPollingEventCallback();
  void Bind(mojom::VRPresentationProviderRequest request);
  mojom::VRPosePtr GetPose(bool presenting);
  void RequestPresent(mojom::VRPresentationProviderRequest request,
                      base::Callback<void(bool)> callback);
  void ExitPresent();
  base::WeakPtr<OpenVRRenderLoop> GetWeakPtr();

  // VRPresentationProvider overrides:
  void SubmitFrame(int16_t frame_index,
                   const gpu::MailboxHolder& mailbox) override;
  void SubmitFrameWithMemoryBuffer(mojo::ScopedHandle texture_handle,
                                   int16_t frame_index) override;
  void UpdateLayerBounds(int16_t frame_id,
                         const gfx::RectF& left_bounds,
                         const gfx::RectF& right_bounds,
                         const gfx::Size& source_size) override;
  void GetVSync(GetVSyncCallback callback) override;

 private:
  void Initialize();

#if defined(OS_WIN)
  D3D11TextureHelper texture_helper_;
#endif

  int16_t next_frame_id_ = 0;
  bool is_presenting_ = false;
  gfx::RectF left_bounds_;
  gfx::RectF right_bounds_;
  gfx::Size source_size_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  base::Callback<void()> on_polling_events_;
  vr::IVRSystem* vr_system_;
  vr::IVRCompositor* vr_compositor_;
  mojo::Binding<mojom::VRPresentationProvider> binding_;
  base::WeakPtrFactory<OpenVRRenderLoop> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OpenVRRenderLoop);
};

OpenVRRenderLoop::OpenVRRenderLoop(vr::IVRSystem* vr_system)
    : base::Thread("OpenVRRenderLoop"),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      vr_system_(vr_system),
      binding_(this),
      weak_ptr_factory_(this) {
  DCHECK(main_thread_task_runner_);
  Initialize();
}

void OpenVRRenderLoop::Bind(mojom::VRPresentationProviderRequest request) {
  binding_.Close();
  binding_.Bind(std::move(request));
}

void OpenVRRenderLoop::SubmitFrame(int16_t frame_index,
                                   const gpu::MailboxHolder& mailbox) {
  NOTREACHED();
}

void OpenVRRenderLoop::SubmitFrameWithMemoryBuffer(
    mojo::ScopedHandle texture_handle,
    int16_t frame_index) {
  TRACE_EVENT1("gpu", "SubmitFrameWithMemoryBuffer", "frameIndex", frame_index);

#if defined(OS_WIN)
  texture_helper_.SetSourceTexture(std::move(texture_handle));
  texture_helper_.AllocateBackBuffer();
  texture_helper_.CopyTextureToBackBuffer(true);

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
  if (error != vr::VRCompositorError_None)
    ExitPresent();
#endif
}

void OpenVRRenderLoop::UpdateLayerBounds(int16_t frame_id,
                                         const gfx::RectF& left_bounds,
                                         const gfx::RectF& right_bounds,
                                         const gfx::Size& source_size) {
  left_bounds_ = left_bounds;
  right_bounds_ = right_bounds;
  source_size_ = source_size;
};

void OpenVRRenderLoop::RequestPresent(
    mojom::VRPresentationProviderRequest request,
    base::Callback<void(bool)> callback) {
  Bind(std::move(request));
  main_thread_task_runner_->PostTask(FROM_HERE, base::Bind(callback, true));
  is_presenting_ = true;
  vr_compositor_->SuspendRendering(false);
}

void OpenVRRenderLoop::ExitPresent() {
  is_presenting_ = false;
  vr_compositor_->ClearLastSubmittedFrame();
  vr_compositor_->SuspendRendering(true);
}

mojom::VRPosePtr OpenVRRenderLoop::GetPose(bool presenting) {
  TRACE_EVENT1("gpu", "getPose", "isPresenting", presenting);
  device::mojom::VRPosePtr pose = device::mojom::VRPose::New();
  pose->orientation.emplace(4);

  pose->orientation.value()[0] = 0;
  pose->orientation.value()[1] = 0;
  pose->orientation.value()[2] = 0;
  pose->orientation.value()[3] = 1;

  pose->position.emplace(3);
  pose->position.value()[0] = 0;
  pose->position.value()[1] = 0;
  pose->position.value()[2] = 0;

  vr::TrackedDevicePose_t rendering_poses[vr::k_unMaxTrackedDeviceCount];
  vr::TrackedDevicePose_t unused_poses[vr::k_unMaxTrackedDeviceCount];

  if (presenting) {
    TRACE_EVENT0("gpu", "WaitGetPoses");
    vr_compositor_->WaitGetPoses(rendering_poses, vr::k_unMaxTrackedDeviceCount,
                                 unused_poses, vr::k_unMaxTrackedDeviceCount);
  } else {
    vr_system_->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseSeated,
                                                0.0f, rendering_poses,
                                                vr::k_unMaxTrackedDeviceCount);
  }

  const auto& hmdPose = rendering_poses[vr::k_unTrackedDeviceIndex_Hmd];
  if (hmdPose.bPoseIsValid && hmdPose.bDeviceIsConnected) {
    const float(&m)[3][4] = hmdPose.mDeviceToAbsoluteTracking.m;

    float w = sqrt(1 + m[0][0] + m[1][1] + m[2][2]) / 2;
    pose->orientation.value()[0] = (m[2][1] - m[1][2]) / (4 * w);
    pose->orientation.value()[1] = (m[0][2] - m[2][0]) / (4 * w);
    pose->orientation.value()[2] = (m[1][0] - m[0][1]) / (4 * w);
    pose->orientation.value()[3] = w;

    pose->position.value()[0] = m[0][3];
    pose->position.value()[1] = m[1][3];
    pose->position.value()[2] = m[2][3];

    pose->linearVelocity = HmdVector3ToWebVR(hmdPose.vVelocity);
    pose->angularVelocity = HmdVector3ToWebVR(hmdPose.vAngularVelocity);
  }

  return pose;
}

void OpenVRRenderLoop::Initialize() {
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

  Start();
}

base::WeakPtr<OpenVRRenderLoop> OpenVRRenderLoop::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace

OpenVRDevice::OpenVRDevice(vr::IVRSystem* vr)
    : vr_system_(vr), is_polling_events_(false), weak_ptr_factory_(this) {
  DCHECK(vr_system_);
  SetVRDisplayInfo(CreateVRDisplayInfo(vr_system_, GetId()));

  render_loop_ = std::make_unique<OpenVRRenderLoop>(vr_system_);
  render_loop_->RegisterPollingEventCallback(base::Bind(
      &OpenVRDevice::OnPollingEvents, weak_ptr_factory_.GetWeakPtr()));
}
OpenVRDevice::~OpenVRDevice() {}

void OpenVRDevice::RequestPresent(
    VRDisplayImpl* display,
    mojom::VRSubmitFrameClientPtr submit_client,
    mojom::VRPresentationProviderRequest request,
    mojom::VRDisplayHost::RequestPresentCallback callback) {
  // Gets us back on this thread to call the callback
  base::Callback<void(bool)> my_callback =
      base::Bind(&OpenVRDevice::OnRequestPresentResult,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback));

  // bind to OpenVRRenderLoop
  render_loop_->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&OpenVRRenderLoop::RequestPresent, render_loop_->GetWeakPtr(),
                 base::Passed(&request), base::Passed(&my_callback)));
}

void OpenVRDevice::OnRequestPresentResult(
    mojom::VRDisplayHost::RequestPresentCallback callback,
    bool result) {
  std::move(callback).Run(result);
}

void OpenVRDevice::ExitPresent() {
  render_loop_->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&OpenVRRenderLoop::ExitPresent, render_loop_->GetWeakPtr()));
}

void OpenVRDevice::GetPose(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  std::move(callback).Run(render_loop_->GetPose(false));
}

// Only deal with events that will cause displayInfo changes for now.
void OpenVRDevice::OnPollingEvents() {
  if (!vr_system_ || is_polling_events_)
    return;

  // Polling events through vr_system_ has started.
  is_polling_events_ = true;

  vr::VREvent_t event;
  bool is_changed = false;
  while (vr_system_->PollNextEvent(&event, sizeof(event))) {
    if (event.trackedDeviceIndex != vr::k_unTrackedDeviceIndex_Hmd &&
        event.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
      continue;
    }

    switch (event.eventType) {
      case vr::VREvent_TrackedDeviceUpdated:
      case vr::VREvent_IpdChanged:
      case vr::VREvent_ChaperoneDataHasChanged:
      case vr::VREvent_ChaperoneSettingsHaveChanged:
      case vr::VREvent_ChaperoneUniverseHasChanged:
        is_changed = true;
        break;

      default:
        break;
    }
  }

  // Polling events through vr_system_ has finished.
  is_polling_events_ = false;

  if (is_changed)
    SetVRDisplayInfo(CreateVRDisplayInfo(vr_system_, GetId()));
}

// Register a callback function to deal with system events.
void OpenVRRenderLoop::RegisterPollingEventCallback(
    const base::Callback<void()>& on_polling_events) {
  if (on_polling_events.is_null())
    return;

  on_polling_events_ = on_polling_events;
}

void OpenVRRenderLoop::UnregisterPollingEventCallback() {
  on_polling_events_.Reset();
}

void OpenVRRenderLoop::GetVSync(
    mojom::VRPresentationProvider::GetVSyncCallback callback) {
  int16_t frame = next_frame_id_++;
  DCHECK(is_presenting_);

  // VSync could be used as a signal to poll events.
  if (!on_polling_events_.is_null()) {
    main_thread_task_runner_->PostTask(FROM_HERE, on_polling_events_);
  }

  vr::Compositor_FrameTiming timing;
  timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
  bool valid_time = vr_compositor_->GetFrameTiming(&timing);
  base::TimeDelta time =
      valid_time ? base::TimeDelta::FromSecondsD(timing.m_flSystemTimeInSeconds)
                 : base::TimeDelta();

  device::mojom::VRPosePtr pose = GetPose(is_presenting_);
  std::move(callback).Run(
      std::move(pose), time, frame,
      device::mojom::VRPresentationProvider::VSyncStatus::SUCCESS);
}

}  // namespace device
