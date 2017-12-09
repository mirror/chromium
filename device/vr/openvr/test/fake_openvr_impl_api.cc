// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "third_party/openvr/src/headers/openvr.h"
#include "third_party/openvr/src/src/ivrclientcore.h"

namespace vr {

class TestVRSystem : public IVRSystem {
 public:
  void GetRecommendedRenderTargetSize(uint32_t* pnWidth,
                                      uint32_t* pnHeight) override;
  HmdMatrix44_t GetProjectionMatrix(EVREye eEye,
                                    float fNearZ,
                                    float fFarZ) override {
    NOTREACHED();
    return {};
  };
  void GetProjectionRaw(EVREye eEye,
                        float* pfLeft,
                        float* pfRight,
                        float* pfTop,
                        float* pfBottom) override;
  bool ComputeDistortion(
      EVREye eEye,
      float fU,
      float fV,
      DistortionCoordinates_t* pDistortionCoordinates) override {
    NOTREACHED();
    return false;
  }
  HmdMatrix34_t GetEyeToHeadTransform(EVREye eEye) override;
  bool GetTimeSinceLastVsync(float* pfSecondsSinceLastVsync,
                             uint64_t* pulFrameCounter) override {
    NOTREACHED();
    return false;
  }
  int32_t GetD3D9AdapterIndex() override {
    NOTREACHED();
    return 0;
  }
  void GetDXGIOutputInfo(int32_t* pnAdapterIndex) override { NOTREACHED(); }
  bool IsDisplayOnDesktop() override {
    NOTREACHED();
    return false;
  }
  bool SetDisplayVisibility(bool bIsVisibleOnDesktop) override {
    NOTREACHED();
    return false;
  }
  void GetDeviceToAbsoluteTrackingPose(
      ETrackingUniverseOrigin eOrigin,
      float fPredictedSecondsToPhotonsFromNow,
      VR_ARRAY_COUNT(unTrackedDevicePoseArrayCount)
          TrackedDevicePose_t* pTrackedDevicePoseArray,
      uint32_t unTrackedDevicePoseArrayCount) override;
  void ResetSeatedZeroPose() override { NOTREACHED(); }
  HmdMatrix34_t GetSeatedZeroPoseToStandingAbsoluteTrackingPose() override;
  HmdMatrix34_t GetRawZeroPoseToStandingAbsoluteTrackingPose() override {
    NOTREACHED();
    return {};
  }
  uint32_t GetSortedTrackedDeviceIndicesOfClass(
      ETrackedDeviceClass eTrackedDeviceClass,
      VR_ARRAY_COUNT(unTrackedDeviceIndexArrayCount)
          TrackedDeviceIndex_t* punTrackedDeviceIndexArray,
      uint32_t unTrackedDeviceIndexArrayCount,
      TrackedDeviceIndex_t unRelativeToTrackedDeviceIndex =
          k_unTrackedDeviceIndex_Hmd) override {
    NOTREACHED();
    return 0;
  }
  EDeviceActivityLevel GetTrackedDeviceActivityLevel(
      TrackedDeviceIndex_t unDeviceId) override {
    NOTREACHED();
    return k_EDeviceActivityLevel_Unknown;
  }  // todo: should use this
  void ApplyTransform(TrackedDevicePose_t* pOutputPose,
                      const TrackedDevicePose_t* pTrackedDevicePose,
                      const HmdMatrix34_t* pTransform) override {
    NOTREACHED();
  }
  TrackedDeviceIndex_t GetTrackedDeviceIndexForControllerRole(
      ETrackedControllerRole unDeviceType) override {
    NOTREACHED();
    return 0;
  }
  ETrackedControllerRole GetControllerRoleForTrackedDeviceIndex(
      TrackedDeviceIndex_t unDeviceIndex) override {
    NOTREACHED();
    return TrackedControllerRole_Invalid;
  }
  ETrackedDeviceClass GetTrackedDeviceClass(
      TrackedDeviceIndex_t unDeviceIndex) override {
    NOTREACHED();
    return TrackedDeviceClass_Invalid;
  }
  bool IsTrackedDeviceConnected(TrackedDeviceIndex_t unDeviceIndex) override {
    NOTREACHED();
    return false;
  }  // todo: should use this
  bool GetBoolTrackedDeviceProperty(
      TrackedDeviceIndex_t unDeviceIndex,
      ETrackedDeviceProperty prop,
      ETrackedPropertyError* pError = 0L) override {
    NOTREACHED();
    return false;
  }
  float GetFloatTrackedDeviceProperty(
      TrackedDeviceIndex_t unDeviceIndex,
      ETrackedDeviceProperty prop,
      ETrackedPropertyError* pError = 0L) override;
  int32_t GetInt32TrackedDeviceProperty(
      TrackedDeviceIndex_t unDeviceIndex,
      ETrackedDeviceProperty prop,
      ETrackedPropertyError* pError = 0L) override {
    NOTREACHED();
    return 0;
  }
  uint64_t GetUint64TrackedDeviceProperty(
      TrackedDeviceIndex_t unDeviceIndex,
      ETrackedDeviceProperty prop,
      ETrackedPropertyError* pError = 0L) override {
    NOTREACHED();
    return 0;
  }
  HmdMatrix34_t GetMatrix34TrackedDeviceProperty(
      TrackedDeviceIndex_t unDeviceIndex,
      ETrackedDeviceProperty prop,
      ETrackedPropertyError* pError = 0L) override {
    NOTREACHED();
    return {};
  }
  uint32_t GetStringTrackedDeviceProperty(
      TrackedDeviceIndex_t unDeviceIndex,
      ETrackedDeviceProperty prop,
      VR_OUT_STRING() char* pchValue,
      uint32_t unBufferSize,
      ETrackedPropertyError* pError = 0L) override;
  const char* GetPropErrorNameFromEnum(ETrackedPropertyError error) override {
    NOTREACHED();
    return nullptr;
  }
  bool PollNextEvent(VREvent_t* pEvent, uint32_t uncbVREvent)
      override;  // I believe we do poll events.
  bool PollNextEventWithPose(ETrackingUniverseOrigin eOrigin,
                             VREvent_t* pEvent,
                             uint32_t uncbVREvent,
                             TrackedDevicePose_t* pTrackedDevicePose) override {
    NOTREACHED();
    return false;
  }
  const char* GetEventTypeNameFromEnum(EVREventType eType) override {
    NOTREACHED();
    return nullptr;
  }
  HiddenAreaMesh_t GetHiddenAreaMesh(
      EVREye eEye,
      EHiddenAreaMeshType type = k_eHiddenAreaMesh_Standard) override {
    NOTREACHED();
    return {};
  }  // todo: should be calling this
  bool GetControllerState(TrackedDeviceIndex_t unControllerDeviceIndex,
                          VRControllerState_t* pControllerState,
                          uint32_t unControllerStateSize) override {
    NOTREACHED();
    return false;
  }  // should be calling this?
  bool GetControllerStateWithPose(
      ETrackingUniverseOrigin eOrigin,
      TrackedDeviceIndex_t unControllerDeviceIndex,
      VRControllerState_t* pControllerState,
      uint32_t unControllerStateSize,
      TrackedDevicePose_t* pTrackedDevicePose) override {
    NOTREACHED();
    return false;
  }  // should be calling this?
  void TriggerHapticPulse(TrackedDeviceIndex_t unControllerDeviceIndex,
                          uint32_t unAxisId,
                          unsigned short usDurationMicroSec) override {
    NOTREACHED();
  }
  const char* GetButtonIdNameFromEnum(EVRButtonId eButtonId) override {
    NOTREACHED();
    return nullptr;
  };
  const char* GetControllerAxisTypeNameFromEnum(
      EVRControllerAxisType eAxisType) override {
    NOTREACHED();
    return nullptr;
  }
  bool CaptureInputFocus() override {
    NOTREACHED();
    return false;
  }  // we should use this?
  void ReleaseInputFocus() override { NOTREACHED(); }
  bool IsInputFocusCapturedByAnotherProcess() override {
    NOTREACHED();
    return false;
  }
  uint32_t DriverDebugRequest(TrackedDeviceIndex_t unDeviceIndex,
                              const char* pchRequest,
                              char* pchResponseBuffer,
                              uint32_t unResponseBufferSize) override {
    NOTREACHED();
    return 0;
  }
  EVRFirmwareError PerformFirmwareUpdate(
      TrackedDeviceIndex_t unDeviceIndex) override {
    NOTREACHED();
    return VRFirmwareError_None;
  }
  void AcknowledgeQuit_Exiting() override {
    NOTREACHED();
  }  // shouldn't exit chrome because vr exits, but should shut down vr
  void AcknowledgeQuit_UserPrompt() override { NOTREACHED(); }
};

class TestVRCompositor : public IVRCompositor {
 public:
  void SetTrackingSpace(ETrackingUniverseOrigin eOrigin) override;
  ETrackingUniverseOrigin GetTrackingSpace() override {
    NOTREACHED();
    return TrackingUniverseSeated;
  }
  EVRCompositorError WaitGetPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount)
                                      TrackedDevicePose_t* pRenderPoseArray,
                                  uint32_t unRenderPoseArrayCount,
                                  VR_ARRAY_COUNT(unGamePoseArrayCount)
                                      TrackedDevicePose_t* pGamePoseArray,
                                  uint32_t unGamePoseArrayCount) override;
  EVRCompositorError GetLastPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount)
                                      TrackedDevicePose_t* pRenderPoseArray,
                                  uint32_t unRenderPoseArrayCount,
                                  VR_ARRAY_COUNT(unGamePoseArrayCount)
                                      TrackedDevicePose_t* pGamePoseArray,
                                  uint32_t unGamePoseArrayCount) override {
    NOTREACHED();
    return VRCompositorError_None;
  }
  EVRCompositorError GetLastPoseForTrackedDeviceIndex(
      TrackedDeviceIndex_t unDeviceIndex,
      TrackedDevicePose_t* pOutputPose,
      TrackedDevicePose_t* pOutputGamePose) override {
    NOTREACHED();
    return VRCompositorError_None;
  }
  EVRCompositorError Submit(
      EVREye eEye,
      const Texture_t* pTexture,
      const VRTextureBounds_t* pBounds = 0,
      EVRSubmitFlags nSubmitFlags = Submit_Default) override;
  void ClearLastSubmittedFrame() override { NOTREACHED(); }  // reached?
  void PostPresentHandoff() override;
  bool GetFrameTiming(Compositor_FrameTiming* pTiming,
                      uint32_t unFramesAgo = 0) override;
  uint32_t GetFrameTimings(Compositor_FrameTiming* pTiming,
                           uint32_t nFrames) override {
    NOTREACHED();
    return 0;
  }
  float GetFrameTimeRemaining() override {
    NOTREACHED();
    return 0;
  }
  void GetCumulativeStats(Compositor_CumulativeStats* pStats,
                          uint32_t nStatsSizeInBytes) override {
    NOTREACHED();
  }
  void FadeToColor(float fSeconds,
                   float fRed,
                   float fGreen,
                   float fBlue,
                   float fAlpha,
                   bool bBackground = false) override {
    NOTREACHED();
  }
  HmdColor_t GetCurrentFadeColor(bool bBackground = false) override {
    NOTREACHED();
    return {};
  }
  void FadeGrid(float fSeconds, bool bFadeIn) override { NOTREACHED(); }
  float GetCurrentGridAlpha() override {
    NOTREACHED();
    return 0;
  }
  EVRCompositorError SetSkyboxOverride(VR_ARRAY_COUNT(unTextureCount)
                                           const Texture_t* pTextures,
                                       uint32_t unTextureCount) override {
    NOTREACHED();
    return VRCompositorError_None;
  }
  void ClearSkyboxOverride() override { NOTREACHED(); }
  void CompositorBringToFront() override { NOTREACHED(); }
  void CompositorGoToBack() override { NOTREACHED(); }
  void CompositorQuit() override { NOTREACHED(); }
  bool IsFullscreen() override {
    NOTREACHED();
    return false;
  }
  uint32_t GetCurrentSceneFocusProcess() override {
    NOTREACHED();
    return 0;
  }
  uint32_t GetLastFrameRenderer() override {
    NOTREACHED();
    return 0;
  }
  bool CanRenderScene() override {
    NOTREACHED();
    return false;
  }  // should we call this?
  void ShowMirrorWindow() override {
    NOTREACHED();
  }  // should we enable this for debugging / in dev tools?
  void HideMirrorWindow() override { NOTREACHED(); }
  bool IsMirrorWindowVisible() override {
    NOTREACHED();
    return false;
  }
  void CompositorDumpImages() override { NOTREACHED(); }
  bool ShouldAppRenderWithLowResources() override {
    NOTREACHED();
    return false;
  }  // should we call this?
  void ForceInterleavedReprojectionOn(bool bOverride) override {
    NOTREACHED();
  }  // should we call this?
  void ForceReconnectProcess() override {
    NOTREACHED();
  }  // any reason to do this?
  void SuspendRendering(bool bSuspend) override;
  EVRCompositorError GetMirrorTextureD3D11(
      EVREye eEye,
      void* pD3D11DeviceOrResource,
      void** ppD3D11ShaderResourceView) override {
    NOTREACHED();
    return VRCompositorError_None;
  }
  void ReleaseMirrorTextureD3D11(void* pD3D11ShaderResourceView) override {
    NOTREACHED();
  }
  EVRCompositorError GetMirrorTextureGL(
      EVREye eEye,
      glUInt_t* pglTextureId,
      glSharedTextureHandle_t* pglSharedTextureHandle) override {
    NOTREACHED();
    return VRCompositorError_None;
  }
  bool ReleaseSharedGLTexture(
      glUInt_t glTextureId,
      glSharedTextureHandle_t glSharedTextureHandle) override {
    NOTREACHED();
    return false;
  }
  void LockGLSharedTextureForAccess(
      glSharedTextureHandle_t glSharedTextureHandle) override {
    NOTREACHED();
  }
  void UnlockGLSharedTextureForAccess(
      glSharedTextureHandle_t glSharedTextureHandle) override {
    NOTREACHED();
  }
  uint32_t GetVulkanInstanceExtensionsRequired(VR_OUT_STRING() char* pchValue,
                                               uint32_t unBufferSize) override {
    NOTREACHED();
    return 0;
  }
  uint32_t GetVulkanDeviceExtensionsRequired(
      VkPhysicalDevice_T* pPhysicalDevice,
      VR_OUT_STRING() char* pchValue,
      uint32_t unBufferSize) override {
    NOTREACHED();
    return 0;
  }
};

