// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/dualshock4_controller_win.h"

#include <Unknwn.h>
#include <WinDef.h>
#include <stdint.h>
#include <windows.h>

namespace device {

Dualshock4ControllerWin::Dualshock4ControllerWin(HANDLE device_handle) {
  UINT size;
  UINT result =
      ::GetRawInputDeviceInfo(device_handle, RIDI_DEVICENAME, nullptr, &size);
  if (result == 0U) {
    std::unique_ptr<wchar_t[]> name_buffer(new wchar_t[size]);
    result = ::GetRawInputDeviceInfo(device_handle, RIDI_DEVICENAME,
                                     name_buffer.get(), &size);
    if (result == size) {
      hid_handle_.Set(::CreateFile(name_buffer.get(),
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_EXISTING, 0U, nullptr));
    }
  }
}

Dualshock4ControllerWin::~Dualshock4ControllerWin() = default;

void Dualshock4ControllerWin::WriteOutputReport(void* report,
                                                size_t report_length) {
  if (!hid_handle_.IsValid())
    return;
  DWORD bytes_written;
  ::WriteFile(hid_handle_.Get(), report, report_length, &bytes_written,
              nullptr);
}

}  // namespace device
