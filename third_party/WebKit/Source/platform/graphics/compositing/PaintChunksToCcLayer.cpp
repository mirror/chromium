// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/compositing/PaintChunksToCcLayer.h"

#include "cc/base/render_surface_filters.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_op_buffer.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/DisplayItemList.h"
#include "platform/graphics/paint/DrawingDisplayItem.h"
#include "platform/graphics/paint/GeometryMapper.h"
#include "platform/graphics/paint/PaintChunk.h"
#include "platform/graphics/paint/PropertyTreeState.h"
#include "platform/graphics/paint/RasterInvalidationTracking.h"

namespace blink {

namespace {

static gfx::Rect g_large_rect(-200000, -200000, 400000, 400000);
static void AppendDisplayItemToCcDisplayItemList(
    const DisplayItem& display_item,
    cc::DisplayItemList& list) {
  DCHECK(DisplayItem::IsDrawingType(display_item.GetType()));
  if (!DisplayItem::IsDrawingType(display_item.GetType()))
    return;

  sk_sp<const PaintRecord> record =
      static_cast<const DrawingDisplayItem&>(display_item).GetPaintRecord();
  if (!record)
    return;
  // In theory we would pass the bounds of the record, previously done as:
  // gfx::Rect bounds = gfx::SkIRectToRect(record->cullRect().roundOut());
  // or use the visual rect directly. However, clip content layers attempt
  // to raster in a different space than that of the visual rects. We'll be
  // reworking visual rects further for SPv2, so for now we just pass a
  // visual rect large enough to make sure items raster.
  cc::PaintOpBuffer* buffer = list.StartPaint();
  buffer->push<cc::DrawRecordOp>(std::move(record));
  list.EndPaintOfUnpaired(g_large_rect);
}

// TODO(trchen): Find a good home for these utility functions.
// Returns nullptr if 'ancestor' is not a strict ancestor of 'node'.
// Otherwise, return the child of 'ancestor' that is an ancestor of 'node' or
// 'node' itself.
static const EffectPaintPropertyNode* StrictChildOfAlongPath(
    const EffectPaintPropertyNode* ancestor,
    const EffectPaintPropertyNode* node) {
  for (; node; node = node->Parent()) {
    if (node->Parent() == ancestor)
      return node;
  }
  return nullptr;
}
static const ClipPaintPropertyNode* StrictChildOfAlongPath(
    const ClipPaintPropertyNode* ancestor,
    const ClipPaintPropertyNode* node) {
  for (; node; node = node->Parent()) {
    if (node->Parent() == ancestor)
      return node;
  }
  return nullptr;
}

class TransformHelper {
 public:
  TransformHelper(const TransformPaintPropertyNode* source,
                  const TransformPaintPropertyNode* destination,
                  cc::DisplayItemList& list)
      : source_(source), destination_(destination), list_(list) {
    if (source_ == destination_)
      return;
    cc::PaintOpBuffer* buffer = list_.StartPaint();
    buffer->push<cc::SaveOp>();
    buffer->push<cc::ConcatOp>(
        static_cast<SkMatrix>(TransformationMatrix::ToSkMatrix44(
            GeometryMapper::SourceToDestinationProjection(source_,
                                                          destination_))));
    list_.EndPaintOfPairedBegin();
  }
  ~TransformHelper() {
    if (source_ == destination_)
      return;
    cc::PaintOpBuffer* buffer = list_.StartPaint();
    buffer->push<cc::RestoreOp>();
    list_.EndPaintOfPairedEnd();
  }

