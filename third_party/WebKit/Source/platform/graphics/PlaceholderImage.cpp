// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/PlaceholderImage.h"

#include <utility>

#include "platform/fonts/FontDescription.h"
#include "platform/fonts/FontFamily.h"
#include "platform/fonts/FontSelectionTypes.h"
#include "platform/geometry/FloatPoint.h"
#include "platform/geometry/FloatRect.h"
#include "platform/geometry/IntPoint.h"
#include "platform/geometry/IntRect.h"
#include "platform/graphics/BitmapImage.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/ImageObserver.h"
#include "platform/graphics/paint/PaintCanvas.h"
#include "platform/graphics/paint/PaintFlags.h"
#include "platform/graphics/paint/PaintRecord.h"
#include "platform/graphics/paint/PaintRecorder.h"
#include "platform/text/TextRun.h"
#include "platform/wtf/StdLibExtras.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSize.h"

namespace blink {

namespace {

// Placeholder image visual specifications:
// https://docs.google.com/document/d/1BHeA1azbgCdZgCnr16VN2g7A9MHPQ_dwKn5szh8evMQ/edit

constexpr int kIconWidth = 24;
constexpr int kIconHeight = 24;
constexpr int kFeaturePaddingX = 8;
constexpr int kIconPaddingY = 5;
constexpr int kPaddingBetweenIconAndText = 2;
constexpr int kTextPaddingY = 9;

constexpr int kFontSize = 14;

void DrawIcon(PaintCanvas* canvas, const PaintFlags& flags, float x, float y) {
  DEFINE_STATIC_REF(Image, icon_image,
                    (Image::LoadPlatformResource("placeholderIcon")));
  DCHECK(!icon_image->IsNull());

  // Note that the |icon_image| is not scaled according to dest_rect / src_rect,
  // and is always drawn at the same size. This is so that placeholder icons are
  // visible (e.g. when replacing a large image that's scaled down to a small
  // area) and so that all placeholder images on the same page look consistent.
  canvas->drawImageRect(icon_image->PaintImageForCurrentFrame(),
                        IntRect(IntPoint::Zero(), icon_image->Size()),
                        FloatRect(x, y, kIconWidth, kIconHeight), &flags,
                        PaintCanvas::kFast_SrcRectConstraint);
}

RefPtr<SharedFontFamily> CreateSharedFontFamily(const AtomicString& name,
                                                RefPtr<SharedFontFamily> next) {
  RefPtr<SharedFontFamily> family = SharedFontFamily::Create();
  family->SetFamily(name);
  if (next)
    family->AppendFamily(std::move(next));
  return family;
}

FontDescription CreatePlaceholderFontDescription() {
  FontDescription description;
  description.FirstFamily().SetFamily("Roboto");
  description.FirstFamily().AppendFamily(CreateSharedFontFamily(
      "Helvetica Neue",
      CreateSharedFontFamily("Helvetica",
                             CreateSharedFontFamily("Arial", nullptr))));

  description.SetGenericFamily(FontDescription::kSansSerifFamily);
  description.SetComputedSize(kFontSize);
  description.SetWeight(FontSelectionValue(500));

  return description;
}

}  // namespace

PlaceholderImage::PlaceholderImage(ImageObserver* observer,
                                   const IntSize& size,
                                   const String& text)
    : Image(observer),
      size_(size),
      text_(text),
      cached_text_width_(-1.0f),
      paint_record_content_id_(-1) {}

PlaceholderImage::~PlaceholderImage() {}

PaintImage PlaceholderImage::PaintImageForCurrentFrame() {
  PaintImageBuilder builder;
  InitPaintImageBuilder(builder);

  const IntRect dest_rect(0, 0, size_.Width(), size_.Height());
  if (paint_record_for_current_frame_) {
    builder.set_paint_record(paint_record_for_current_frame_, dest_rect,
                             paint_record_content_id_);
    return builder.TakePaintImage();
  }

  PaintRecorder paint_recorder;
  Draw(paint_recorder.beginRecording(FloatRect(dest_rect)), PaintFlags(),
       FloatRect(dest_rect), FloatRect(dest_rect),
       kDoNotRespectImageOrientation, kClampImageToSourceRect);

  paint_record_for_current_frame_ = paint_recorder.finishRecordingAsPicture();
  paint_record_content_id_ = PaintImage::GetNextContentId();
  builder.set_paint_record(paint_record_for_current_frame_, dest_rect,
                           paint_record_content_id_);
  return builder.TakePaintImage();
}

void PlaceholderImage::Draw(PaintCanvas* canvas,
                            const PaintFlags& base_flags,
                            const FloatRect& dest_rect,
                            const FloatRect& src_rect,
                            RespectImageOrientationEnum respect_orientation,
                            ImageClampingMode image_clamping_mode) {
  if (!src_rect.Intersects(FloatRect(0.0f, 0.0f,
                                     static_cast<float>(size_.Width()),
                                     static_cast<float>(size_.Height())))) {
    return;
  }

  PaintFlags flags(base_flags);
  flags.setStyle(PaintFlags::kFill_Style);
  flags.setColor(SkColorSetARGB(0x80, 0xD9, 0xD9, 0xD9));
  canvas->drawRect(dest_rect, flags);

  if (dest_rect.Width() < kIconWidth + 2 * kFeaturePaddingX ||
      dest_rect.Height() < kIconHeight + 2 * kIconPaddingY) {
    return;
  }

  if (!text_.IsEmpty() && cached_text_width_ < 0.0f) {
    font_ = Font(CreatePlaceholderFontDescription());
    font_.Update(nullptr);
    cached_text_width_ = font_.Width(TextRun(text_));
  }

  const float icon_and_text_width =
      cached_text_width_ +
      (kIconWidth + 2 * kFeaturePaddingX + kPaddingBetweenIconAndText);

  if (text_.IsEmpty() || dest_rect.Width() < icon_and_text_width) {
    DrawIcon(canvas, flags,
             dest_rect.X() + (dest_rect.Width() - kIconWidth) / 2.0f,
             dest_rect.Y() + (dest_rect.Height() - kIconHeight) / 2.0f);
    return;
  }

  const float feature_x =
      dest_rect.X() + (dest_rect.Width() - icon_and_text_width) / 2.0f;
  const float feature_y =
      dest_rect.Y() +
      (dest_rect.Height() - (kIconHeight + 2 * kIconPaddingY)) / 2.0f;

  DrawIcon(canvas, base_flags, feature_x + kFeaturePaddingX,
           feature_y + kIconPaddingY);

  flags.setColor(SkColorSetARGB(0xAB, 0, 0, 0));
  font_.DrawBidiText(canvas, TextRunPaintInfo(TextRun(text_)),
                     FloatPoint(feature_x + (kFeaturePaddingX + kIconWidth +
                                             kPaddingBetweenIconAndText),
                                feature_y + (kTextPaddingY + kFontSize)),
                     Font::kUseFallbackIfFontNotReady, 1.0f, flags);
}

void PlaceholderImage::DrawPattern(GraphicsContext& context,
                                   const FloatRect& src_rect,
                                   const FloatSize& scale,
                                   const FloatPoint& phase,
                                   SkBlendMode mode,
                                   const FloatRect& dest_rect,
                                   const FloatSize& repeat_spacing) {
  DCHECK(context.Canvas());

  PaintFlags flags = context.FillFlags();
  flags.setBlendMode(mode);

  // Ignore the pattern specifications and just draw a single placeholder image
  // over the whole |dest_rect|. This is done in order to prevent repeated icons
  // from cluttering tiled background images.
  Draw(context.Canvas(), flags, dest_rect, src_rect,
       kDoNotRespectImageOrientation, kClampImageToSourceRect);
}

void PlaceholderImage::DestroyDecodedData() {
  paint_record_for_current_frame_.reset();
}

}  // namespace blink
