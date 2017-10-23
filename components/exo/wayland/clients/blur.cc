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

double AdjustSigma(double sigma,
                   double max_non_scale_blur_sigma,
                   int* scale_factor) {
  *scale_factor = 1;
  if (sigma > max_non_scale_blur_sigma) {
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
               double max_non_scale_blur_sigma,
               int frames,
               int max_frames_per_commit) {
  Buffer* buffer = buffers_.front().get();
  SkScalar grid_width = size_.width() / kGridSize;
  SkScalar grid_height = size_.height() / kGridSize;

  // Create background grid image. Simulates a wallpaper.
  if (!grid_image) {
    SkImageInfo info = SkImageInfo::MakeN32(size_.width(), size_.height(),
                                            kOpaque_SkAlphaType);
    auto surface(SkSurface::MakeRaster(info));
    auto* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);
    for (size_t y = 0; y < kGridSize; ++y) {
      for (size_t x = 0; x < kGridSize; ++x) {
        if ((y + x) % 2)
          continue;
        SkPaint paint;
        paint.setColor(SK_ColorLTGRAY);
        canvas->save();
        canvas->translate(x * grid_width, y * grid_height);
        canvas->drawIRect(SkIRect::MakeWH(grid_width, grid_height), paint);
        canvas->restore();
      }
    }
    grid_image = surface->makeImageSnapshot();
  }

  // Create blur image if needed.
  sk_sp<SkImageFilter> blur_filter;
  sk_sp<SkSurface> blur_surface;
  int blur_scale_x = 1;
  int blur_scale_y = 1;
  if (sigma_x || sigma_y) {
    sigma_x = AdjustSigma(sigma_x, max_non_scale_blur_sigma, &blur_scale_x);
    sigma_y = AdjustSigma(sigma_y, max_non_scale_blur_sigma, &blur_scale_y);
    blur_filter = SkBlurImageFilter::Make(sigma_x, sigma_y, nullptr, nullptr,
                                          SkBlurImageFilter::kClamp_TileMode);
    blur_surface = buffer->sk_surface->makeSurface(SkImageInfo::MakeN32(
        size_.width() / blur_scale_x, size_.height() / blur_scale_y,
        kOpaque_SkAlphaType));
  }

  bool callback_pending = false;
  std::unique_ptr<wl_callback> frame_callback;
  wl_callback_listener frame_listener = {FrameCallback};
  int frame_count = 0;
  base::TimeTicks initial_time = base::TimeTicks::Now();
  while (frame_count < frames) {
    // Draw to temporary blur surface when needed.
    SkCanvas* canvas;
    if (blur_filter)
      canvas = blur_surface->getCanvas();
    else
      canvas = buffer->sk_surface->getCanvas();

    canvas->save();
    canvas->scale(1.0f / blur_scale_x, 1.0f / blur_scale_y);

    // Draw background grid.
    canvas->clear(SK_ColorWHITE);
    canvas->drawImage(grid_image, 0, 0);

    // Draw rectangles. Simulates background contents.
    SkScalar rect_size = SkScalarHalf(std::min(grid_width, grid_height));
    SkIRect rect =
        SkIRect::MakeXYWH(-SkScalarHalf(rect_size), -SkScalarHalf(rect_size),
                          rect_size, rect_size);
    base::TimeDelta elapsed_time = base::TimeTicks::Now() - initial_time;
    SkScalar rotation = elapsed_time.InMilliseconds() * kRotationSpeed / 1000;
    for (size_t y = 0; y < kGridSize; ++y) {
      for (size_t x = 0; x < kGridSize; ++x) {
        const SkColor kColors[] = {SK_ColorBLUE, SK_ColorGREEN,
                                   SK_ColorRED,  SK_ColorYELLOW,
                                   SK_ColorCYAN, SK_ColorMAGENTA};
        SkPaint paint;
        paint.setColor(kColors[(y * kGridSize + x) % arraysize(kColors)]);
        canvas->save();
        canvas->translate(x * grid_width + SkScalarHalf(grid_width),
                          y * grid_height + SkScalarHalf(grid_height));
        canvas->rotate(rotation / (y * kGridSize + x + 1));
        canvas->drawIRect(rect, paint);
        canvas->restore();
      }
    }
    canvas->restore();

    if (blur_filter) {
      SkIRect subset;
      SkIPoint offset;
      sk_sp<SkImage> blur_image = blur_surface->makeImageSnapshot();
      sk_sp<SkImage> blurred_image =
          blur_image->makeWithFilter(blur_filter.get(), blur_image->bounds(),
                                     blur_image->bounds(), &subset, &offset);
      canvas = buffer->sk_surface->getCanvas();
      canvas->save();
      canvas->scale(blur_scale_x, blur_scale_y);
      SkPaint paint;
      paint.setBlendMode(SkBlendMode::kSrc);
      // Simulate multi-texturing by adding foreground opacity.
      int alpha = (1.0 - kForegroundOpacity) * 255.0 + 0.5;
      paint.setColor(SkColorSetA(SK_ColorBLACK, alpha));
      canvas->drawImage(blurred_image, -subset.x(), -subset.y(), &paint);
      canvas->restore();
    } else {
      SkPaint paint;
      int alpha = kForegroundOpacity * 255.0 + 0.5;
      paint.setColor(SkColorSetA(SK_ColorBLACK, alpha));
      canvas->drawIRect(SkIRect::MakeWH(size_.width(), size_.height()), paint);
    }

    if (gr_context_) {
      gr_context_->flush();
      glFinish();
    }

    frame_count++;
    if (callback_pending && (frame_count % max_frames_per_commit))
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
