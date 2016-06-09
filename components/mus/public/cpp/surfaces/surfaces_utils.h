// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MUS_PUBLIC_CPP_SURFACES_SURFACES_UTILS_H_
#define COMPONENTS_MUS_PUBLIC_CPP_SURFACES_SURFACES_UTILS_H_

#include "cc/ipc/quads.mojom.h"
#include "components/mus/public/cpp/surfaces/mojo_surfaces_export.h"

namespace cc {
class SharedQuadState;
}

namespace gfx {
class Rect;
class Size;
}

namespace mojo {

MOJO_SURFACES_EXPORT cc::SharedQuadState CreateDefaultSQS(
    const gfx::Size& size);

// Constructs a pass with the given id, output_rect and damage_rect set to rect,
// transform_to_root_target set to identity and has_transparent_background set
// to false.
MOJO_SURFACES_EXPORT cc::mojom::RenderPassPtr CreateDefaultPass(
    int id,
    const gfx::Rect& rect);

}  // namespace mojo

#endif  // COMPONENTS_MUS_PUBLIC_CPP_SURFACES_SURFACES_UTILS_H_
