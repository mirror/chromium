// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_WIN_H_
#define DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_WIN_H_

#include <memory>

#include "build/build_config.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Unknwn.h>
#include <WinDef.h>
#include <XInput.h>
#include <stdlib.h>
#include <windows.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/scoped_native_library.h"
#include "device/gamepad/gamepad_data_fetcher.h"
#include "device/gamepad/gamepad_standard_mappings.h"
#include "device/gamepad/public/cpp/gamepads.h"

namespace device {

class GamepadPlatformDataFetcherWin : public GamepadDataFetcher {
 public:
  typedef GamepadDataFetcherFactoryImpl<GamepadPlatformDataFetcherWin,
                                        GAMEPAD_SOURCE_WIN_XINPUT>
      Factory;

  GamepadPlatformDataFetcherWin();
  ~GamepadPlatformDataFetcherWin() override;

  GamepadSource source() override;

  // GamepadDataFetcher implementation.
  void GetGamepadData(bool devices_changed_hint) override;

  void PlayEffect(
      int pad_index,
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback) override;

  void ResetVibration(
      int pad_index,
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback) override;

 private:
  void OnAddedToProvider() override;

  // The three function types we use from xinput1_3.dll.
  typedef void(WINAPI* XInputEnableFunc)(BOOL enable);
  typedef DWORD(WINAPI* XInputGetCapabilitiesFunc)(
      DWORD dwUserIndex,
      DWORD dwFlags,
      XINPUT_CAPABILITIES* pCapabilities);
  typedef DWORD(WINAPI* XInputGetStateFunc)(DWORD dwUserIndex,
                                            XINPUT_STATE* pState);
  typedef DWORD(WINAPI* XInputSetStateFunc)(DWORD dwUserIndex,
                                            XINPUT_VIBRATION* pVibration);

  // Get functions from dynamically loading the xinput dll.
  // Returns true if loading was successful.
  bool GetXInputDllFunctions();

  // Scan for connected XInput and DirectInput gamepads.
  void EnumerateDevices();
  void GetXInputPadData(int i);

  void PlayDualRumbleEffect(int sequence_id,
                            int pad_index,
                            double duration,
                            double start_delay,
                            double strong_magnitude,
                            double weak_magnitude);
  void StartXInputVibration(int sequence_id,
                            int pad_index,
                            double duration,
                            double strong_magnitude,
                            double weak_magnitude);
  void StopXInputVibration(int sequence_id, int pad_index);
  void SetXInputVibration(int pad_index,
                          double strong_magnitude,
                          double weak_magnitude);

  base::ScopedNativeLibrary xinput_dll_;
  bool xinput_available_;

  // Function pointers to XInput functionality, retrieved in
  // |GetXinputDllFunctions|.
  XInputGetCapabilitiesFunc xinput_get_capabilities_;
  XInputGetStateFunc xinput_get_state_;
  XInputSetStateFunc xinput_set_state_;

  bool xinput_connected_[XUSER_MAX_COUNT];
  bool xinput_ffb_supported_[XUSER_MAX_COUNT];
  int sequence_ids_[XUSER_MAX_COUNT];
  mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback
      pending_callbacks_[XUSER_MAX_COUNT];

  DISALLOW_COPY_AND_ASSIGN(GamepadPlatformDataFetcherWin);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_PLATFORM_DATA_FETCHER_WIN_H_
