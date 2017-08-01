// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/win/color_profile_reader.h"

#include <d3d11_1.h>
#include <stddef.h>
#include <windows.h>

#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/win/scoped_comptr.h"
#include "ui/display/win/display_info.h"
#include "ui/gfx/icc_profile.h"

#if defined(NTDDI_WIN10_RS2)
#define ENABLE_HDR_DETECTION
#include <dxgi1_6.h>
#endif

namespace display {
namespace win {
namespace {

BOOL CALLBACK EnumMonitorCallback(HMONITOR monitor,
                                  HDC input_hdc,
                                  LPRECT rect,
                                  LPARAM data) {
  base::string16 device_name;
  MONITORINFOEX monitor_info;
  ::ZeroMemory(&monitor_info, sizeof(monitor_info));
  monitor_info.cbSize = sizeof(monitor_info);
  ::GetMonitorInfo(monitor, &monitor_info);
  device_name = base::string16(monitor_info.szDevice);

  base::string16 profile_path;
  HDC hdc = ::CreateDC(monitor_info.szDevice, NULL, NULL, NULL);
  if (hdc) {
    DWORD path_length = MAX_PATH;
    WCHAR path[MAX_PATH + 1];
    BOOL result = ::GetICMProfile(hdc, &path_length, path);
    ::DeleteDC(hdc);
    if (result)
      profile_path = base::string16(path);
  }

  std::map<base::string16, base::string16>* device_to_path_map =
      reinterpret_cast<std::map<base::string16, base::string16>*>(data);
  (*device_to_path_map)[device_name] = profile_path;
  return TRUE;
}

}  // namespace

ColorProfileReader::ColorProfileReader(Client* client)
    : client_(client), weak_factory_(this) {}

ColorProfileReader::~ColorProfileReader() {}

void ColorProfileReader::UpdateIfNeeded() {
  UpdateDXGIDisplayIdToColorSpaceMap();

  if (update_in_flight_)
    return;

  DeviceToPathMap new_device_to_path_map = BuildDeviceToPathMap();
  if (device_to_path_map_ == new_device_to_path_map)
    return;

  if (!base::SequencedWorkerPool::IsEnabled())
    return;

  update_in_flight_ = true;
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&ColorProfileReader::ReadProfilesOnBackgroundThread,
                 new_device_to_path_map),
      base::Bind(&ColorProfileReader::ReadProfilesCompleted,
                 weak_factory_.GetWeakPtr()));
}

// static
ColorProfileReader::DeviceToPathMap ColorProfileReader::BuildDeviceToPathMap() {
  DeviceToPathMap device_to_path_map;
  EnumDisplayMonitors(nullptr, nullptr, EnumMonitorCallback,
                      reinterpret_cast<LPARAM>(&device_to_path_map));
  return device_to_path_map;
}

// static
ColorProfileReader::DeviceToDataMap
ColorProfileReader::ReadProfilesOnBackgroundThread(
    DeviceToPathMap new_device_to_path_map) {
  DeviceToDataMap new_device_to_data_map;
  for (auto entry : new_device_to_path_map) {
    const base::string16& device_name = entry.first;
    const base::string16& profile_path = entry.second;
    std::string profile_data;
    base::ReadFileToString(base::FilePath(profile_path), &profile_data);
    new_device_to_data_map[device_name] = profile_data;
  }
  return new_device_to_data_map;
}

void ColorProfileReader::ReadProfilesCompleted(
    DeviceToDataMap device_to_data_map) {
  DCHECK(update_in_flight_);
  update_in_flight_ = false;

  display_id_to_color_space_map_.clear();
  for (auto entry : device_to_data_map) {
    const base::string16& device_name = entry.first;
    const std::string& profile_data = entry.second;
    int64_t display_id =
        DisplayInfo::DeviceIdFromDeviceName(device_name.c_str());

    if (profile_data.empty()) {
      display_id_to_color_space_map_[display_id] = default_color_space_;
    } else {
      display_id_to_color_space_map_[display_id] =
          gfx::ICCProfile::FromData(profile_data.data(), profile_data.size())
              .GetColorSpace();
    }
  }

  client_->OnColorProfilesChanged();
}

const gfx::ColorSpace& ColorProfileReader::GetDisplayColorSpace(
    int64_t display_id) const {
  std::map<int64_t, gfx::ColorSpace>::const_iterator found;

  // The DXGI API takes precedence over the manually read ICC profiles.
  found = dxgi_display_id_to_color_space_map_.find(display_id);
  if (found != dxgi_display_id_to_color_space_map_.end())
    return found->second;

  found = display_id_to_color_space_map_.find(display_id);
  if (found != display_id_to_color_space_map_.end())
    return found->second;

  return default_color_space_;
}

void ColorProfileReader::UpdateDXGIDisplayIdToColorSpaceMap() {
  dxgi_display_id_to_color_space_map_.clear();
  bool hdr_monitor_found = false;
#if defined(ENABLE_HDR_DETECTION)
  HRESULT hr;
  base::win::ScopedComPtr<IDXGIFactory> dxgi_factory;
  CreateDXGIFactory(IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
  if (!dxgi_factory)
    return;

  for (UINT adapter_index = 0; true; ++adapter_index) {
    base::win::ScopedComPtr<IDXGIAdapter> dxgi_adapter;
    hr = dxgi_factory->EnumAdapters(adapter_index, dxgi_adapter.GetAddressOf());
    if (FAILED(hr))
      break;

    unsigned int i = 0;
    while (true) {
      base::win::ScopedComPtr<IDXGIOutput> output;
      if (FAILED(dxgi_adapter->EnumOutputs(i++, output.GetAddressOf())))
        break;
      base::win::ScopedComPtr<IDXGIOutput6> output6;
      if (FAILED(output.CopyTo(output6.GetAddressOf())))
        continue;

      DXGI_OUTPUT_DESC1 desc;
      if (FAILED(output6->GetDesc1(&desc)))
        continue;

      UMA_HISTOGRAM_SPARSE_SLOWLY("GPU.Output.ColorSpace", desc.ColorSpace);
      UMA_HISTOGRAM_SPARSE_SLOWLY("GPU.Output.MaxLuminance", desc.MaxLuminance);

      const base::string16& device_name = desc.DeviceName;
      int64_t display_id =
          DisplayInfo::DeviceIdFromDeviceName(device_name.c_str());
      if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
        dxgi_display_id_to_color_space_map_[display_id] =
            gfx::ColorSpace::CreateSCRGBLinear();
        hdr_monitor_found = true;
      }
    }
  }
  UMA_HISTOGRAM_BOOLEAN("GPU.Output.HDR", hdr_monitor_found);
#endif
}

}  // namespace win
}  // namespace display
