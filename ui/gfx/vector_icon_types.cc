// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/vector_icon_types.h"

namespace gfx {

bool VectorIcon::operator<(const VectorIcon& other) const {
  if (&other == this)
    return false;

  if (path_size != other.path_size)
    return path_size < other.path_size;
  if (path_1x_size != other.path_1x_size)
    return path_1x_size < other.path_1x_size;

  int diff = std::memcmp(path, other.path, path_size);
  if (diff)
    return diff < 0;

  return std::memcmp(path_1x, other.path_1x, path_1x_size) < 0;
}

}  // namespace gfx