class TestVRClientCore : public IVRClientCore {
 public:
  EVRInitError Init(EVRApplicationType eApplicationType) override;
  void Cleanup() override;
  EVRInitError IsInterfaceVersionValid(
      const char* pchInterfaceVersion) override;
  void* GetGenericInterface(const char* pchNameAndVersion,
                            EVRInitError* peError) override;
  bool BIsHmdPresent() override;
  const char* GetEnglishStringForHmdError(EVRInitError eError) override {
    NOTREACHED();
    return nullptr;
  }
  const char* GetIDForVRInitError(EVRInitError eError) override {
    NOTREACHED();
    return nullptr;
  }
};

TestVRSystem g_system;
TestVRCompositor g_compositor;
TestVRClientCore g_loader;

// Test methods (things tests may want to control):
EVRInitError TestVRClientCore::Init(EVRApplicationType eApplicationType) {
  return VRInitError_None;
}

void TestVRClientCore::Cleanup() {}

EVRInitError TestVRClientCore::IsInterfaceVersionValid(
    const char* pchInterfaceVersion) {
  return VRInitError_None;
}

void* TestVRClientCore::GetGenericInterface(const char* pchNameAndVersion,
                                            EVRInitError* peError) {
  *peError = VRInitError_None;
  if (strcmp(pchNameAndVersion, IVRSystem_Version) == 0)
    return (IVRSystem*)(&g_system);
  if (strcmp(pchNameAndVersion, IVRCompositor_Version) == 0)
    return (IVRCompositor*)(&g_compositor);

  *peError = VRInitError_Init_InvalidInterface;
  return nullptr;
}

