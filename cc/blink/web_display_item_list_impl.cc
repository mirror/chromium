// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/blink/web_display_item_list_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "cc/base/render_surface_filters.h"
#include "cc/paint/clip_display_item.h"
#include "cc/paint/clip_path_display_item.h"
#include "cc/paint/compositing_display_item.h"
#include "cc/paint/drawing_display_item.h"
#include "cc/paint/filter_display_item.h"
#include "cc/paint/float_clip_display_item.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/transform_display_item.h"
#include "third_party/WebKit/public/platform/WebFloatRect.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkMatrix44.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/transform.h"

namespace cc_blink {

namespace {

}  // namespace

WebDisplayItemListImpl::WebDisplayItemListImpl()
    : display_item_list_(new cc::DisplayItemList) {}

WebDisplayItemListImpl::WebDisplayItemListImpl(
    cc::DisplayItemList* display_list)
    : display_item_list_(display_list) {
}

void WebDisplayItemListImpl::AppendDrawingItem(
    const blink::WebRect& visual_rect,
    sk_sp<const cc::PaintRecord> record,
    const blink::WebRect& record_bounds) {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::DrawRecordOp>(std::move(record));
  display_item_list_->EndPaintOfUnpaired(visual_rect);
}

void WebDisplayItemListImpl::AppendClipItem(
    const blink::WebRect& clip_rect,
    const blink::WebVector<SkRRect>& rounded_clip_rects) {
  bool antialias = true;
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::SaveOp>();
  buffer->push<cc::ClipRectOp>(gfx::RectToSkRect(clip_rect),
                               SkClipOp::kIntersect, antialias);
  for (const SkRRect& rrect : rounded_clip_rects) {
    if (rrect.isRect()) {
      buffer->push<cc::ClipRectOp>(rrect.rect(), SkClipOp::kIntersect,
                                   antialias);
    } else {
      buffer->push<cc::ClipRRectOp>(rrect, SkClipOp::kIntersect, antialias);
    }
  }
  display_item_list_->EndPaintOfPairedBegin();
}

void WebDisplayItemListImpl::AppendEndClipItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::AppendClipPathItem(const SkPath& clip_path,
                                                bool antialias) {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::SaveOp>();
  buffer->push<cc::ClipPathOp>(clip_path, SkClipOp::kIntersect, antialias);
  display_item_list_->EndPaintOfPairedBegin();
}

void WebDisplayItemListImpl::AppendEndClipPathItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::AppendFloatClipItem(
    const blink::WebFloatRect& clip_rect) {
  bool antialias = false;
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::SaveOp>();
  buffer->push<cc::ClipRectOp>(gfx::RectFToSkRect(clip_rect),
                               SkClipOp::kIntersect, antialias);
  display_item_list_->EndPaintOfPairedBegin();
}

void WebDisplayItemListImpl::AppendEndFloatClipItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::AppendTransformItem(const SkMatrix44& matrix) {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::SaveOp>();
  if (!matrix.isIdentity())
    buffer->push<cc::ConcatOp>(static_cast<SkMatrix>(matrix));
  display_item_list_->EndPaintOfPairedBegin();
}

void WebDisplayItemListImpl::AppendEndTransformItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::AppendCompositingItem(
    float opacity,
    SkBlendMode xfermode,
    SkRect* bounds,
    SkColorFilter* color_filter) {
  DCHECK_GE(opacity, 0.f);
  DCHECK_LE(opacity, 1.f);

  cc::PaintFlags flags;
  flags.setBlendMode(xfermode);
  // TODO(ajuma): This should really be rounding instead of flooring the alpha
  // value, but that breaks slimming paint reftests.
  flags.setAlpha(static_cast<uint8_t>(gfx::ToFlooredInt(255 * opacity)));
  flags.setColorFilter(sk_ref_sp(color_filter));

  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::SaveLayerOp>(bounds, &flags);
  display_item_list_->EndPaintOfPairedBegin();
}

void WebDisplayItemListImpl::AppendEndCompositingItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::AppendFilterItem(
    const cc::FilterOperations& filters,
    const blink::WebFloatRect& filter_bounds,
    const blink::WebFloatPoint& origin) {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();

  buffer->push<cc::SaveOp>();
  buffer->push<cc::TranslateOp>(origin.x, origin.y);

  cc::PaintFlags flags;
  flags.setImageFilter(cc::RenderSurfaceFilters::BuildImageFilter(
      filters, gfx::SizeF(filter_bounds.width, filter_bounds.height)));

  SkRect layer_bounds = gfx::RectFToSkRect(filter_bounds);
  layer_bounds.offset(-origin.x, -origin.y);
  buffer->push<cc::SaveLayerOp>(&layer_bounds, &flags);
  buffer->push<cc::TranslateOp>(-origin.x, -origin.y);

  display_item_list_->EndPaintOfPairedBegin(
      gfx::ToEnclosingRect(filter_bounds));
}

void WebDisplayItemListImpl::AppendEndFilterItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::AppendScrollItem(
    const blink::WebSize& scroll_offset,
    ScrollContainerId) {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::SaveOp>();
  buffer->push<cc::TranslateOp>(-scroll_offset.width, -scroll_offset.height);
  display_item_list_->EndPaintOfPairedBegin();
}

void WebDisplayItemListImpl::AppendEndScrollItem() {
  cc::PaintOpBuffer* buffer = display_item_list_->StartPaint();
  buffer->push<cc::RestoreOp>();
  display_item_list_->EndPaintOfPairedEnd();
}

void WebDisplayItemListImpl::SetIsSuitableForGpuRasterization(bool isSuitable) {
  display_item_list_->SetIsSuitableForGpuRasterization(isSuitable);
}

WebDisplayItemListImpl::~WebDisplayItemListImpl() {
}

}  // namespace cc_blink
