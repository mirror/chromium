// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/mojo/image_capture_types.h"

namespace media {

namespace mojo {

mojom::PhotoStatePtr CreateEmptyPhotoState() {
  mojom::PhotoStatePtr photo_capabilities = mojom::PhotoState::New();
  photo_capabilities->height = mojom::Range::New();
  photo_capabilities->width = mojom::Range::New();
  photo_capabilities->exposure_compensation = mojom::Range::New();
  photo_capabilities->color_temperature = mojom::Range::New();
  photo_capabilities->iso = mojom::Range::New();
  photo_capabilities->brightness = mojom::Range::New();
  photo_capabilities->contrast = mojom::Range::New();
  photo_capabilities->saturation = mojom::Range::New();
  photo_capabilities->sharpness = mojom::Range::New();
  photo_capabilities->zoom = mojom::Range::New();
  photo_capabilities->torch = false;
  photo_capabilities->red_eye_reduction = mojom::RedEyeReduction::NEVER;
  return photo_capabilities;
}

}  // namespace mojo
}  // namespace media