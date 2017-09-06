// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_COPY_OUTPUT_RESULT_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_COPY_OUTPUT_RESULT_STRUCT_TRAITS_H_

#include "components/viz/common/quads/copy_output_result.h"
#include "services/viz/public/cpp/compositing/texture_mailbox_struct_traits.h"
#include "services/viz/public/interfaces/compositing/copy_output_result.mojom-shared.h"
#include "services/viz/public/interfaces/compositing/texture_mailbox_releaser.mojom.h"
#include "skia/public/interfaces/bitmap_skbitmap_struct_traits.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"

namespace mojo {

template <>
struct StructTraits<viz::mojom::CopyOutputResultDataView,
                    std::unique_ptr<viz::CopyOutputResult>> {
  static const gfx::Size& size(
      const std::unique_ptr<viz::CopyOutputResult>& result) {
    return result->size_;
  }

  static const SkBitmap& bitmap(
      const std::unique_ptr<viz::CopyOutputResult>& result);

  static const viz::TextureMailbox& texture_mailbox(
      const std::unique_ptr<viz::CopyOutputResult>& result) {
    return result->texture_mailbox_;
  }

  static viz::mojom::TextureMailboxReleaserPtr releaser(
      const std::unique_ptr<viz::CopyOutputResult>& result);

  static bool Read(viz::mojom::CopyOutputResultDataView data,
                   std::unique_ptr<viz::CopyOutputResult>* out_p);
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_CPP_COMPOSITING_COPY_OUTPUT_RESULT_STRUCT_TRAITS_H_
