// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_FILTER_OPERATION_STRUCT_TRAITS_H_
#define CC_IPC_FILTER_OPERATION_STRUCT_TRAITS_H_

#include "base/containers/span.h"
#include "cc/ipc/filter_operation.mojom-shared.h"
#include "skia/public/interfaces/blur_image_filter_tile_mode_struct_traits.h"
#include "skia/public/interfaces/image_filter_struct_traits.h"
#include "ui/gfx/filter_operation.h"

namespace mojo {

namespace {
cc::mojom::FilterType CCFilterTypeToMojo(
    const gfx::FilterOperation::FilterType& type) {
  switch (type) {
    case gfx::FilterOperation::GRAYSCALE:
      return cc::mojom::FilterType::GRAYSCALE;
    case gfx::FilterOperation::SEPIA:
      return cc::mojom::FilterType::SEPIA;
    case gfx::FilterOperation::SATURATE:
      return cc::mojom::FilterType::SATURATE;
    case gfx::FilterOperation::HUE_ROTATE:
      return cc::mojom::FilterType::HUE_ROTATE;
    case gfx::FilterOperation::INVERT:
      return cc::mojom::FilterType::INVERT;
    case gfx::FilterOperation::BRIGHTNESS:
      return cc::mojom::FilterType::BRIGHTNESS;
    case gfx::FilterOperation::CONTRAST:
      return cc::mojom::FilterType::CONTRAST;
    case gfx::FilterOperation::OPACITY:
      return cc::mojom::FilterType::OPACITY;
    case gfx::FilterOperation::BLUR:
      return cc::mojom::FilterType::BLUR;
    case gfx::FilterOperation::DROP_SHADOW:
      return cc::mojom::FilterType::DROP_SHADOW;
    case gfx::FilterOperation::COLOR_MATRIX:
      return cc::mojom::FilterType::COLOR_MATRIX;
    case gfx::FilterOperation::ZOOM:
      return cc::mojom::FilterType::ZOOM;
    case gfx::FilterOperation::REFERENCE:
      return cc::mojom::FilterType::REFERENCE;
    case gfx::FilterOperation::SATURATING_BRIGHTNESS:
      return cc::mojom::FilterType::SATURATING_BRIGHTNESS;
    case gfx::FilterOperation::ALPHA_THRESHOLD:
      return cc::mojom::FilterType::ALPHA_THRESHOLD;
  }
  NOTREACHED();
  return cc::mojom::FilterType::FILTER_TYPE_LAST;
}

gfx::FilterOperation::FilterType MojoFilterTypeToCC(
    const cc::mojom::FilterType& type) {
  switch (type) {
    case cc::mojom::FilterType::GRAYSCALE:
      return gfx::FilterOperation::GRAYSCALE;
    case cc::mojom::FilterType::SEPIA:
      return gfx::FilterOperation::SEPIA;
    case cc::mojom::FilterType::SATURATE:
      return gfx::FilterOperation::SATURATE;
    case cc::mojom::FilterType::HUE_ROTATE:
      return gfx::FilterOperation::HUE_ROTATE;
    case cc::mojom::FilterType::INVERT:
      return gfx::FilterOperation::INVERT;
    case cc::mojom::FilterType::BRIGHTNESS:
      return gfx::FilterOperation::BRIGHTNESS;
    case cc::mojom::FilterType::CONTRAST:
      return gfx::FilterOperation::CONTRAST;
    case cc::mojom::FilterType::OPACITY:
      return gfx::FilterOperation::OPACITY;
    case cc::mojom::FilterType::BLUR:
      return gfx::FilterOperation::BLUR;
    case cc::mojom::FilterType::DROP_SHADOW:
      return gfx::FilterOperation::DROP_SHADOW;
    case cc::mojom::FilterType::COLOR_MATRIX:
      return gfx::FilterOperation::COLOR_MATRIX;
    case cc::mojom::FilterType::ZOOM:
      return gfx::FilterOperation::ZOOM;
    case cc::mojom::FilterType::REFERENCE:
      return gfx::FilterOperation::REFERENCE;
    case cc::mojom::FilterType::SATURATING_BRIGHTNESS:
      return gfx::FilterOperation::SATURATING_BRIGHTNESS;
    case cc::mojom::FilterType::ALPHA_THRESHOLD:
      return gfx::FilterOperation::ALPHA_THRESHOLD;
  }
  NOTREACHED();
  return gfx::FilterOperation::FILTER_TYPE_LAST;
}

}  // namespace

template <>
struct StructTraits<cc::mojom::FilterOperationDataView, gfx::FilterOperation> {
  static cc::mojom::FilterType type(const gfx::FilterOperation& op) {
    return CCFilterTypeToMojo(op.type());
  }

