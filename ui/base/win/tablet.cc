// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/tablet.h"

#include <cfgmgr32.h>
#include <initguid.h>
#include <mdmregistration.h>
#include <powrprof.h>
#include <setupapi.h>
#include <shobjidl.h>
#include <tchar.h>     // Must be before tpcshrd.h or for any use of _T macro
#include <uiviewsettingsinterop.h>
#include <windows.ui.viewmanagement.h>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"
#include "ui/base/win/hidden_window.h"

using namespace base::win;

namespace {

// Returns the current platform role. We use the PowerDeterminePlatformRoleEx
// API for that.
POWER_PLATFORM_ROLE GetPlatformRole() {
  return PowerDeterminePlatformRoleEx(POWER_PLATFORM_ROLE_V2);
}

// Uses the Windows 10 WRL API's to query the current system state. The API's
// we are using in the function below are supported in Win32 apps as per msdn.
// It looks like the API implementation is buggy at least on Surface 4 causing
// it to always return UserInteractionMode_Touch which as per documentation
// indicates tablet mode.
bool IsWindows10TabletMode() {
  if (base::win::GetVersion() < VERSION_WIN10)
    return false;

  if (!ResolveCoreWinRTDelayload() ||
    !ScopedHString::ResolveCoreWinRTStringDelayload()) {
    return false;
  }

  ScopedHString view_settings_guid = ScopedHString::Create(
    RuntimeClass_Windows_UI_ViewManagement_UIViewSettings);
  ScopedComPtr<IUIViewSettingsInterop> view_settings_interop;
  HRESULT hr = base::win::RoGetActivationFactory(
    view_settings_guid.get(), IID_PPV_ARGS(&view_settings_interop));
  if (FAILED(hr))
    return false;

  ScopedComPtr<ABI::Windows::UI::ViewManagement::IUIViewSettings> view_settings;
  hr = view_settings_interop->GetForWindow(ui::GetHiddenWindow(),
    IID_PPV_ARGS(&view_settings));
  if (FAILED(hr))
    return false;

  ABI::Windows::UI::ViewManagement::UserInteractionMode mode =
    ABI::Windows::UI::ViewManagement::UserInteractionMode_Mouse;
  view_settings->get_UserInteractionMode(&mode);
  return mode == ABI::Windows::UI::ViewManagement::UserInteractionMode_Touch;
}


}  // namespace

namespace ui {

// Returns true if a physical keyboard is detected on Windows 8 and up.
// Uses the Setup APIs to enumerate the attached keyboards and returns true
// if the keyboard count is 1 or more.. While this will work in most cases
// it won't work if there are devices which expose keyboard interfaces which
// are attached to the machine.
bool IsKeyboardPresentOnSlate(std::string* reason) {
  bool result = false;

  if (base::win::GetVersion() < VERSION_WIN8) {
    *reason = "Detection not supported";
    return false;
  }

  // This function is only supported for Windows 8 and up.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableUsbKeyboardDetect)) {
    if (reason)
      *reason = "Detection disabled";
    return false;
  }

  // This function should be only invoked for machines with touch screens.
  if ((GetSystemMetrics(SM_DIGITIZER) & NID_INTEGRATED_TOUCH) !=
      NID_INTEGRATED_TOUCH) {
    if (reason) {
      *reason += "NID_INTEGRATED_TOUCH\n";
      result = true;
    } else {
      return true;
    }
  }

  // If it is a tablet device we assume that there is no keyboard attached.
  if (IsTabletDevice(reason)) {
    if (reason)
      *reason += "Tablet device.\n";
    return false;
  } else {
    if (reason) {
      *reason += "Not a tablet device";
      result = true;
    } else {
      return true;
    }
  }

  // To determine whether a keyboard is present on the device, we do the
  // following:-
  // 1. Check whether the device supports auto rotation. If it does then
  //    it possibly supports flipping from laptop to slate mode. If it
  //    does not support auto rotation, then we assume it is a desktop
  //    or a normal laptop and assume that there is a keyboard.

  // 2. If the device supports auto rotation, then we get its platform role
  //    and check the system metric SM_CONVERTIBLESLATEMODE to see if it is
  //    being used in slate mode. If yes then we return false here to ensure
  //    that the OSK is displayed.

  // 3. If step 1 and 2 fail then we check attached keyboards and return true
  //    if we find ACPI\* or HID\VID* keyboards.

  typedef BOOL(WINAPI * GetAutoRotationState)(PAR_STATE state);

  GetAutoRotationState get_rotation_state =
      reinterpret_cast<GetAutoRotationState>(::GetProcAddress(
          GetModuleHandle(L"user32.dll"), "GetAutoRotationState"));