bool TestVRClientCore::BIsHmdPresent() {
  return true;
}

void TestVRSystem::GetRecommendedRenderTargetSize(uint32_t* pnWidth,
                                                  uint32_t* pnHeight) {
  *pnWidth = 1024;
  *pnHeight = 768;
}

void TestVRSystem::GetProjectionRaw(EVREye eEye,
                                    float* pfLeft,
                                    float* pfRight,
                                    float* pfTop,
                                    float* pfBottom) {
  *pfLeft = 1;
  *pfRight = 1;
  *pfTop = 1;
  *pfBottom = 1;
}

HmdMatrix34_t TestVRSystem::GetEyeToHeadTransform(EVREye eEye) {
  HmdMatrix34_t ret = {};
  ret.m[0][0] = 1;
  ret.m[1][1] = 1;
  ret.m[2][2] = 1;
  ret.m[0][3] = (eEye == Eye_Left) ? 0.1f : -0.1f;
  return ret;
}

void TestVRSystem::GetDeviceToAbsoluteTrackingPose(
    ETrackingUniverseOrigin eOrigin,
    float fPredictedSecondsToPhotonsFromNow,
    VR_ARRAY_COUNT(unTrackedDevicePoseArrayCount)
        TrackedDevicePose_t* pTrackedDevicePoseArray,
    uint32_t unTrackedDevicePoseArrayCount) {
  TrackedDevicePose_t pose = {};
  pose.mDeviceToAbsoluteTracking.m[0][0] = 1;
  pose.mDeviceToAbsoluteTracking.m[1][1] = 1;
  pose.mDeviceToAbsoluteTracking.m[2][2] = 1;
  pose.mDeviceToAbsoluteTracking.m[0][2] = 5;

  pose.vVelocity = {0, 0, 0};
  pose.vAngularVelocity = {0, 0, 0};
  pose.eTrackingResult = TrackingResult_Running_OK;
  pose.bPoseIsValid = true;
  pose.bDeviceIsConnected = true;
  pTrackedDevicePoseArray[0] = pose;

  // todo: set poses for other members of array
  for (unsigned int i = 1; i < unTrackedDevicePoseArrayCount; ++i) {
    TrackedDevicePose_t pose = {};
    pTrackedDevicePoseArray[i] = pose;
  }
}

