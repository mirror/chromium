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

constexpr gfx::Rect g_large_rect(-200000, -200000, 400000, 400000);
void AppendDisplayItemToCcDisplayItemList(const DisplayItem& display_item,
                                          cc::DisplayItemList& list) {
  DCHECK(DisplayItem::IsDrawingType(display_item.GetType()));

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

void AppendRestore(cc::DisplayItemList& list, size_t n) {
  cc::PaintOpBuffer* buffer = list.StartPaint();
  while (n--)
    buffer->push<cc::RestoreOp>();
  list.EndPaintOfPairedEnd();
}

}  // unnamed namespace

void PaintChunksToCcLayer::ConvertInternal(
    const Vector<const PaintChunk*>& paint_chunks,
    const PropertyTreeState& layer_state,
    const DisplayItemList& display_items,
    cc::DisplayItemList& cc_list) {
  const TransformPaintPropertyNode* current_transform = layer_state.Transform();
  const ClipPaintPropertyNode* current_clip = layer_state.Clip();
  const EffectPaintPropertyNode* current_effect = layer_state.Effect();

  struct ClipEntry {
    const TransformPaintPropertyNode* transform;
    const ClipPaintPropertyNode* clip;
  };
  Vector<ClipEntry> clip_stack;
  auto SwitchToClip = [&](const ClipPaintPropertyNode* target_clip) {
    if (target_clip == current_clip)
      return;
    const ClipPaintPropertyNode* lca_clip =
        GeometryMapper::LowestCommonAncestor(target_clip, current_clip);
    while (current_clip != lca_clip) {
      DCHECK(clip_stack.size())
          << "Error: Chunk has a clip that escaped its effect's clip.";
      if (!clip_stack.size())
        break;
      ClipEntry& previous_state = clip_stack.back();
      current_transform = previous_state.transform;
      current_clip = previous_state.clip;
      clip_stack.pop_back();
      AppendRestore(cc_list, 1);
    }

    Vector<const ClipPaintPropertyNode*, 1u> pending_clips;
    for (const ClipPaintPropertyNode* clip = target_clip; clip != current_clip;
         clip = clip->Parent())
      pending_clips.push_back(clip);

    for (size_t i = pending_clips.size(); i--;) {
      const ClipPaintPropertyNode* sub_clip = pending_clips[i];
      DCHECK_EQ(current_clip, sub_clip->Parent());

      clip_stack.emplace_back(ClipEntry{current_transform, current_clip});
      cc::PaintOpBuffer* buffer = cc_list.StartPaint();

      buffer->push<cc::SaveOp>();
      const TransformPaintPropertyNode* target_transform =
          sub_clip->LocalTransformSpace();
      if (current_transform != target_transform) {
        buffer->push<cc::ConcatOp>(
            static_cast<SkMatrix>(TransformationMatrix::ToSkMatrix44(
                GeometryMapper::SourceToDestinationProjection(
                    target_transform, current_transform))));
        current_transform = target_transform;
      }
      buffer->push<cc::ClipRectOp>(
          static_cast<SkRect>(sub_clip->ClipRect().Rect()),
          SkClipOp::kIntersect, false);
      buffer->push<cc::ClipRRectOp>(static_cast<SkRRect>(sub_clip->ClipRect()),
                                    SkClipOp::kIntersect, true);

      cc_list.EndPaintOfPairedBegin();
      current_clip = sub_clip;
    }
  };

  struct EffectEntry {
    const TransformPaintPropertyNode* transform;
    const ClipPaintPropertyNode* clip;
    const EffectPaintPropertyNode* effect;
    Vector<ClipEntry> clip_stack;
  };
  Vector<EffectEntry> effect_stack;
  auto SwitchToEffect = [&](const EffectPaintPropertyNode* target_effect) {
    if (target_effect == current_effect)
      return;
    const EffectPaintPropertyNode* lca_effect =
        GeometryMapper::LowestCommonAncestor(target_effect, current_effect);
    while (current_effect != lca_effect) {
      DCHECK(effect_stack.size()) << "Error: Chunk layerized into a layer with "
                                     "an effect that's too deep.";
      if (!effect_stack.size())
        break;

      EffectEntry& previous_state = effect_stack.back();
      current_transform = previous_state.transform;
      current_clip = previous_state.clip;
      current_effect = previous_state.effect;
      for (size_t i = clip_stack.size(); i--;)
        AppendRestore(cc_list, 1);
      clip_stack.swap(previous_state.clip_stack);
      effect_stack.pop_back();
      AppendRestore(cc_list, 2);
    }

    Vector<const EffectPaintPropertyNode*, 1u> pending_effects;
    for (const EffectPaintPropertyNode* effect = target_effect;
         effect != current_effect; effect = effect->Parent())
      pending_effects.push_back(effect);

    for (size_t i = pending_effects.size(); i--;) {
      const EffectPaintPropertyNode* sub_effect = pending_effects[i];
      DCHECK_EQ(current_effect, sub_effect->Parent());
      SwitchToClip(sub_effect->OutputClip());

      effect_stack
          .emplace_back(
              EffectEntry{current_transform, current_clip, current_effect, {}})
          .clip_stack.swap(clip_stack);
      cc::PaintOpBuffer* buffer = cc_list.StartPaint();

      cc::PaintFlags flags;
      flags.setBlendMode(sub_effect->BlendMode());
      // TODO(ajuma): This should really be rounding instead of flooring the
      // alpha value, but that breaks slimming paint reftests.
      flags.setAlpha(
          static_cast<uint8_t>(gfx::ToFlooredInt(255 * sub_effect->Opacity())));
      flags.setColorFilter(GraphicsContext::WebCoreColorFilterToSkiaColorFilter(
          sub_effect->GetColorFilter()));
      buffer->push<cc::SaveLayerOp>(nullptr, &flags);

      const TransformPaintPropertyNode* target_transform =
          sub_effect->LocalTransformSpace();
      if (current_transform != target_transform) {
        buffer->push<cc::ConcatOp>(
            static_cast<SkMatrix>(TransformationMatrix::ToSkMatrix44(
                GeometryMapper::SourceToDestinationProjection(
                    target_transform, current_transform))));
        current_transform = target_transform;
      }

      // TODO(chrishtr): specify origin of the filter.
      FloatPoint filter_origin;
      buffer->push<cc::TranslateOp>(filter_origin.X(), filter_origin.Y());

      // The size parameter is only used to computed the origin of zoom
      // operation, which we never generate.
      gfx::SizeF empty;
      cc::PaintFlags filter_flags;
      filter_flags.setImageFilter(cc::RenderSurfaceFilters::BuildImageFilter(
          sub_effect->Filter().AsCcFilterOperations(), empty));
      buffer->push<cc::SaveLayerOp>(nullptr, &filter_flags);
      buffer->push<cc::TranslateOp>(-filter_origin.X(), -filter_origin.Y());

      cc_list.EndPaintOfPairedBegin();
      current_effect = sub_effect;
    }
  };

  for (auto chunk_it = paint_chunks.begin(); chunk_it != paint_chunks.end();
       chunk_it++) {
    const PaintChunk& chunk = **chunk_it;
    const PropertyTreeState& chunk_state = chunk.properties.property_tree_state;
    SwitchToEffect(chunk_state.Effect());
    SwitchToClip(chunk_state.Clip());
    bool transformed = chunk_state.Transform() != current_transform;
    if (transformed) {
      cc::PaintOpBuffer* buffer = cc_list.StartPaint();
      buffer->push<cc::SaveOp>();
      buffer->push<cc::ConcatOp>(
          static_cast<SkMatrix>(TransformationMatrix::ToSkMatrix44(
              GeometryMapper::SourceToDestinationProjection(
                  chunk_state.Transform(), current_transform))));
      cc_list.EndPaintOfPairedBegin();
    }
    for (const auto& item : display_items.ItemsInPaintChunk(chunk))
      AppendDisplayItemToCcDisplayItemList(item, cc_list);
    if (transformed)
      AppendRestore(cc_list, 1);
  }

  for (size_t i = clip_stack.size(); i--;)
    AppendRestore(cc_list, 1);
  for (size_t i = effect_stack.size(); i--;) {
    AppendRestore(cc_list, 2);
    for (size_t j = effect_stack[i].clip_stack.size(); j--;)
      AppendRestore(cc_list, 1);
  }
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

  ConvertInternal(paint_chunks, layer_state, display_items, *cc_list);

  if (need_translate)
    AppendRestore(*cc_list, 1);

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