  static float amount(const gfx::FilterOperation& operation) {
    if (operation.type() == gfx::FilterOperation::COLOR_MATRIX ||
        operation.type() == gfx::FilterOperation::REFERENCE) {
      return 0.f;
    }
    return operation.amount();
  }

  static float outer_threshold(const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::ALPHA_THRESHOLD)
      return 0.f;
    return operation.outer_threshold();
  }

  static gfx::Point drop_shadow_offset(const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::DROP_SHADOW)
      return gfx::Point();
    return operation.drop_shadow_offset();
  }

  static uint32_t drop_shadow_color(const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::DROP_SHADOW)
      return 0;
    return operation.drop_shadow_color();
  }

  static sk_sp<SkImageFilter> image_filter(
      const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::REFERENCE)
      return sk_sp<SkImageFilter>();
    return operation.image_filter();
  }

  static base::span<const float> matrix(const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::COLOR_MATRIX)
      return base::span<const float>();
    return operation.matrix();
  }

  static base::span<const gfx::Rect> shape(
      const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::ALPHA_THRESHOLD)
      return base::span<gfx::Rect>();
    return operation.shape();
  }

  static int32_t zoom_inset(const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::ZOOM)
      return 0;
    return operation.zoom_inset();
  }

  static skia::mojom::BlurTileMode blur_tile_mode(
      const gfx::FilterOperation& operation) {
    if (operation.type() != gfx::FilterOperation::BLUR)
      return skia::mojom::BlurTileMode::CLAMP_TO_BLACK;
    return EnumTraits<skia::mojom::BlurTileMode, SkBlurImageFilter::TileMode>::
        ToMojom(operation.blur_tile_mode());
  }

  static bool Read(cc::mojom::FilterOperationDataView data,
                   gfx::FilterOperation* out) {
    out->set_type(MojoFilterTypeToCC(data.type()));
    switch (out->type()) {
      case gfx::FilterOperation::GRAYSCALE:
      case gfx::FilterOperation::SEPIA:
      case gfx::FilterOperation::SATURATE:
      case gfx::FilterOperation::HUE_ROTATE:
      case gfx::FilterOperation::INVERT:
      case gfx::FilterOperation::BRIGHTNESS:
      case gfx::FilterOperation::SATURATING_BRIGHTNESS:
      case gfx::FilterOperation::CONTRAST:
      case gfx::FilterOperation::OPACITY:
        out->set_amount(data.amount());
        return true;
      case gfx::FilterOperation::BLUR:
        out->set_amount(data.amount());
        SkBlurImageFilter::TileMode tile_mode;
        if (!data.ReadBlurTileMode(&tile_mode))
          return false;
        out->set_blur_tile_mode(tile_mode);
        return true;
      case gfx::FilterOperation::DROP_SHADOW: {
        out->set_amount(data.amount());
        gfx::Point offset;
        if (!data.ReadDropShadowOffset(&offset))
          return false;
        out->set_drop_shadow_offset(offset);
        out->set_drop_shadow_color(data.drop_shadow_color());
        return true;
      }
      case gfx::FilterOperation::COLOR_MATRIX: {
        // TODO(fsamuel): It would be nice to modify gfx::FilterOperation to
        // avoid this extra copy.
        gfx::FilterOperation::Matrix matrix_buffer = {};
        base::span<float> matrix(matrix_buffer);
        if (!data.ReadMatrix(&matrix))
          return false;
        out->set_matrix(matrix_buffer);
        return true;
      }
      case gfx::FilterOperation::ZOOM: {
        if (data.amount() < 0.f || data.zoom_inset() < 0)
          return false;
        out->set_amount(data.amount());
        out->set_zoom_inset(data.zoom_inset());
        return true;
      }
      case gfx::FilterOperation::REFERENCE: {
        sk_sp<SkImageFilter> filter;
        if (!data.ReadImageFilter(&filter))
          return false;
        out->set_image_filter(filter);
        return true;
      }
      case gfx::FilterOperation::ALPHA_THRESHOLD:
        out->set_amount(data.amount());
        out->set_outer_threshold(data.outer_threshold());
        gfx::FilterOperation::ShapeRects shape;
        if (!data.ReadShape(&shape))
          return false;
        out->set_shape(shape);
        return true;
    }
    return false;
  }
};

}  // namespace mojo

#endif  // CC_IPC_FILTER_OPERATION_STRUCT_TRAITS_H_
