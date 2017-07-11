// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/core/v8/serialization/SerializedColorParams.h"

namespace blink {

SerializedColorParams::SerializedColorParams()
    : color_space_(SerializedColorSpace::kLegacy),
      pixel_format_(SerializedPixelFormat::kRGBA8),
      storage_format_(SerializedStorageFormat::kUint8Clamped) {}

SerializedColorParams::SerializedColorParams(
    CanvasColorParams color_params,
    ImageDataStorageFormat storage_format) {
  switch (color_params.color_space()) {
    case kSRGBCanvasColorSpace:
      color_space_ = SerializedColorSpace::kSRGB;
      break;
    case kRec2020CanvasColorSpace:
      color_space_ = SerializedColorSpace::kRec2020;
      break;
    case kP3CanvasColorSpace:
      color_space_ = SerializedColorSpace::kP3;
      break;
    default:
      color_space_ = SerializedColorSpace::kLegacy;
  }
  pixel_format_ = SerializedPixelFormat::kRGBA8;
  if (color_params.pixel_format() == kF16CanvasPixelFormat)
    pixel_format_ = SerializedPixelFormat::kF16;
  switch (storage_format) {
    case kUint16ArrayStorageFormat:
      storage_format_ = SerializedStorageFormat::kUint16;
      break;
    case kFloat32ArrayStorageFormat:
      storage_format_ = SerializedStorageFormat::kFloat32;
      break;
    default:
      storage_format_ = SerializedStorageFormat::kUint8Clamped;
  }
}

SerializedColorParams::SerializedColorParams(uint32_t color_space,
                                             uint32_t pixel_format,
                                             uint32_t storage_format) {
  SetSerializedColorSpace(color_space);
  SetSerializedPixelFormat(pixel_format);
  SetSerializedStorageFormat(storage_format);
}

CanvasColorParams SerializedColorParams::GetCanvasColorParams() {
  CanvasColorSpace color_space;
  switch (color_space_) {
    case SerializedColorSpace::kLegacy:
      color_space = kLegacyCanvasColorSpace;
      break;
    case SerializedColorSpace::kSRGB:
      color_space = kSRGBCanvasColorSpace;
      break;
    case SerializedColorSpace::kRec2020:
      color_space = kRec2020CanvasColorSpace;
      break;
    case SerializedColorSpace::kP3:
      color_space = kP3CanvasColorSpace;
  }

  CanvasPixelFormat pixel_format = kRGBA8CanvasPixelFormat;
  if (pixel_format_ == SerializedPixelFormat::kF16)
    pixel_format = kF16CanvasPixelFormat;
  return CanvasColorParams(color_space, pixel_format);
}

CanvasColorSpace SerializedColorParams::GetColorSpace() {
  return GetCanvasColorParams().color_space();
}

ImageDataStorageFormat SerializedColorParams::GetStorageFormat() {
  switch (storage_format_) {
    case SerializedStorageFormat::kUint16:
      return kUint16ArrayStorageFormat;
    case SerializedStorageFormat::kFloat32:
      return kFloat32ArrayStorageFormat;
    default:
      return kUint8ClampedArrayStorageFormat;
  }
}

bool SerializedColorParams::SetSerializedColorSpace(uint32_t color_space) {
  if (color_space > static_cast<uint32_t>(SerializedColorSpace::kLast))
    return false;
  color_space_ = static_cast<SerializedColorSpace>(color_space);
  return true;
}

uint32_t SerializedColorParams::GetSerializedColorSpace() {
  return static_cast<uint32_t>(color_space_);
}

bool SerializedColorParams::SetSerializedPixelFormat(uint32_t pixel_format) {
  if (pixel_format > static_cast<uint32_t>(SerializedPixelFormat::kLast))
    return false;
  pixel_format_ = static_cast<SerializedPixelFormat>(pixel_format);
  return true;
}

uint32_t SerializedColorParams::GetSerializedPixelFormat() {
  return static_cast<uint32_t>(pixel_format_);
}

bool SerializedColorParams::SetSerializedStorageFormat(
    uint32_t storage_format) {
  if (storage_format > static_cast<uint32_t>(SerializedStorageFormat::kLast))
    return false;
  storage_format_ = static_cast<SerializedStorageFormat>(storage_format);
  return true;
}

uint32_t SerializedColorParams::GetSerializedStorageFormat() {
  return static_cast<uint32_t>(storage_format_);
}

}  // namespace blink
