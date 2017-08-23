// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/types/display_snapshot.h"

#include <inttypes.h>

#include <algorithm>
#include <sstream>
#include <utility>

#include "base/strings/stringprintf.h"

namespace display {

namespace {

// The display serial number beginning byte position and its length in the
// EDID number as defined in the spec.
// https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
constexpr size_t kSerialNumberBeginingByte = 12U;
constexpr size_t kSerialNumberLengthInBytes = 4U;

std::string ModeListString(
    const std::vector<std::unique_ptr<const DisplayMode>>& modes) {
  std::stringstream stream;
  bool first = true;
  for (auto& mode : modes) {
    if (!first)
      stream << ", ";
    stream << mode->ToString();
    first = false;
  }
  return stream.str();
}

std::string DisplayConnectionTypeString(DisplayConnectionType type) {
  switch (type) {
    case DISPLAY_CONNECTION_TYPE_NONE:
      return "none";
    case DISPLAY_CONNECTION_TYPE_UNKNOWN:
      return "unknown";
    case DISPLAY_CONNECTION_TYPE_INTERNAL:
      return "internal";
    case DISPLAY_CONNECTION_TYPE_VGA:
      return "vga";
    case DISPLAY_CONNECTION_TYPE_HDMI:
      return "hdmi";
    case DISPLAY_CONNECTION_TYPE_DVI:
      return "dvi";
    case DISPLAY_CONNECTION_TYPE_DISPLAYPORT:
      return "dp";
    case DISPLAY_CONNECTION_TYPE_NETWORK:
      return "network";
  }
  NOTREACHED();
  return "";
}

}  // namespace

// static
std::unique_ptr<DisplaySnapshot> DisplaySnapshot::Clone(
    const DisplaySnapshot& other) {
  DisplayModeList clone_modes;
  const DisplayMode* current_mode = nullptr;
  const DisplayMode* native_mode = nullptr;

  // Clone the display modes and find equivalent pointers to the native and
  // current mode.
  for (auto& mode : other.modes()) {
    clone_modes.push_back(mode->Clone());
    if (mode.get() == other.current_mode())
      current_mode = mode.get();
    if (mode.get() == other.native_mode())
      native_mode = mode.get();
  }

  return std::make_unique<DisplaySnapshot>(
      other.display_id(), other.origin(), other.physical_size(), other.type(),
      other.is_aspect_preserving_scaling(), other.has_overscan(),
      other.has_color_correction_matrix(), other.display_name(),
      other.sys_path(), other.product_id(), std::move(clone_modes),
      other.edid(), current_mode, native_mode, other.maximum_cursor_size());
}

DisplaySnapshot::DisplaySnapshot(int64_t display_id,
                                 const gfx::Point& origin,
                                 const gfx::Size& physical_size,
                                 DisplayConnectionType type,
                                 bool is_aspect_preserving_scaling,
                                 bool has_overscan,
                                 bool has_color_correction_matrix,
                                 std::string display_name,
                                 const base::FilePath& sys_path,
                                 int64_t product_id,
                                 DisplayModeList modes,
                                 const std::vector<uint8_t>& edid,
                                 const DisplayMode* current_mode,
                                 const DisplayMode* native_mode,
                                 const gfx::Size& maximum_cursor_size)
    : display_id_(display_id),
      origin_(origin),
      physical_size_(physical_size),
      type_(type),
      is_aspect_preserving_scaling_(is_aspect_preserving_scaling),
      has_overscan_(has_overscan),
      has_color_correction_matrix_(has_color_correction_matrix),
      display_name_(display_name),
      sys_path_(sys_path),
      modes_(std::move(modes)),
      edid_(edid),
      current_mode_(current_mode),
      native_mode_(native_mode),
      product_id_(kInvalidProductID) {
  // We must explicitly clear out the bytes that represent the serial number.
  const size_t end =
      std::min(kSerialNumberBeginingByte + kSerialNumberLengthInBytes,
               edid_.size());
  for (size_t i = kSerialNumberBeginingByte; i < end; ++i)
    edid_[i] = 0;
}

DisplaySnapshot::~DisplaySnapshot() {}

std::string DisplaySnapshot::ToString() const {
  return base::StringPrintf(
      "id=%" PRId64
      " current_mode=%s native_mode=%s origin=%s"
      " physical_size=%s, type=%s name=\"%s\" modes=(%s)",
      display_id_,
      current_mode_ ? current_mode_->ToString().c_str() : "nullptr",
      native_mode_ ? native_mode_->ToString().c_str() : "nullptr",
      origin_.ToString().c_str(), physical_size_.ToString().c_str(),
      DisplayConnectionTypeString(type_).c_str(), display_name_.c_str(),
      ModeListString(modes_).c_str());
}

// static
gfx::BufferFormat DisplaySnapshot::PrimaryFormat() {
  return gfx::BufferFormat::BGRX_8888;
}

}  // namespace display
