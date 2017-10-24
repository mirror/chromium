// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/wayland/clients/blur.h"

#include "base/command_line.h"
#include "components/exo/wayland/clients/client_helper.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/effects/SkBlurImageFilter.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gl/gl_bindings.h"

namespace exo {
namespace wayland {
namespace clients {
namespace {

// Rotation speed (degrees/second).
const double kRotationSpeed = 360.0;

// The opacity of the foreground.
const double kForegroundOpacity = 0.7;

// Rects grid size.
const int kGridSize = 4;

// Adjust sigma by increasing the scale factor until less than |max_sigma|.
// Returns the adjusted sigma value.
double AdjustSigma(double sigma, double max_sigma, int* scale_factor) {
  *scale_factor = 1;
  while (sigma > max_sigma) {
    *scale_factor *= 2;
    sigma /= 2.0;
  }
  return sigma;
}

void FrameCallback(void* data, wl_callback* callback, uint32_t time) {
  bool* callback_pending = static_cast<bool*>(data);
  *callback_pending = false;
}

}  // namespace

Blur::Blur() = default;

Blur::~Blur() = default;

void Blur::Run(double sigma_x,
               double sigma_y,
               double max_sigma,
               bool offscreen,
               int frames) {
  Buffer* buffer = buffers_.front().get();
  SkScalar cell_width = size_.width() / kGridSize;
  SkScalar cell_height = size_.height() / kGridSize;

  // Create background grid image. Simulates a wallpaper.
  if (!grid_image) {
    SkImageInfo info = SkImageInfo::MakeN32(size_.width(), size_.height(),
                                            kOpaque_SkAlphaType);
    sk_sp<SkSurface> surface(SkSurface::MakeRaster(info));
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);
    for (size_t y = 0u; y < kGridSize; ++y) {
      for (size_t x = 0u; x < kGridSize; ++x) {
        if ((y + x) % 2)
          continue;
        SkPaint paint;
        paint.setColor(SK_ColorLTGRAY);
        canvas->save();
        canvas->translate(x * cell_width, y * cell_height);
        canvas->drawIRect(SkIRect::MakeWH(cell_width, cell_height), paint);
        canvas->restore();
      }
    }
    grid_image = surface->makeImageSnapshot();
  }

  // Create blur images if needed.
  sk_sp<SkImageFilter> blur_filter;
  std::vector<sk_sp<SkSurface>> blur_surfaces;
  int blur_scale_factor_x = 1;
  int blur_scale_factor_y = 1;
  if (sigma_x > 0.0 || sigma_y > 0.0) {
    sigma_x = AdjustSigma(sigma_x, max_sigma, &blur_scale_factor_x);
    sigma_y = AdjustSigma(sigma_y, max_sigma, &blur_scale_factor_y);
    blur_filter = SkBlurImageFilter::Make(sigma_x, sigma_y, nullptr, nullptr,
                                          SkBlurImageFilter::kClamp_TileMode);
    SkISize blur_size = SkISize::Make(size_.width() / blur_scale_factor_x,
                                      size_.height() / blur_scale_factor_y);
    do {
      blur_surfaces.push_back(
          buffer->sk_surface->makeSurface(SkImageInfo::MakeN32(
              blur_size.width(), blur_size.height(), kOpaque_SkAlphaType)));
      blur_size =
          SkISize::Make(std::min(size_.width(), blur_size.width() * 2),
                        std::min(size_.height(), blur_size.height() * 2));
    } while (blur_size.width() < size_.width() ||
             blur_size.height() < size_.height());
  }