bool TestVRSystem::PollNextEvent(VREvent_t*, unsigned int) {
  return false;
}

uint32_t TestVRSystem::GetStringTrackedDeviceProperty(
    TrackedDeviceIndex_t unDeviceIndex,
    ETrackedDeviceProperty prop,
    VR_OUT_STRING() char* pchValue,
    uint32_t unBufferSize,
    ETrackedPropertyError* pError) {
  if (pError) {
    *pError = TrackedProp_Success;
  }
  sprintf_s(pchValue, unBufferSize, "test-value");
  return 11;
}

float TestVRSystem::GetFloatTrackedDeviceProperty(
    TrackedDeviceIndex_t unDeviceIndex,
    ETrackedDeviceProperty prop,
    ETrackedPropertyError* pError) {
  if (pError) {
    *pError = TrackedProp_Success;
  }
  switch (prop) {
    case Prop_UserIpdMeters_Float:
      return 0.6f;
    default:
      NOTREACHED();
  }
  return 0;
}

HmdMatrix34_t TestVRSystem::GetSeatedZeroPoseToStandingAbsoluteTrackingPose() {
  HmdMatrix34_t ret = {};
  ret.m[0][0] = 1;
  ret.m[1][1] = 1;
  ret.m[2][2] = 1;
  ret.m[1][3] = 1.2f;
  return ret;
}