  if (get_rotation_state) {
    AR_STATE auto_rotation_state = AR_ENABLED;
    get_rotation_state(&auto_rotation_state);
    if ((auto_rotation_state & AR_NOSENSOR) ||
        (auto_rotation_state & AR_NOT_SUPPORTED)) {
      // If there is no auto rotation sensor or rotation is not supported in
      // the current configuration, then we can assume that this is a desktop
      // or a traditional laptop.
      if (reason) {
        *reason += (auto_rotation_state & AR_NOSENSOR) ? "AR_NOSENSOR\n"
                                                       : "AR_NOT_SUPPORTED\n";
        result = true;
      } else {
        return true;
      }
    }
  }

  const GUID KEYBOARD_CLASS_GUID = {
      0x4D36E96B,
      0xE325,
      0x11CE,
      {0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18}};

  // Query for all the keyboard devices.
  HDEVINFO device_info =
      SetupDiGetClassDevs(&KEYBOARD_CLASS_GUID, NULL, NULL, DIGCF_PRESENT);
  if (device_info == INVALID_HANDLE_VALUE) {
    if (reason)
      *reason += "No keyboard info\n";
    return result;
  }

  // Enumerate all keyboards and look for ACPI\PNP and HID\VID devices. If
  // the count is more than 1 we assume that a keyboard is present. This is
  // under the assumption that there will always be one keyboard device.
  for (DWORD i = 0;; ++i) {
    SP_DEVINFO_DATA device_info_data = {0};
    device_info_data.cbSize = sizeof(device_info_data);
    if (!SetupDiEnumDeviceInfo(device_info, i, &device_info_data))
      break;

    // Get the device ID.
    wchar_t device_id[MAX_DEVICE_ID_LEN];
    CONFIGRET status = CM_Get_Device_ID(device_info_data.DevInst, device_id,
                                        MAX_DEVICE_ID_LEN, 0);
    if (status == CR_SUCCESS) {
      // To reduce the scope of the hack we only look for ACPI and HID\\VID
      // prefixes in the keyboard device ids.
      if (StartsWith(device_id, L"ACPI",
                     base::CompareCase::INSENSITIVE_ASCII) ||
          StartsWith(device_id, L"HID\\VID",
                     base::CompareCase::INSENSITIVE_ASCII)) {
        if (reason) {
          *reason += "device: ";
          *reason += base::WideToUTF8(device_id);
          *reason += '\n';
        }
        // The heuristic we are using is to check the count of keyboards and
        // return true if the API's report one or more keyboards. Please note
        // that this will break for non keyboard devices which expose a
        // keyboard PDO.
        result = true;
      }
    }
  }
  return result;
}

bool IsTabletDevice(std::string* reason) {
  if (base::win::GetVersion() < VERSION_WIN8) {
    if (reason)
      *reason = "Tablet device detection not supported below Windows 8\n";
    return false;
  }

  if (IsWindows10TabletMode())
    return true;

  if (GetSystemMetrics(SM_MAXIMUMTOUCHES) == 0) {
    if (reason) {
      *reason += "Device does not support touch.\n";
    } else {
      return false;
    }
  }

  // If the device is docked, the user is treating the device as a PC.
  if (GetSystemMetrics(SM_SYSTEMDOCKED) != 0) {
    if (reason) {
      *reason += "SM_SYSTEMDOCKED\n";
    } else {
      return false;
    }
  }

  // If the device is not supporting rotation, it's unlikely to be a tablet,
  // a convertible or a detachable.
  // See
  // https://msdn.microsoft.com/en-us/library/windows/desktop/dn629263(v=vs.85).aspx
  typedef decltype(GetAutoRotationState)* GetAutoRotationStateType;
  GetAutoRotationStateType get_auto_rotation_state_func =
      reinterpret_cast<GetAutoRotationStateType>(GetProcAddress(
          GetModuleHandle(L"user32.dll"), "GetAutoRotationState"));

  if (get_auto_rotation_state_func) {
    AR_STATE rotation_state;
    ZeroMemory(&rotation_state, sizeof(AR_STATE));
    if (get_auto_rotation_state_func(&rotation_state)) {
      if ((rotation_state & AR_NOT_SUPPORTED) || (rotation_state & AR_LAPTOP) ||
          (rotation_state & AR_NOSENSOR))
        return false;
    }
  }

  // PlatformRoleSlate was added in Windows 8+.
  POWER_PLATFORM_ROLE role = GetPlatformRole();
  bool mobile_power_profile = (role == PlatformRoleMobile);
  bool slate_power_profile = (role == PlatformRoleSlate);

  bool is_tablet = false;
  if (mobile_power_profile || slate_power_profile) {
    is_tablet = !GetSystemMetrics(SM_CONVERTIBLESLATEMODE);
    if (!is_tablet) {
      if (reason) {
        *reason += "Not in slate mode.\n";
      } else {
        return false;
      }
    } else {
      if (reason) {
        *reason += (role == PlatformRoleMobile) ? "PlatformRoleMobile\n"
                                                : "PlatformRoleSlate\n";
      }
    }
  } else {
    if (reason)
      *reason += "Device role is not mobile or slate.\n";
  }
  return is_tablet;
}

}  // namespace ui