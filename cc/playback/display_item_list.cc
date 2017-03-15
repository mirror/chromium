// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/playback/display_item_list.h"

#include <stddef.h>

#include <string>

#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/base/math_util.h"
#include "cc/debug/picture_debug_util.h"
#include "cc/playback/clip_display_item.h"
#include "cc/playback/clip_path_display_item.h"
#include "cc/playback/compositing_display_item.h"
#include "cc/playback/drawing_display_item.h"
#include "cc/playback/filter_display_item.h"
#include "cc/playback/float_clip_display_item.h"
#include "cc/playback/largest_display_item.h"
#include "cc/playback/transform_display_item.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/skia_util.h"

namespace cc {

namespace {

// We don't perform per-layer solid color analysis when there are too many skia
// operations.
const int kOpCountThatIsOkToAnalyze = 10;

bool GetCanvasClipBounds(SkCanvas* canvas, gfx::Rect* clip_bounds) {
  SkRect canvas_clip_bounds;
  if (!canvas->getLocalClipBounds(&canvas_clip_bounds))
    return false;
  *clip_bounds = ToEnclosingRect(gfx::SkRectToRectF(canvas_clip_bounds));
  return true;
}

const int kDefaultNumDisplayItemsToReserve = 100;

}  // namespace

DisplayItemList::Inputs::Inputs()
    : items(LargestDisplayItemSize(),
            LargestDisplayItemSize() * kDefaultNumDisplayItemsToReserve) {}

DisplayItemList::Inputs::~Inputs() = default;

DisplayItemList::DisplayItemList() = default;

DisplayItemList::~DisplayItemList() = default;

void DisplayItemList::Raster(SkCanvas* canvas,
                             SkPicture::AbortCallback* callback,
                             const gfx::Rect& canvas_target_playback_rect,
                             float contents_scale) const {
  canvas->save();
  if (!canvas_target_playback_rect.IsEmpty()) {
    // canvas_target_playback_rect is specified in device space. We can't
    // use clipRect because canvas CTM will be applied on it. Use clipRegion
    // instead because it ignores canvas CTM.
    SkRegion device_clip;
    device_clip.setRect(gfx::RectToSkIRect(canvas_target_playback_rect));
    canvas->clipRegion(device_clip);
  }
  canvas->scale(contents_scale, contents_scale);
  Raster(canvas, callback);
  canvas->restore();
}

DISABLE_CFI_PERF
void DisplayItemList::Raster(SkCanvas* canvas,
                             SkPicture::AbortCallback* callback) const {
  gfx::Rect canvas_playback_rect;
  if (!GetCanvasClipBounds(canvas, &canvas_playback_rect))
    return;

  std::vector<size_t> indices;
  rtree_.Search(canvas_playback_rect, &indices);
  for (size_t index : indices) {
    inputs_.items[index].Raster(canvas, callback);
    // We use a callback during solid color analysis on the compositor thread to
    // break out early. Since we're handling a sequence of pictures via rtree
    // query results ourselves, we have to respect the callback and early out.
    if (callback && callback->abort())
      break;
  }
}

void DisplayItemList::GrowCurrentBeginItemVisualRect(
    const gfx::Rect& visual_rect) {
  if (!inputs_.begin_item_indices.empty())
    inputs_.visual_rects[inputs_.begin_item_indices.back()].Union(visual_rect);
}

void DisplayItemList::Finalize() {
  TRACE_EVENT0("cc", "DisplayItemList::Finalize");
  DCHECK(inputs_.items.size() == inputs_.visual_rects.size())
      << "items.size() " << inputs_.items.size() << " visual_rects.size() "
      << inputs_.visual_rects.size();
  rtree_.Build(inputs_.visual_rects);

  if (!retain_visual_rects_)
    // This clears both the vector and the vector's capacity, since
    // visual_rects won't be used anymore.
    std::vector<gfx::Rect>().swap(inputs_.visual_rects);
}

bool DisplayItemList::IsSuitableForGpuRasterization() const {
  // TODO(wkorman): This is more permissive than Picture's implementation, since
  // none of the items might individually trigger a veto even though they
  // collectively have enough "bad" operations that a corresponding Picture
  // would get vetoed. See crbug.com/513016.
  return inputs_.all_items_are_suitable_for_gpu_rasterization;
}

int DisplayItemList::ApproximateOpCount() const {
  return approximate_op_count_;
}

size_t DisplayItemList::ApproximateMemoryUsage() const {
  size_t memory_usage = sizeof(*this);

  size_t external_memory_usage = 0;
  for (const auto& item : inputs_.items) {
    size_t bytes = 0;
    switch (item.type()) {
      case DisplayItem::CLIP:
        bytes = static_cast<const ClipDisplayItem&>(item).ExternalMemoryUsage();
        break;
      case DisplayItem::CLIP_PATH:
        bytes =
            static_cast<const ClipPathDisplayItem&>(item).ExternalMemoryUsage();
        break;
      case DisplayItem::COMPOSITING:
        bytes = static_cast<const CompositingDisplayItem&>(item)
                    .ExternalMemoryUsage();
        break;
      case DisplayItem::DRAWING:
        bytes =
            static_cast<const DrawingDisplayItem&>(item).ExternalMemoryUsage();
        break;
      case DisplayItem::FLOAT_CLIP:
        bytes = static_cast<const FloatClipDisplayItem&>(item)
                    .ExternalMemoryUsage();
        break;
      case DisplayItem::FILTER:
        bytes =
            static_cast<const FilterDisplayItem&>(item).ExternalMemoryUsage();
        break;
      case DisplayItem::TRANSFORM:
        bytes = static_cast<const TransformDisplayItem&>(item)
                    .ExternalMemoryUsage();
        break;
      case DisplayItem::END_CLIP:
      case DisplayItem::END_CLIP_PATH:
      case DisplayItem::END_COMPOSITING:
      case DisplayItem::END_FLOAT_CLIP:
      case DisplayItem::END_FILTER:
      case DisplayItem::END_TRANSFORM:
        break;
    }
    external_memory_usage += bytes;
  }

  // Memory outside this class due to |items_|.
  memory_usage += inputs_.items.GetCapacityInBytes() + external_memory_usage;

  // TODO(jbroman): Does anything else owned by this class substantially
  // contribute to memory usage?
  // TODO(vmpstr): Probably DiscardableImageMap is worth counting here.

  return memory_usage;
}

bool DisplayItemList::ShouldBeAnalyzedForSolidColor() const {
  return ApproximateOpCount() <= kOpCountThatIsOkToAnalyze;
}

void DisplayItemList::EmitTraceSnapshot() const {
  bool include_items;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.display_items"), &include_items);
  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("cc.debug.display_items") ","
      TRACE_DISABLED_BY_DEFAULT("cc.debug.picture") ","
      TRACE_DISABLED_BY_DEFAULT("devtools.timeline.picture"),
      "cc::DisplayItemList", this, CreateTracedValue(include_items));
}