  bool callback_pending = false;
  std::unique_ptr<wl_callback> frame_callback;
  wl_callback_listener frame_listener = {FrameCallback};
  int frame_count = 0;
  base::TimeTicks initial_time = base::TimeTicks::Now();
  while (frame_count < frames) {
    size_t larger_surface_index = blur_surfaces.size();
    do {
      SkCanvas* canvas = buffer->sk_surface->getCanvas();
      double scale_x = 1.0;
      double scale_y = 1.0;

      if (!blur_surfaces.empty()) {
        SkSurface* surface = blur_surfaces[larger_surface_index - 1].get();
        if (larger_surface_index < blur_surfaces.size()) {
          SkSurface* larger_surface = blur_surfaces[larger_surface_index].get();
          scale_x =
              static_cast<double>(surface->width()) / larger_surface->width();
          scale_y =
              static_cast<double>(surface->height()) / larger_surface->height();
        } else {
          scale_x = static_cast<double>(surface->width()) / size_.width();
          scale_y = static_cast<double>(surface->height()) / size_.height();
        }
        canvas = surface->getCanvas();
      }

      canvas->save();
      canvas->scale(scale_x, scale_y);

      if (larger_surface_index < blur_surfaces.size()) {
        // Scale larger surface.
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setFilterQuality(SkFilterQuality::kLow_SkFilterQuality);
        blur_surfaces[larger_surface_index]->draw(canvas, 0, 0, &paint);
      } else {
        // Draw background grid.
        canvas->clear(SK_ColorWHITE);
        canvas->drawImage(grid_image, 0, 0);

        // Draw rectangles.
        SkScalar rect_size = SkScalarHalf(std::min(cell_width, cell_height));
        SkIRect rect =
            SkIRect::MakeXYWH(-SkScalarHalf(rect_size),
                              -SkScalarHalf(rect_size), rect_size, rect_size);
        base::TimeDelta elapsed_time = base::TimeTicks::Now() - initial_time;
        SkScalar rotation =
            elapsed_time.InMilliseconds() * kRotationSpeed / 1000;
        for (size_t y = 0u; y < kGridSize; ++y) {
          for (size_t x = 0u; x < kGridSize; ++x) {
            const SkColor kColors[] = {SK_ColorBLUE, SK_ColorGREEN,
                                       SK_ColorRED,  SK_ColorYELLOW,
                                       SK_ColorCYAN, SK_ColorMAGENTA};
            SkPaint paint;
            paint.setColor(kColors[(y * kGridSize + x) % arraysize(kColors)]);
            canvas->save();
            canvas->translate(x * cell_width + SkScalarHalf(cell_width),
                              y * cell_height + SkScalarHalf(cell_height));
            canvas->rotate(rotation / (y * kGridSize + x + 1));
            canvas->drawIRect(rect, paint);
            canvas->restore();
          }
        }
      }
      canvas->restore();
    } while (larger_surface_index-- > 1);

    if (blur_filter) {
      SkIRect subset;
      SkIPoint offset;
      sk_sp<SkImage> blur_image = blur_surfaces.front()->makeImageSnapshot();
      sk_sp<SkImage> blurred_image =
          blur_image->makeWithFilter(blur_filter.get(), blur_image->bounds(),
                                     blur_image->bounds(), &subset, &offset);
      SkCanvas* canvas = buffer->sk_surface->getCanvas();
      canvas->save();
      canvas->scale(blur_scale_factor_x, blur_scale_factor_y);
      SkPaint paint;
      paint.setBlendMode(SkBlendMode::kSrc);
      paint.setFilterQuality(SkFilterQuality::kLow_SkFilterQuality);
      // Simulate multi-texturing by adding foreground opacity.
      int alpha = (1.0 - kForegroundOpacity) * 255.0 + 0.5;
      paint.setColor(SkColorSetA(SK_ColorBLACK, alpha));
      canvas->drawImage(blurred_image, -subset.x(), -subset.y(), &paint);
      canvas->restore();
    } else {
      SkPaint paint;
      int alpha = kForegroundOpacity * 255.0 + 0.5;
      paint.setColor(SkColorSetA(SK_ColorBLACK, alpha));
      SkCanvas* canvas = buffer->sk_surface->getCanvas();
      canvas->drawIRect(SkIRect::MakeWH(size_.width(), size_.height()), paint);
    }

    if (gr_context_) {
      gr_context_->flush();
      glFinish();
    }

    // Submit 1 of 50 frames for onscreen display when in offscreen mode.
    if ((frame_count++ % 50) && offscreen)
      continue;

    while (callback_pending)
      wl_display_dispatch(display_.get());

    callback_pending = true;

    wl_surface_set_buffer_scale(surface_.get(), scale_);
    wl_surface_set_buffer_transform(surface_.get(), transform_);
    wl_surface_damage(surface_.get(), 0, 0, surface_size_.width(),
                      surface_size_.height());
    wl_surface_attach(surface_.get(), buffer->buffer.get(), 0, 0);

    frame_callback.reset(wl_surface_frame(surface_.get()));
    wl_callback_add_listener(frame_callback.get(), &frame_listener,
                             &callback_pending);
    wl_surface_commit(surface_.get());
    wl_display_flush(display_.get());
  }
}

}  // namespace clients
}  // namespace wayland
}  // namespace exo
