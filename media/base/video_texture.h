// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_VIDEO_TEXTURE_H_
#define MEDIA_BASE_VIDEO_TEXTURE_H_

namespace media {

struct TexParams {
  unsigned target = 0;
  unsigned texture = 0;
  int level = 0;
  bool premultiply_alpha = false;
  bool flip_y = false;
};

struct TexFormat {
  unsigned internal = 0;
  unsigned value = 0;
  unsigned type = 0;
};

struct TexOffset {
  int x = 0;
  int y = 0;
  int z = 0;
};

}  // namespace media

#endif  // MEDIA_BASE_VIDEO_TEXTURE_H_