std::unique_ptr<base::trace_event::TracedValue>
DisplayItemList::CreateTracedValue(bool include_items) const {
  auto state = base::MakeUnique<base::trace_event::TracedValue>();
  state->BeginDictionary("params");

  if (include_items) {
    state->BeginArray("items");

    auto visual_rects_it = inputs_.visual_rects.begin();
    for (const DisplayItem& base_item : inputs_.items) {
      gfx::Rect visual_rect;
      if (visual_rects_it != inputs_.visual_rects.end()) {
        visual_rect = *visual_rects_it;
        ++visual_rects_it;
      }

      switch (base_item.type()) {
        case DisplayItem::CLIP: {
          const auto& item = static_cast<const ClipDisplayItem&>(base_item);
          std::string output =
              base::StringPrintf("ClipDisplayItem rect: [%s] visualRect: [%s]",
                                 item.clip_rect().ToString().c_str(),
                                 visual_rect.ToString().c_str());
          for (const SkRRect& rounded_rect : item.rounded_clip_rects()) {
            base::StringAppendF(
                &output, " rounded_rect: [rect: [%s]",
                gfx::SkRectToRectF(rounded_rect.rect()).ToString().c_str());
            base::StringAppendF(&output, " radii: [");
            SkVector upper_left_radius =
                rounded_rect.radii(SkRRect::kUpperLeft_Corner);
            base::StringAppendF(&output, "[%f,%f],", upper_left_radius.x(),
                                upper_left_radius.y());
            SkVector upper_right_radius =
                rounded_rect.radii(SkRRect::kUpperRight_Corner);
            base::StringAppendF(&output, " [%f,%f],", upper_right_radius.x(),
                                upper_right_radius.y());
            SkVector lower_right_radius =
                rounded_rect.radii(SkRRect::kLowerRight_Corner);
            base::StringAppendF(&output, " [%f,%f],", lower_right_radius.x(),
                                lower_right_radius.y());
            SkVector lower_left_radius =
                rounded_rect.radii(SkRRect::kLowerLeft_Corner);
            base::StringAppendF(&output, " [%f,%f]]", lower_left_radius.x(),
                                lower_left_radius.y());
          }
          state->AppendString(output);
          break;
        }
        case DisplayItem::END_CLIP:
          state->AppendString(
              base::StringPrintf("EndClipDisplayItem visualRect: [%s]",
                                 visual_rect.ToString().c_str()));
          break;
        case DisplayItem::CLIP_PATH: {
          const auto& item = static_cast<const ClipPathDisplayItem&>(base_item);
          state->AppendString(base::StringPrintf(
              "ClipPathDisplayItem length: %d visualRect: [%s]",
              item.clip_path().countPoints(), visual_rect.ToString().c_str()));
          break;
        }
        case DisplayItem::END_CLIP_PATH:
          state->AppendString(
              base::StringPrintf("EndClipPathDisplayItem visualRect: [%s]",
                                 visual_rect.ToString().c_str()));
          break;
        case DisplayItem::COMPOSITING: {
          const auto& item =
              static_cast<const CompositingDisplayItem&>(base_item);
          std::string output = base::StringPrintf(
              "CompositingDisplayItem alpha: %d, xfermode: %d, visualRect: "
              "[%s]",
              item.alpha(), static_cast<int>(item.xfermode()),
              visual_rect.ToString().c_str());
          if (item.has_bounds()) {
            base::StringAppendF(
                &output, ", bounds: [%s]",
                gfx::SkRectToRectF(item.bounds()).ToString().c_str());
          }
          state->AppendString(output);
          break;
        }
        case DisplayItem::END_COMPOSITING:
          state->AppendString(
              base::StringPrintf("EndCompositingDisplayItem visualRect: [%s]",
                                 visual_rect.ToString().c_str()));
          break;
        case DisplayItem::DRAWING: {
          const auto& item = static_cast<const DrawingDisplayItem&>(base_item);
          state->BeginDictionary();
          state->SetString("name", "DrawingDisplayItem");

          state->BeginArray("visualRect");
          state->AppendInteger(visual_rect.x());
          state->AppendInteger(visual_rect.y());
          state->AppendInteger(visual_rect.width());
          state->AppendInteger(visual_rect.height());
          state->EndArray();

          state->BeginArray("cullRect");
          state->AppendInteger(item.picture().cullRect().x());
          state->AppendInteger(item.picture().cullRect().y());
          state->AppendInteger(item.picture().cullRect().width());
          state->AppendInteger(item.picture().cullRect().height());
          state->EndArray();

          std::string b64_picture;
          PictureDebugUtil::SerializeAsBase64(ToSkPicture(&item.picture()),
                                              &b64_picture);
          state->SetString("skp64", b64_picture);
          state->EndDictionary();
          break;
        }
        case DisplayItem::FILTER: {
          const auto& item = static_cast<const FilterDisplayItem&>(base_item);
          state->AppendString(base::StringPrintf(
              "FilterDisplayItem bounds: [%s] visualRect: [%s]",
              item.bounds().ToString().c_str(),
              visual_rect.ToString().c_str()));
          break;
        }
        case DisplayItem::END_FILTER:
          state->AppendString(
              base::StringPrintf("EndFilterDisplayItem visualRect: [%s]",
                                 visual_rect.ToString().c_str()));
          break;
        case DisplayItem::FLOAT_CLIP: {
          const auto& item =
              static_cast<const FloatClipDisplayItem&>(base_item);
          state->AppendString(base::StringPrintf(
              "FloatClipDisplayItem rect: [%s] visualRect: [%s]",
              item.clip_rect().ToString().c_str(),
              visual_rect.ToString().c_str()));
          break;
        }
        case DisplayItem::END_FLOAT_CLIP:
          state->AppendString(
              base::StringPrintf("EndFloatClipDisplayItem visualRect: [%s]",
                                 visual_rect.ToString().c_str()));
          break;
        case DisplayItem::TRANSFORM: {
          const auto& item =
              static_cast<const TransformDisplayItem&>(base_item);
          state->AppendString(base::StringPrintf(
              "TransformDisplayItem transform: [%s] visualRect: [%s]",
              item.transform().ToString().c_str(),
              visual_rect.ToString().c_str()));
          break;
        }
        case DisplayItem::END_TRANSFORM:
          state->AppendString(
              base::StringPrintf("EndTransformDisplayItem visualRect: [%s]",
                                 visual_rect.ToString().c_str()));
          break;
      }
    }
    state->EndArray();  // "items".
  }

  MathUtil::AddToTracedValue("layer_rect", rtree_.GetBounds(), state.get());
  state->EndDictionary();  // "params".

  {
    SkPictureRecorder recorder;
    gfx::Rect bounds = rtree_.GetBounds();
    SkCanvas* canvas = recorder.beginRecording(bounds.width(), bounds.height());
    canvas->translate(-bounds.x(), -bounds.y());
    canvas->clipRect(gfx::RectToSkRect(bounds));
    Raster(canvas, nullptr, gfx::Rect(), 1.f);
    sk_sp<SkPicture> picture = recorder.finishRecordingAsPicture();

    std::string b64_picture;
    PictureDebugUtil::SerializeAsBase64(picture.get(), &b64_picture);
    state->SetString("skp64", b64_picture);
  }

  return state;
}

void DisplayItemList::GenerateDiscardableImagesMetadata() {
  // This should be only called once.
  DCHECK(image_map_.empty());

  gfx::Rect bounds = rtree_.GetBounds();
  DiscardableImageMap::ScopedMetadataGenerator generator(
      &image_map_, gfx::Size(bounds.right(), bounds.bottom()));
  auto* canvas = generator.canvas();
  for (const auto& item : inputs_.items)
    item.Raster(canvas, nullptr);
}

void DisplayItemList::GetDiscardableImagesInRect(
    const gfx::Rect& rect,
    float contents_scale,
    std::vector<DrawImage>* images) {
  image_map_.GetDiscardableImagesInRect(rect, contents_scale, images);
}

gfx::Rect DisplayItemList::GetRectForImage(ImageId image_id) const {
  return image_map_.GetRectForImage(image_id);
}

}  // namespace cc
