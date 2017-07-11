// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SerializedColorParams_h
#define SerializedColorParams_h

#include "core/html/ImageData.h"
#include "platform/graphics/CanvasColorParams.h"

namespace blink {

// This enumeration specifies the extra tags used to specify the color settings
// of the serialized/deserialized ImageData and ImageBitmap objects.
enum class ImageSerializationTag {
  kEndTag = 0,
  kCanvasColorSpaceTag = 1,   // followed by a SerializedColorSpace enum.
  kCanvasPixelFormatTag = 2,  // followed by a SerializedPixelFormat enum.
  kImageDataStorageFormatTag =
      3,  // followed by an SerializedStorageFormat enum.
  kOriginClean =
      4,  // followed by 1 if the image is origin clean and zero otherwise.
  kIsPremultiplied =
      5,  // followed by 1 if the image is premultiplied and zero otherwise.
};

// This enumeration specifies the values used to serialize CanvasColorSpace.
enum class SerializedColorSpace {
  kLegacy = 0,
  kSRGB = 1,
  kRec2020 = 2,
  kP3 = 3,
  kLast = kP3,
};

// This enumeration specifies the values used to serialize CanvasPixelFormat.
enum class SerializedPixelFormat {
  kRGBA8 = 0,
  kRGB10A2 = 1,
  kRGBA12 = 2,
  kF16 = 3,
  kLast = kF16,
};

// This enumeration specifies the values used to serialize
// ImageDataStorageFormat.
enum class SerializedStorageFormat {
  kUint8Clamped = 0,
  kUint16 = 1,
  kFloat32 = 2,
  kLast = kFloat32,
};

class SerializedColorParams {
 public:
  SerializedColorParams();
  SerializedColorParams(
      CanvasColorParams,
      ImageDataStorageFormat = kUint8ClampedArrayStorageFormat);
  SerializedColorParams(uint32_t color_space,
                        uint32_t pixel_format,
                        uint32_t storage_format = 0);

  CanvasColorParams GetCanvasColorParams();
  CanvasColorSpace GetColorSpace();
  ImageDataStorageFormat GetStorageFormat();

  bool SetSerializedColorSpace(uint32_t color_space);
  bool SetSerializedPixelFormat(uint32_t pixel_format);
  bool SetSerializedStorageFormat(uint32_t storage_format);

  uint32_t GetSerializedColorSpace();
  uint32_t GetSerializedPixelFormat();
  uint32_t GetSerializedStorageFormat();

 private:
  SerializedColorSpace color_space_;
  SerializedPixelFormat pixel_format_;
  SerializedStorageFormat storage_format_;
};

}  // namespace blink

#endif