void TestVRCompositor::SuspendRendering(bool bSuspend) {}

void TestVRCompositor::SetTrackingSpace(ETrackingUniverseOrigin) {}

EVRCompositorError TestVRCompositor::WaitGetPoses(TrackedDevicePose_t* poses1,
                                                  unsigned int count1,
                                                  TrackedDevicePose_t* poses2,
                                                  unsigned int count2) {
  // Sleep(10);
  if (poses1)
    g_system.GetDeviceToAbsoluteTrackingPose(TrackingUniverseSeated, 0, poses1,
                                             count1);

  if (poses2)
    g_system.GetDeviceToAbsoluteTrackingPose(TrackingUniverseSeated, 0, poses2,
                                             count2);

  return VRCompositorError_None;
}

EVRCompositorError TestVRCompositor::Submit(EVREye,
                                            Texture_t const*,
                                            VRTextureBounds_t const*,
                                            EVRSubmitFlags) {
  return VRCompositorError_None;
}

void TestVRCompositor::PostPresentHandoff() {}

bool TestVRCompositor::GetFrameTiming(Compositor_FrameTiming*, unsigned int) {
  return false;
}

}  // namespace vr

// DLL methods that we need to export:
// expose the vrsystem and compositor

// todo: calling convention?
extern "C" {
__declspec(dllexport) void* VRClientCoreFactory(const char* pInterfaceName,
                                                int* pReturnCode) {
  // todo: consider checking pInterfaceName
  return &vr::g_loader;
}
}

// In addition to this dll, we'll need to deploy some config files:
// CVRPathRegistry_Public::GetPaths
// VR_IsHmdPresent
// VR_IsRuntimeInstalled

// environment variables:
// VR_OVERRIDE: path\to\test, such that path\to\test\bin\vrclient.dll is this
// dll VR_CONFIG_PATH: doesn't appear to be needed, just set to a folder
// VR_LOG_PATH: doesn't appear to be needed, just set to a folder
