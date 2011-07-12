// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_IMAGE_IMAGE_UTIL_H_
#define UI_GFX_IMAGE_IMAGE_UTIL_H_
#pragma once

#include <vector>

#include "base/basictypes.h"

namespace gfx {
class Image;
}

namespace gfx {

// Creates an image from the given PNG-encoded input.  The caller owns the
// returned Image.  If there was an error creating the image, returns NULL.
Image* ImageFromPNGEncodedData(const unsigned char* input, size_t input_size);

// Fills the |dst| vector with PNG-encoded bytes based on the given Image.
// Returns true if the Image was encoded successfully.
bool PNGEncodedDataFromImage(const Image& image,
                             std::vector<unsigned char>* dst);

}

#endif  // UI_GFX_IMAGE_IMAGE_UTIL_H_