 private:
  const TransformPaintPropertyNode* source_;
  const TransformPaintPropertyNode* destination_;
  cc::DisplayItemList& list_;
};

}  // unnamed namespace

void PaintChunksToCcLayer::ConvertRecursively(
    const TransformPaintPropertyNode* current_transform,
    const ClipPaintPropertyNode* current_clip,
    const EffectPaintPropertyNode* current_effect) {
  // This function walks through all chunks that inherits current_clip and
  // current_effect. Chunks having exactly current_clip and current_effect
  // are converted into drawing items directly. Otherwise the outermost
  // clip or effect gets "pushed" as meta display items and the function
  // recurs with the updated state.
  while (chunk_it_ != paint_chunks_.end()) {
    const PaintChunk& chunk = **chunk_it_;
    const PropertyTreeState& chunk_state = chunk.properties.property_tree_state;

    // First determine whether the next thing in draw order is a nested effect,
    // a chunk, or reached the end of the current effect.
    const ClipPaintPropertyNode* target_clip = nullptr;
    if (chunk_state.Effect() == current_effect) {
      // The next thing is a chunk. Now determine whether the current clip
      // matches the desired clip.
      if (chunk_state.Clip() != current_clip) {
        target_clip = chunk_state.Clip();
        // Clip mismatches. Falling through.
      } else {
        // Clip matches. Setup CTM and emit drawing items.
        TransformHelper helper(chunk_state.Transform(), current_transform,
                               cc_list_);
        for (const auto& item : display_items_.ItemsInPaintChunk(chunk))
          AppendDisplayItemToCcDisplayItemList(item, cc_list_);
        chunk_it_++;
        continue;
      }
    } else {
      const EffectPaintPropertyNode* sub_effect =
          StrictChildOfAlongPath(current_effect, chunk_state.Effect());
      // Reached the end of current effect. Exit current recursion.
      if (!sub_effect)
        break;
      // At this point we know we are going to enter an effect, but before
      // entering, first determine whether the effect's output clip matches
      // the current clip.
      if (sub_effect->OutputClip() != current_clip) {
        target_clip = sub_effect->OutputClip();
        // Clip mismatches. Falling through.
      } else {
        // Clip matches. Switch CTM to the outermost nested effect's local
        // space, pushes the outermost nested effect then recur.
        TransformHelper helper(sub_effect->LocalTransformSpace(),
                               current_transform, cc_list_);
        {
          cc::PaintOpBuffer* buffer = cc_list_.StartPaint();

          cc::PaintFlags flags;
          flags.setBlendMode(sub_effect->BlendMode());
          // TODO(ajuma): This should really be rounding instead of flooring the
          // alpha value, but that breaks slimming paint reftests.
          flags.setAlpha(static_cast<uint8_t>(
              gfx::ToFlooredInt(255 * sub_effect->Opacity())));
          flags.setColorFilter(
              GraphicsContext::WebCoreColorFilterToSkiaColorFilter(
                  sub_effect->GetColorFilter()));
          buffer->push<cc::SaveLayerOp>(nullptr, &flags);

          // TODO(chrishtr): specify origin of the filter.
          FloatPoint filter_origin;
          buffer->push<cc::SaveOp>();
          buffer->push<cc::TranslateOp>(filter_origin.X(), filter_origin.Y());

          // The size parameter is only used to computed the origin of zoom
          // operation, which we never generate.
          gfx::SizeF empty;
          cc::PaintFlags filter_flags;
          filter_flags.setImageFilter(
              cc::RenderSurfaceFilters::BuildImageFilter(
                  sub_effect->Filter().AsCcFilterOperations(), empty));
          buffer->push<cc::SaveLayerOp>(nullptr, &filter_flags);
          buffer->push<cc::TranslateOp>(-filter_origin.X(), -filter_origin.Y());

          cc_list_.EndPaintOfPairedBegin();
        }
        ConvertRecursively(sub_effect->LocalTransformSpace(), current_clip,
                           sub_effect);
        {
          cc::PaintOpBuffer* buffer = cc_list_.StartPaint();
          buffer->push<cc::RestoreOp>();
          buffer->push<cc::RestoreOp>();
          buffer->push<cc::RestoreOp>();
          cc_list_.EndPaintOfPairedEnd();
        }
        continue;
      }
    }

    // We reached here only if the above fell through due to clip mismatch.
    // (Otherwise some chunks would be consumed and continue would have been
    // invoked.) Determine whether the target clip is a descendant of the
    // current clip or we need to exit some clip first.
    DCHECK(target_clip && target_clip != current_clip);
    const ClipPaintPropertyNode* sub_clip =
        StrictChildOfAlongPath(current_clip, target_clip);
    // The current clip is not an ancestor of the target. Exit current
    // recursion.
    if (!sub_clip)
      break;

    // Switch CTM to the outermost nested clip's local space, pushes the
    // outermost nested clip then recur.
    TransformHelper helper(sub_clip->LocalTransformSpace(), current_transform,
                           cc_list_);
    {
      cc::PaintOpBuffer* buffer = cc_list_.StartPaint();
      buffer->push<cc::SaveOp>();
      buffer->push<cc::ClipRectOp>(
          static_cast<SkRect>(sub_clip->ClipRect().Rect()),
          SkClipOp::kIntersect, false);
      buffer->push<cc::ClipRRectOp>(static_cast<SkRRect>(sub_clip->ClipRect()),
                                    SkClipOp::kIntersect, true);
      cc_list_.EndPaintOfPairedBegin();
    }
    ConvertRecursively(sub_clip->LocalTransformSpace(), sub_clip,
                       current_effect);
    {
      cc::PaintOpBuffer* buffer = cc_list_.StartPaint();
      buffer->push<cc::RestoreOp>();
      cc_list_.EndPaintOfPairedEnd();
    }
  }
}

PaintChunksToCcLayer::PaintChunksToCcLayer(
    const Vector<const PaintChunk*>& paint_chunks,
    const DisplayItemList& display_items,
    cc::DisplayItemList& cc_list)
    : paint_chunks_(paint_chunks),
      chunk_it_(paint_chunks.begin()),
      display_items_(display_items),
      cc_list_(cc_list) {}

PaintChunksToCcLayer::~PaintChunksToCcLayer() {
  DCHECK_EQ(paint_chunks_.end(), chunk_it_);
}

scoped_refptr<cc::DisplayItemList> PaintChunksToCcLayer::Convert(
    const Vector<const PaintChunk*>& paint_chunks,
    const PropertyTreeState& layer_state,
    const gfx::Vector2dF& layer_offset,
    const DisplayItemList& display_items,
    RasterUnderInvalidationCheckingParams* under_invalidation_checking_params) {
  auto cc_list = make_scoped_refptr(new cc::DisplayItemList);

  bool need_translate = !layer_offset.IsZero();
  if (need_translate) {
    cc::PaintOpBuffer* buffer = cc_list->StartPaint();
    buffer->push<cc::SaveOp>();
    buffer->push<cc::TranslateOp>(-layer_offset.x(), -layer_offset.y());
    cc_list->EndPaintOfPairedBegin();
  }

  PaintChunksToCcLayer(paint_chunks, display_items, *cc_list)
      .ConvertRecursively(layer_state.Transform(), layer_state.Clip(),
                          layer_state.Effect());

  if (need_translate) {
    cc::PaintOpBuffer* buffer = cc_list->StartPaint();
    buffer->push<cc::RestoreOp>();
    cc_list->EndPaintOfPairedEnd();
  }

  if (under_invalidation_checking_params) {
    auto& params = *under_invalidation_checking_params;
    PaintRecorder recorder;
    recorder.beginRecording(params.interest_rect);
    // Create a complete cloned list for under-invalidation checking. We can't
    // use cc_list because it is not finalized yet.
    auto list_clone =
        Convert(paint_chunks, layer_state, layer_offset, display_items);
    recorder.getRecordingCanvas()->drawPicture(list_clone->ReleaseAsRecord());
    params.tracking.CheckUnderInvalidations(params.debug_name,
                                            recorder.finishRecordingAsPicture(),
                                            params.interest_rect);
    if (auto record = params.tracking.under_invalidation_record) {
      cc::PaintOpBuffer* buffer = cc_list->StartPaint();
      buffer->push<cc::DrawRecordOp>(record);
      cc_list->EndPaintOfUnpaired(g_large_rect);
    }
  }

  cc_list->Finalize();
  return cc_list;
}

}  // namespace blink
