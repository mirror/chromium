// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ImageSerializationTag_h
#define ImageSerializationTag_h

namespace blink {

// This enumeration specifies the extra tags used to specify the color settings
// of the serialized/deserialized ImageData and ImageBitmap objects.
enum ImageSerializationTag {
  kCanvasColorSpaceTag = 'c',
  kCanvasPixelForamtTag = 'p',
  kImageDataStorageFormatTag = 's',
  kEndTag = 0xFF
};

}  // namespace blink

#endif
