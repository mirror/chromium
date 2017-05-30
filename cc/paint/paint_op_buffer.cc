// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_buffer.h"

#include "base/containers/stack_container.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_record.h"
#include "third_party/skia/include/core/SkAnnotation.h"

namespace cc {

#define TYPES(M)           \
  M(AnnotateOp)            \
  M(ClipPathOp)            \
  M(ClipRectOp)            \
  M(ClipRRectOp)           \
  M(ConcatOp)              \
  M(DrawArcOp)             \
  M(DrawCircleOp)          \
  M(DrawColorOp)           \
  M(DrawDisplayItemListOp) \
  M(DrawDRRectOp)          \
  M(DrawImageOp)           \
  M(DrawImageRectOp)       \
  M(DrawIRectOp)           \
  M(DrawLineOp)            \
  M(DrawOvalOp)            \
  M(DrawPathOp)            \
  M(DrawPosTextOp)         \
  M(DrawRecordOp)          \
  M(DrawRectOp)            \
  M(DrawRRectOp)           \
  M(DrawTextOp)            \
  M(DrawTextBlobOp)        \
  M(NoopOp)                \
  M(RestoreOp)             \
  M(RotateOp)              \
  M(SaveOp)                \
  M(SaveLayerOp)           \
  M(SaveLayerAlphaOp)      \
  M(ScaleOp)               \
  M(SetMatrixOp)           \
  M(TranslateOp)

using RasterFunction = void (*)(const PaintOp* op,
                                SkCanvas* canvas,
                                const SkMatrix& original_ctm);
using RasterWithFlagsFunction = void (*)(const PaintOpWithFlags* op,
                                         const PaintFlags* flags,
                                         SkCanvas* canvas,
                                         const SkMatrix& original_ctm);

NOINLINE static void RasterWithAlphaInternal(RasterFunction raster_fn,
                                             const PaintOp* op,
                                             SkCanvas* canvas,
                                             uint8_t alpha) {
  // TODO(enne): is it ok to just drop the bounds here?
  canvas->saveLayerAlpha(nullptr, alpha);
  SkMatrix unused_matrix;
  raster_fn(op, canvas, unused_matrix);
  canvas->restore();
}

// Helper template to share common code for RasterWithAlpha when paint ops
// have or don't have PaintFlags.
template <typename T, bool HasFlags>
struct Rasterizer {
  static void RasterWithAlpha(const T* op, SkCanvas* canvas, uint8_t alpha) {
    static_assert(
        !T::kHasPaintFlags,
        "This function should not be used for a PaintOp that has PaintFlags");
    DCHECK(T::kIsDrawOp);
    RasterWithAlphaInternal(&T::Raster, op, canvas, alpha);
  }
};

NOINLINE static void RasterWithAlphaInternalForFlags(
    RasterWithFlagsFunction raster_fn,
    const PaintOpWithFlags* op,
    SkCanvas* canvas,
    uint8_t alpha) {
  SkMatrix unused_matrix;
  if (alpha == 255) {
    raster_fn(op, &op->flags, canvas, unused_matrix);
  } else if (op->flags.SupportsFoldingAlpha()) {
    PaintFlags flags = op->flags;
    flags.setAlpha(SkMulDiv255Round(flags.getAlpha(), alpha));
    raster_fn(op, &flags, canvas, unused_matrix);
  } else {
    canvas->saveLayerAlpha(nullptr, alpha);
    raster_fn(op, &op->flags, canvas, unused_matrix);
    canvas->restore();
  }
}

template <typename T>
struct Rasterizer<T, true> {
  static void RasterWithAlpha(const T* op, SkCanvas* canvas, uint8_t alpha) {
    static_assert(T::kHasPaintFlags,
                  "This function expects the PaintOp to have PaintFlags");
    DCHECK(T::kIsDrawOp);
    RasterWithAlphaInternalForFlags(&T::RasterWithFlags, op, canvas, alpha);
  }
};

template <>
struct Rasterizer<DrawRecordOp, false> {
  static void RasterWithAlpha(const DrawRecordOp* op,
                              SkCanvas* canvas,
                              uint8_t alpha) {
    // This "looking into records" optimization is done here instead of
    // in the PaintOpBuffer::Raster function as DisplayItemList calls
    // into RasterWithAlpha directly.
    if (op->record->size() == 1u) {
      PaintOp* single_op = op->record->GetFirstOp();
      // RasterWithAlpha only supported for draw ops.
      if (single_op->IsDrawOp()) {
        single_op->RasterWithAlpha(canvas, alpha);
        return;
      }
    }

    canvas->saveLayerAlpha(nullptr, alpha);
    SkMatrix unused_matrix;
    DrawRecordOp::Raster(op, canvas, unused_matrix);
    canvas->restore();
  }
};

// TODO(enne): partially specialize RasterWithAlpha for draw color?

static constexpr size_t kNumOpTypes =
    static_cast<size_t>(PaintOpType::LastPaintOpType) + 1;

// Verify that every op is in the TYPES macro.
#define M(T) +1
static_assert(kNumOpTypes == TYPES(M), "Missing op in list");
#undef M

using SerializeFunction = size_t (*)(const PaintOp* op,
                                     void* memory,
                                     size_t size);
#define M(T) &T::Serialize,
static const SerializeFunction g_serialize_functions[kNumOpTypes] = {TYPES(M)};
#undef M

using DeserializeFunction = bool (*)(SkCanvas* canvas,
                                     const void* memory,
                                     size_t size,
                                     const SkMatrix& original_ctm);
#define M(T) &T::Deserialize,
static const DeserializeFunction g_deserialize_functions[kNumOpTypes] = {
    TYPES(M)};
#undef M

using RasterFunction = void (*)(const PaintOp* op,
                                SkCanvas* canvas,
                                const SkMatrix& original_ctm);
#define M(T) &T::Raster,
static const RasterFunction g_raster_functions[kNumOpTypes] = {TYPES(M)};
#undef M

using RasterAlphaFunction = void (*)(const PaintOp* op,
                                     SkCanvas* canvas,
                                     uint8_t alpha);
#define M(T) \
  T::kIsDrawOp ? \
  [](const PaintOp* op, SkCanvas* canvas, uint8_t alpha) { \
    Rasterizer<T, T::kHasPaintFlags>::RasterWithAlpha( \
        static_cast<const T*>(op), canvas, alpha); \
  } : static_cast<RasterAlphaFunction>(nullptr),
static const RasterAlphaFunction g_raster_alpha_functions[kNumOpTypes] = {
    TYPES(M)};
#undef M

// Most state ops (matrix, clip, save, restore) have a trivial destructor.
// TODO(enne): evaluate if we need the nullptr optimization or if
// we even need to differentiate trivial destructors here.
using VoidFunction = void (*)(PaintOp* op);
#define M(T)                                           \
  !std::is_trivially_destructible<T>::value            \
      ? [](PaintOp* op) { static_cast<T*>(op)->~T(); } \
      : static_cast<VoidFunction>(nullptr),
static const VoidFunction g_destructor_functions[kNumOpTypes] = {TYPES(M)};
#undef M

#define M(T) T::kIsDrawOp,
static bool g_is_draw_op[kNumOpTypes] = {TYPES(M)};
#undef M

#define M(T)                                         \
  static_assert(sizeof(T) <= sizeof(LargestPaintOp), \
                #T " must be no bigger than LargestPaintOp");
TYPES(M);
#undef M

#define M(T)                                               \
  static_assert(ALIGNOF(T) <= PaintOpBuffer::PaintOpAlign, \
                #T " must have alignment no bigger than PaintOpAlign");
TYPES(M);
#undef M

#undef TYPES

SkRect PaintOp::kUnsetRect = {SK_ScalarInfinity, 0, 0, 0};

template <typename T>
size_t SimpleSerialize(const PaintOp* op, void* memory, size_t size) {
  if (sizeof(T) > size)
    return 0;
  memcpy(memory, op, sizeof(T));
  return sizeof(T);
}

class PaintOpWriter {
 public:
  PaintOpWriter(void* memory, size_t size)
      : memory_(static_cast<char*>(memory)),
        size_(size),
        remaining_bytes_(size) {
    // Leave space for header of type/skip.
    size_t header = 4;
    DCHECK_GE(remaining_bytes_, header);

    memory_ += header;
    remaining_bytes_ -= header;
  }

  // TODO(enne): move this to another file, only expose valid types
  // into the header.
  template <typename T>
  void Write(const T& val) {
    if (!valid_)
      return;

    // TODO(enne): write this.  Most types need just a memcpy
    // skdata, skpaint, skpath need some special love.

    size_t written = 0u;
    memory_ += written;
    remaining_bytes_ -= written;
  }

  void WriteData(size_t bytes, const void* input) {
    Write(bytes);

    if (bytes > remaining_bytes_)
      valid_ = false;
    if (!valid_)
      return;

    memcpy(memory_, input, bytes);
    memory_ += bytes;
    remaining_bytes_ -= bytes;
  }

  size_t size() const { return valid_ ? size_ - remaining_bytes_ : 0u; }

 private:
  char* memory_;
  size_t size_;
  size_t remaining_bytes_;
  bool valid_ = true;
};

class PaintOpReader {
 public:
  PaintOpReader(const void* memory, size_t size)
      : memory_(static_cast<const char*>(memory)), remaining_bytes_(size) {
    // Skip past header.
    size_t header = 4;
    DCHECK_GE(remaining_bytes_, header);

    memory_ += header;
    remaining_bytes_ -= header;
  }

  template <typename T>
  void Read(T* val) {
    if (!valid_)
      return;

    // TODO(enne): write this.  Most types need just a memcpy
    // skdata, skpaint, skpath need some special love.

    size_t read_bytes = 0u;
    memory_ += read_bytes;
    remaining_bytes_ -= read_bytes;
  }

  void ReadData(std::vector<char>* data) {
    size_t bytes;
    Read(&bytes);
    if (remaining_bytes_ < bytes)
      valid_ = false;
    if (!valid_)
      return;

    data->resize(bytes);
    memcpy(&(*data)[0], memory_, bytes);
    memory_ += bytes;
    remaining_bytes_ -= bytes;
  }

  bool valid() const { return valid_; }

 private:
  const char* memory_;
  size_t remaining_bytes_;
  bool valid_ = true;
};

size_t AnnotateOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const AnnotateOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->annotation_type);
  helper.Write(op->rect);
  helper.Write(op->data);
  return helper.size();
}

size_t ClipPathOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const ClipPathOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->path);
  helper.Write(op->op);
  helper.Write(op->antialias);
  return helper.size();
}

size_t ClipRectOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<ClipRectOp>(op, memory, size);
}

size_t ClipRRectOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<ClipRRectOp>(op, memory, size);
}

size_t ConcatOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<ConcatOp>(op, memory, size);
}

size_t DrawArcOp::Serialize(const PaintOp* base_op, void* memory, size_t size) {
  auto* op = static_cast<const DrawArcOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->oval);
  helper.Write(op->start_angle);
  helper.Write(op->sweep_angle);
  helper.Write(op->use_center);
  return helper.size();
}

size_t DrawCircleOp::Serialize(const PaintOp* base_op,
                               void* memory,
                               size_t size) {
  auto* op = static_cast<const DrawCircleOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->cx);
  helper.Write(op->cy);
  helper.Write(op->radius);
  return helper.size();
}

size_t DrawColorOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<DrawColorOp>(op, memory, size);
}

size_t DrawDisplayItemListOp::Serialize(const PaintOp* op,
                                        void* memory,
                                        size_t size) {
  // TODO(enne): maybe flatten this here?
  return 4;
}

size_t DrawDRRectOp::Serialize(const PaintOp* base_op,
                               void* memory,
                               size_t size) {
  auto* op = static_cast<const DrawDRRectOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->outer);
  helper.Write(op->inner);
  return helper.size();
}

size_t DrawImageOp::Serialize(const PaintOp* base_op,
                              void* memory,
                              size_t size) {
  auto* op = static_cast<const DrawImageOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->image);
  helper.Write(op->left);
  helper.Write(op->top);
  return helper.size();
}

size_t DrawImageRectOp::Serialize(const PaintOp* base_op,
                                  void* memory,
                                  size_t size) {
  auto* op = static_cast<const DrawImageRectOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->image);
  helper.Write(op->src);
  helper.Write(op->dst);
  helper.Write(op->constraint);
  return helper.size();
}

size_t DrawIRectOp::Serialize(const PaintOp* base_op,
                              void* memory,
                              size_t size) {
  auto* op = static_cast<const DrawIRectOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->rect);
  return helper.size();
}

size_t DrawLineOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const DrawLineOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->x0);
  helper.Write(op->y0);
  helper.Write(op->x1);
  helper.Write(op->y1);
  return helper.size();
}

size_t DrawOvalOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const DrawOvalOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->oval);
  return helper.size();
}

size_t DrawPathOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const DrawPathOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->path);
  return helper.size();
}

size_t DrawPosTextOp::Serialize(const PaintOp* base_op,
                                void* memory,
                                size_t size) {
  auto* op = static_cast<const DrawPosTextOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.WriteData(op->bytes, op->GetData());
  helper.WriteData(op->count * sizeof(decltype(op->GetArray())),
                   op->GetArray());
  return helper.size();
}

size_t DrawRecordOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  // TODO(enne): maybe flatten this here?
  return 4;
}

size_t DrawRectOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const DrawRectOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->rect);
  return helper.size();
}

size_t DrawRRectOp::Serialize(const PaintOp* base_op,
                              void* memory,
                              size_t size) {
  auto* op = static_cast<const DrawRRectOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->rrect);
  return helper.size();
}

size_t DrawTextOp::Serialize(const PaintOp* base_op,
                             void* memory,
                             size_t size) {
  auto* op = static_cast<const DrawTextOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->x);
  helper.Write(op->y);
  helper.WriteData(op->bytes, op->GetData());
  return helper.size();
}

size_t DrawTextBlobOp::Serialize(const PaintOp* base_op,
                                 void* memory,
                                 size_t size) {
  auto* op = static_cast<const DrawTextBlobOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->blob);
  helper.Write(op->x);
  helper.Write(op->y);
  return helper.size();
}

size_t NoopOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<NoopOp>(op, memory, size);
}

size_t RestoreOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<RestoreOp>(op, memory, size);
}

size_t RotateOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<RotateOp>(op, memory, size);
}

size_t SaveOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<SaveOp>(op, memory, size);
}

size_t SaveLayerOp::Serialize(const PaintOp* base_op,
                              void* memory,
                              size_t size) {
  auto* op = static_cast<const SaveLayerOp*>(base_op);
  PaintOpWriter helper(memory, size);
  helper.Write(op->flags);
  helper.Write(op->bounds);
  return helper.size();
}

size_t SaveLayerAlphaOp::Serialize(const PaintOp* op,
                                   void* memory,
                                   size_t size) {
  return SimpleSerialize<SaveLayerOp>(op, memory, size);
}

size_t ScaleOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<ScaleOp>(op, memory, size);
}

size_t SetMatrixOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<SetMatrixOp>(op, memory, size);
}

size_t TranslateOp::Serialize(const PaintOp* op, void* memory, size_t size) {
  return SimpleSerialize<TranslateOp>(op, memory, size);
}

template <typename T>
bool SimpleDeserialize(SkCanvas* canvas,
                       const void* memory,
                       size_t size,
                       const SkMatrix& original_ctm) {
  if (sizeof(T) > size)
    return false;

  const T* op = reinterpret_cast<const T*>(memory);
  T::Raster(op, canvas, original_ctm);
  return true;
}

bool AnnotateOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  AnnotateOp op;

  helper.Read(&op.annotation_type);
  helper.Read(&op.rect);
  helper.Read(&op.data);

  if (!helper.valid())
    return false;

  AnnotateOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool ClipPathOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  return SimpleDeserialize<ClipPathOp>(canvas, memory, size, original_ctm);
}

bool ClipRectOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  return SimpleDeserialize<ClipRectOp>(canvas, memory, size, original_ctm);
}

bool ClipRRectOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  return SimpleDeserialize<ClipRRectOp>(canvas, memory, size, original_ctm);
}

bool ConcatOp::Deserialize(SkCanvas* canvas,
                           const void* memory,
                           size_t size,
                           const SkMatrix& original_ctm) {
  return SimpleDeserialize<ConcatOp>(canvas, memory, size, original_ctm);
}

bool DrawArcOp::Deserialize(SkCanvas* canvas,
                            const void* memory,
                            size_t size,
                            const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawArcOp op;
  helper.Read(&op.flags);
  helper.Read(&op.oval);
  helper.Read(&op.start_angle);
  helper.Read(&op.sweep_angle);
  helper.Read(&op.use_center);

  if (!helper.valid())
    return false;

  DrawArcOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawCircleOp::Deserialize(SkCanvas* canvas,
                               const void* memory,
                               size_t size,
                               const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawCircleOp op;
  helper.Read(&op.flags);
  helper.Read(&op.cx);
  helper.Read(&op.cy);
  helper.Read(&op.radius);

  if (!helper.valid())
    return false;

  DrawCircleOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawColorOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  return SimpleDeserialize<DrawColorOp>(canvas, memory, size, original_ctm);
}

bool DrawDisplayItemListOp::Deserialize(SkCanvas* canvas,
                                        const void* memory,
                                        size_t size,
                                        const SkMatrix& original_ctm) {
  // TODO(enne): see comments in serialize.  Should this get flattened
  // and never deserialized?
  return true;
}

bool DrawDRRectOp::Deserialize(SkCanvas* canvas,
                               const void* memory,
                               size_t size,
                               const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawDRRectOp op;
  helper.Read(&op.flags);
  helper.Read(&op.outer);
  helper.Read(&op.inner);

  if (!helper.valid())
    return false;

  DrawDRRectOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawImageOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawImageOp op;
  helper.Read(&op.flags);
  helper.Read(&op.image);
  helper.Read(&op.left);
  helper.Read(&op.top);

  if (!helper.valid())
    return false;

  DrawImageOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawImageRectOp::Deserialize(SkCanvas* canvas,
                                  const void* memory,
                                  size_t size,
                                  const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawImageRectOp op;
  helper.Read(&op.flags);
  helper.Read(&op.image);
  helper.Read(&op.src);
  helper.Read(&op.dst);
  helper.Read(&op.constraint);

  if (!helper.valid())
    return false;

  DrawImageRectOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawIRectOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawIRectOp op;
  helper.Read(&op.flags);
  helper.Read(&op.rect);

  if (!helper.valid())
    return false;

  DrawIRectOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawLineOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawLineOp op;
  helper.Read(&op.flags);
  helper.Read(&op.x0);
  helper.Read(&op.y0);
  helper.Read(&op.x1);
  helper.Read(&op.y1);

  if (!helper.valid())
    return false;

  DrawLineOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawOvalOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawOvalOp op;
  helper.Read(&op.flags);
  helper.Read(&op.oval);

  if (!helper.valid())
    return false;

  DrawOvalOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawPathOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawPathOp op;
  helper.Read(&op.flags);
  helper.Read(&op.path);

  if (!helper.valid())
    return false;

  DrawPathOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawPosTextOp::Deserialize(SkCanvas* canvas,
                                const void* memory,
                                size_t size,
                                const SkMatrix& original_ctm) {
  // TODO(enne): maybe we don't need to copy this data.
  PaintFlags flags;

  std::vector<char> data;
  // TODO(enne): array needs to be aligned OOPS fix this
  std::vector<char> array;

  PaintOpReader helper(memory, size);
  helper.Read(&flags);
  helper.ReadData(&data);
  helper.ReadData(&array);

  if (!helper.valid())
    return false;

  canvas->drawPosText(&data[0], data.size(),
                      reinterpret_cast<SkPoint*>(&array[0]), ToSkPaint(flags));
  return true;
}

bool DrawRecordOp::Deserialize(SkCanvas* canvas,
                               const void* memory,
                               size_t size,
                               const SkMatrix& original_ctm) {
  // TODO(enne): see comments in draw display item list op etc
  return true;
}

bool DrawRectOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawRectOp op;
  helper.Read(&op.flags);
  helper.Read(&op.rect);

  if (!helper.valid())
    return false;

  DrawRectOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawRRectOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawRRectOp op;
  helper.Read(&op.flags);
  helper.Read(&op.rrect);

  if (!helper.valid())
    return false;

  DrawRRectOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool DrawTextOp::Deserialize(SkCanvas* canvas,
                             const void* memory,
                             size_t size,
                             const SkMatrix& original_ctm) {
  // TODO(enne): maybe we don't need to copy this data.
  PaintFlags flags;
  SkScalar x;
  SkScalar y;

  std::vector<char> data;

  PaintOpReader helper(memory, size);
  helper.Read(&flags);
  helper.Read(&x);
  helper.Read(&y);
  helper.ReadData(&data);

  if (!helper.valid())
    return false;

  canvas->drawText(&data[0], data.size(), x, y, ToSkPaint(flags));
  return true;
}

bool DrawTextBlobOp::Deserialize(SkCanvas* canvas,
                                 const void* memory,
                                 size_t size,
                                 const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  DrawTextBlobOp op;
  helper.Read(&op.flags);
  helper.Read(&op.blob);
  helper.Read(&op.x);
  helper.Read(&op.y);

  if (!helper.valid())
    return false;

  DrawTextBlobOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool NoopOp::Deserialize(SkCanvas* canvas,
                         const void* memory,
                         size_t size,
                         const SkMatrix& original_ctm) {
  return SimpleDeserialize<NoopOp>(canvas, memory, size, original_ctm);
}

bool RestoreOp::Deserialize(SkCanvas* canvas,
                            const void* memory,
                            size_t size,
                            const SkMatrix& original_ctm) {
  return SimpleDeserialize<RestoreOp>(canvas, memory, size, original_ctm);
}

bool RotateOp::Deserialize(SkCanvas* canvas,
                           const void* memory,
                           size_t size,
                           const SkMatrix& original_ctm) {
  return SimpleDeserialize<RotateOp>(canvas, memory, size, original_ctm);
}

bool SaveOp::Deserialize(SkCanvas* canvas,
                         const void* memory,
                         size_t size,
                         const SkMatrix& original_ctm) {
  return SimpleDeserialize<SaveOp>(canvas, memory, size, original_ctm);
}

bool SaveLayerOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  PaintOpReader helper(memory, size);
  SaveLayerOp op;
  helper.Read(&op.flags);
  helper.Read(&op.bounds);

  if (!helper.valid())
    return false;

  SaveLayerOp::Raster(&op, canvas, original_ctm);
  return true;
}

bool SaveLayerAlphaOp::Deserialize(SkCanvas* canvas,
                                   const void* memory,
                                   size_t size,
                                   const SkMatrix& original_ctm) {
  return SimpleDeserialize<SaveLayerAlphaOp>(canvas, memory, size,
                                             original_ctm);
}

bool ScaleOp::Deserialize(SkCanvas* canvas,
                          const void* memory,
                          size_t size,
                          const SkMatrix& original_ctm) {
  return SimpleDeserialize<ScaleOp>(canvas, memory, size, original_ctm);
}

bool SetMatrixOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  // TODO(enne): this probably isn't valid in a toctou sense, and we
  // probably need to make a local copy of the matrix.
  return SimpleDeserialize<SetMatrixOp>(canvas, memory, size, original_ctm);
}

bool TranslateOp::Deserialize(SkCanvas* canvas,
                              const void* memory,
                              size_t size,
                              const SkMatrix& original_ctm) {
  return SimpleDeserialize<TranslateOp>(canvas, memory, size, original_ctm);
}

void AnnotateOp::Raster(const PaintOp* base_op,
                        SkCanvas* canvas,
                        const SkMatrix& original_ctm) {
  auto* op = static_cast<const AnnotateOp*>(base_op);
  switch (op->annotation_type) {
    case PaintCanvas::AnnotationType::URL:
      SkAnnotateRectWithURL(canvas, op->rect, op->data.get());
      break;
    case PaintCanvas::AnnotationType::LINK_TO_DESTINATION:
      SkAnnotateLinkToDestination(canvas, op->rect, op->data.get());
      break;
    case PaintCanvas::AnnotationType::NAMED_DESTINATION: {
      SkPoint point = SkPoint::Make(op->rect.x(), op->rect.y());
      SkAnnotateNamedDestination(canvas, point, op->data.get());
      break;
    }
  }
}

void ClipPathOp::Raster(const PaintOp* base_op,
                        SkCanvas* canvas,
                        const SkMatrix& original_ctm) {
  auto* op = static_cast<const ClipPathOp*>(base_op);
  canvas->clipPath(op->path, op->op, op->antialias);
}

void ClipRectOp::Raster(const PaintOp* base_op,
                        SkCanvas* canvas,
                        const SkMatrix& original_ctm) {
  auto* op = static_cast<const ClipRectOp*>(base_op);
  canvas->clipRect(op->rect, op->op, op->antialias);
}

void ClipRRectOp::Raster(const PaintOp* base_op,
                         SkCanvas* canvas,
                         const SkMatrix& original_ctm) {
  auto* op = static_cast<const ClipRRectOp*>(base_op);
  canvas->clipRRect(op->rrect, op->op, op->antialias);
}

void ConcatOp::Raster(const PaintOp* base_op,
                      SkCanvas* canvas,
                      const SkMatrix& original_ctm) {
  auto* op = static_cast<const ConcatOp*>(base_op);
  canvas->concat(op->matrix);
}

void DrawArcOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                const PaintFlags* flags,
                                SkCanvas* canvas,
                                const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawArcOp*>(base_op);
  canvas->drawArc(op->oval, op->start_angle, op->sweep_angle, op->use_center,
                  ToSkPaint(*flags));
}

void DrawCircleOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                   const PaintFlags* flags,
                                   SkCanvas* canvas,
                                   const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawCircleOp*>(base_op);
  canvas->drawCircle(op->cx, op->cy, op->radius, ToSkPaint(*flags));
}

void DrawColorOp::Raster(const PaintOp* base_op,
                         SkCanvas* canvas,
                         const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawColorOp*>(base_op);
  canvas->drawColor(op->color, op->mode);
}

void DrawDisplayItemListOp::Raster(const PaintOp* base_op,
                                   SkCanvas* canvas,
                                   const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawDisplayItemListOp*>(base_op);
  op->list->Raster(canvas);
}

void DrawDRRectOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                   const PaintFlags* flags,
                                   SkCanvas* canvas,
                                   const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawDRRectOp*>(base_op);
  canvas->drawDRRect(op->outer, op->inner, ToSkPaint(*flags));
}

void DrawImageOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawImageOp*>(base_op);
  canvas->drawImage(op->image.sk_image().get(), op->left, op->top,
                    ToSkPaint(flags));
}

void DrawImageRectOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                      const PaintFlags* flags,
                                      SkCanvas* canvas,
                                      const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawImageRectOp*>(base_op);
  // TODO(enne): Probably PaintCanvas should just use the skia enum directly.
  SkCanvas::SrcRectConstraint skconstraint =
      static_cast<SkCanvas::SrcRectConstraint>(op->constraint);
  canvas->drawImageRect(op->image.sk_image().get(), op->src, op->dst,
                        ToSkPaint(flags), skconstraint);
}

void DrawIRectOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawIRectOp*>(base_op);
  canvas->drawIRect(op->rect, ToSkPaint(*flags));
}

void DrawLineOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawLineOp*>(base_op);
  canvas->drawLine(op->x0, op->y0, op->x1, op->y1, ToSkPaint(*flags));
}

void DrawOvalOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawOvalOp*>(base_op);
  canvas->drawOval(op->oval, ToSkPaint(*flags));
}

void DrawPathOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawPathOp*>(base_op);
  canvas->drawPath(op->path, ToSkPaint(*flags));
}

void DrawPosTextOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                    const PaintFlags* flags,
                                    SkCanvas* canvas,
                                    const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawPosTextOp*>(base_op);
  canvas->drawPosText(op->GetData(), op->bytes, op->GetArray(),
                      ToSkPaint(*flags));
}

void DrawRecordOp::Raster(const PaintOp* base_op,
                          SkCanvas* canvas,
                          const SkMatrix& original_ctm) {
  // Don't use drawPicture here, as it adds an implicit clip.
  auto* op = static_cast<const DrawRecordOp*>(base_op);
  op->record->playback(canvas);
}

void DrawRectOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawRectOp*>(base_op);
  canvas->drawRect(op->rect, ToSkPaint(*flags));
}

void DrawRRectOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawRRectOp*>(base_op);
  canvas->drawRRect(op->rrect, ToSkPaint(*flags));
}

void DrawTextOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                 const PaintFlags* flags,
                                 SkCanvas* canvas,
                                 const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawTextOp*>(base_op);
  canvas->drawText(op->GetData(), op->bytes, op->x, op->y, ToSkPaint(*flags));
}

void DrawTextBlobOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                     const PaintFlags* flags,
                                     SkCanvas* canvas,
                                     const SkMatrix& original_ctm) {
  auto* op = static_cast<const DrawTextBlobOp*>(base_op);
  canvas->drawTextBlob(op->blob.get(), op->x, op->y, ToSkPaint(*flags));
}

void RestoreOp::Raster(const PaintOp* base_op,
                       SkCanvas* canvas,
                       const SkMatrix& original_ctm) {
  canvas->restore();
}

void RotateOp::Raster(const PaintOp* base_op,
                      SkCanvas* canvas,
                      const SkMatrix& original_ctm) {
  auto* op = static_cast<const RotateOp*>(base_op);
  canvas->rotate(op->degrees);
}

void SaveOp::Raster(const PaintOp* base_op,
                    SkCanvas* canvas,
                    const SkMatrix& original_ctm) {
  canvas->save();
}

void SaveLayerOp::RasterWithFlags(const PaintOpWithFlags* base_op,
                                  const PaintFlags* flags,
                                  SkCanvas* canvas,
                                  const SkMatrix& original_ctm) {
  auto* op = static_cast<const SaveLayerOp*>(base_op);
  // See PaintOp::kUnsetRect
  bool unset = op->bounds.left() == SK_ScalarInfinity;

  canvas->saveLayer(unset ? nullptr : &op->bounds, ToSkPaint(flags));
}

void SaveLayerAlphaOp::Raster(const PaintOp* base_op,
                              SkCanvas* canvas,
                              const SkMatrix& original_ctm) {
  auto* op = static_cast<const SaveLayerAlphaOp*>(base_op);
  // See PaintOp::kUnsetRect
  bool unset = op->bounds.left() == SK_ScalarInfinity;
  canvas->saveLayerAlpha(unset ? nullptr : &op->bounds, op->alpha);
}

void ScaleOp::Raster(const PaintOp* base_op,
                     SkCanvas* canvas,
                     const SkMatrix& original_ctm) {
  auto* op = static_cast<const ScaleOp*>(base_op);
  canvas->scale(op->sx, op->sy);
}

void SetMatrixOp::Raster(const PaintOp* base_op,
                         SkCanvas* canvas,
                         const SkMatrix& original_ctm) {
  auto* op = static_cast<const SetMatrixOp*>(base_op);
  canvas->setMatrix(SkMatrix::Concat(original_ctm, op->matrix));
}

void TranslateOp::Raster(const PaintOp* base_op,
                         SkCanvas* canvas,
                         const SkMatrix& original_ctm) {
  auto* op = static_cast<const TranslateOp*>(base_op);
  canvas->translate(op->dx, op->dy);
}

bool PaintOp::IsDrawOp() const {
  return g_is_draw_op[type];
}

void PaintOp::Raster(SkCanvas* canvas, const SkMatrix& original_ctm) const {
  g_raster_functions[type](this, canvas, original_ctm);
}

void PaintOp::RasterWithAlpha(SkCanvas* canvas, uint8_t alpha) const {
  g_raster_alpha_functions[type](this, canvas, alpha);
}

size_t PaintOp::Serialize(void* memory, size_t size) const {
  // Need at least enough room for a header.
  if (size < 4)
    return 0;

  // TODO(enne): dcheck that memory is aligned

  size_t written = g_serialize_functions[type](this, memory, size);
  if (!written)
    return 0u;

  size_t skip =
      MathUtil::UncheckedRoundUp(written, PaintOpBuffer::PaintOpAlign);
  DCHECK_LE(written, size);
  DCHECK_LT(skip, static_cast<size_t>(1) << 24);
  // If rounding up overflows size, then skip exactly to the end.
  // The op itself will still fit, but the next op wouldn't be aligned.
  skip = std::min(size, skip);

  // Update skip and type.
  uint32_t type_and_skip = type | skip << 8;
  static_cast<uint32_t*>(memory)[0] = type_and_skip;

  return skip;
}

int ClipPathOp::CountSlowPaths() const {
  return antialias && !path.isConvex() ? 1 : 0;
}

int DrawLineOp::CountSlowPaths() const {
  if (const SkPathEffect* effect = flags.getPathEffect()) {
    SkPathEffect::DashInfo info;
    SkPathEffect::DashType dashType = effect->asADash(&info);
    if (flags.getStrokeCap() != PaintFlags::kRound_Cap &&
        dashType == SkPathEffect::kDash_DashType && info.fCount == 2) {
      // The PaintFlags will count this as 1, so uncount that here as
      // this kind of line is special cased and not slow.
      return -1;
    }
  }
  return 0;
}

int DrawPathOp::CountSlowPaths() const {
  // This logic is copied from SkPathCounter instead of attempting to expose
  // that from Skia.
  if (!flags.isAntiAlias() || path.isConvex())
    return 0;

  PaintFlags::Style paintStyle = flags.getStyle();
  const SkRect& pathBounds = path.getBounds();
  if (paintStyle == PaintFlags::kStroke_Style && flags.getStrokeWidth() == 0) {
    // AA hairline concave path is not slow.
    return 0;
  } else if (paintStyle == PaintFlags::kFill_Style &&
             pathBounds.width() < 64.f && pathBounds.height() < 64.f &&
             !path.isVolatile()) {
    // AADF eligible concave path is not slow.
    return 0;
  } else {
    return 1;
  }
}

int DrawRecordOp::CountSlowPaths() const {
  return record->numSlowPaths();
}

AnnotateOp::AnnotateOp() = default;

AnnotateOp::AnnotateOp(PaintCanvas::AnnotationType annotation_type,
                       const SkRect& rect,
                       sk_sp<SkData> data)
    : annotation_type(annotation_type), rect(rect), data(std::move(data)) {}

AnnotateOp::~AnnotateOp() = default;

DrawDisplayItemListOp::DrawDisplayItemListOp(
    scoped_refptr<DisplayItemList> list)
    : list(list) {}

size_t DrawDisplayItemListOp::AdditionalBytesUsed() const {
  return list->ApproximateMemoryUsage();
}

bool DrawDisplayItemListOp::HasDiscardableImages() const {
  return list->has_discardable_images();
}

DrawDisplayItemListOp::DrawDisplayItemListOp(const DrawDisplayItemListOp& op) =
    default;

DrawDisplayItemListOp& DrawDisplayItemListOp::operator=(
    const DrawDisplayItemListOp& op) = default;

DrawDisplayItemListOp::~DrawDisplayItemListOp() = default;

DrawImageOp::DrawImageOp(const PaintImage& image,
                         SkScalar left,
                         SkScalar top,
                         const PaintFlags* flags)
    : PaintOpWithFlags(flags ? *flags : PaintFlags()),
      image(image),
      left(left),
      top(top) {}

bool DrawImageOp::HasDiscardableImages() const {
  // TODO(khushalsagar): Callers should not be able to change the lazy generated
  // state for a PaintImage.
  return image.sk_image()->isLazyGenerated();
}

DrawImageOp::~DrawImageOp() = default;

DrawImageRectOp::DrawImageRectOp() = default;

DrawImageRectOp::DrawImageRectOp(const PaintImage& image,
                                 const SkRect& src,
                                 const SkRect& dst,
                                 const PaintFlags* flags,
                                 PaintCanvas::SrcRectConstraint constraint)
    : PaintOpWithFlags(flags ? *flags : PaintFlags()),
      image(image),
      src(src),
      dst(dst),
      constraint(constraint) {}

bool DrawImageRectOp::HasDiscardableImages() const {
  return image.sk_image()->isLazyGenerated();
}

DrawImageRectOp::~DrawImageRectOp() = default;

DrawPosTextOp::DrawPosTextOp(size_t bytes,
                             size_t count,
                             const PaintFlags& flags)
    : PaintOpWithArray(flags, bytes, count) {}

DrawPosTextOp::~DrawPosTextOp() = default;

DrawRecordOp::DrawRecordOp(sk_sp<const PaintRecord> record)
    : record(std::move(record)) {}

DrawRecordOp::~DrawRecordOp() = default;

size_t DrawRecordOp::AdditionalBytesUsed() const {
  return record->bytes_used();
}

bool DrawRecordOp::HasDiscardableImages() const {
  return record->HasDiscardableImages();
}

DrawTextBlobOp::DrawTextBlobOp() = default;

DrawTextBlobOp::DrawTextBlobOp(sk_sp<SkTextBlob> blob,
                               SkScalar x,
                               SkScalar y,
                               const PaintFlags& flags)
    : PaintOpWithFlags(flags), blob(std::move(blob)), x(x), y(y) {}

DrawTextBlobOp::~DrawTextBlobOp() = default;

PaintOpBuffer::PaintOpBuffer() = default;

PaintOpBuffer::~PaintOpBuffer() {
  Reset();
}

void PaintOpBuffer::Reset() {
  for (auto* op : Iterator(this)) {
    auto func = g_destructor_functions[op->type];
    if (func)
      func(op);
  }

  // Leave data_ allocated, reserved_ unchanged.
  used_ = 0;
  op_count_ = 0;
  num_slow_paths_ = 0;
}

static const PaintOp* NextOp(const std::vector<size_t>& range_starts,
                             const std::vector<size_t>& range_indices,
                             base::StackVector<const PaintOp*, 3>* stack_ptr,
                             PaintOpBuffer::Iterator* iter,
                             size_t* range_index) {
  auto& stack = *stack_ptr;
  if (stack->size()) {
    const PaintOp* op = stack->front();
    // Shift paintops forward
    stack->erase(stack->begin());
    return op;
  }
  if (!*iter)
    return nullptr;

  // This grabs the PaintOp from the current iterator position, and advances it
  // to the next position immediately. We'll see we reached the end of the
  // buffer on the next call to this method.
  const PaintOp* op = **iter;

  size_t active_range = range_indices[*range_index];
  DCHECK_GE(iter->op_idx(), range_starts[active_range]);

  if (active_range + 1 == range_starts.size()) {
    // In the last possible range, so go right to the end of the buffer.
    ++*iter;
  } else {
    size_t range_end = range_starts[active_range + 1];
    DCHECK_LE(iter->op_idx(), range_end);

    ++*iter;
    if (iter->op_idx() == range_end) {
      if (*range_index + 1 == range_indices.size()) {
        // Leaving the last range that we want to iterate.
        *iter = iter->end();
      } else {
        // Move to the next range.
        ++(*range_index);
        size_t next_range_start = range_starts[range_indices[*range_index]];
        while (iter->op_idx() < next_range_start)
          ++(*iter);
      }
    }
  }

  return op;
}

bool PaintOpBuffer::Deserialize(SkCanvas* canvas,
                                const void* memory,
                                size_t size,
                                const SkMatrix& original_ctm) const {
  // TODO(enne): need to handle raster optimizations here, like playback.
  // possibly by saving off the ops.  Looking ahead seems to be a TOCTOU
  // violation though, so need to be smarter about it than what playback does.

  size_t remaining_bytes = size;
  const char* ptr = static_cast<const char*>(memory);

  while (remaining_bytes) {
    // TODO(enne): TOCTOU sketchiness, but PaintOp derived types don't ever
    // ask about their type or skip.
    uint32_t type_and_skip = static_cast<const uint32_t*>(memory)[0];
    uint8_t type = static_cast<uint8_t>(type_and_skip);
    uint32_t skip = type_and_skip >> 8;

    // Verify skip doesn't run off the buffer.
    if (skip > remaining_bytes)
      return false;
    // skip must always be aligned to paint ops unless skipping to the end.
    if (skip % PaintOpAlign != 0 || skip == remaining_bytes)
      return false;

    if (!g_deserialize_functions[type](canvas, ptr, skip, original_ctm))
      return false;

    ptr += skip;
    remaining_bytes -= skip;
  }

  return true;
}

void PaintOpBuffer::playback(SkCanvas* canvas,
                             SkPicture::AbortCallback* callback) const {
  // Treats the entire PaintOpBuffer as a single range.
  PlaybackRanges({0}, {0}, canvas, callback);
}

void PaintOpBuffer::PlaybackRanges(const std::vector<size_t>& range_starts,
                                   const std::vector<size_t>& range_indices,
                                   SkCanvas* canvas,
                                   SkPicture::AbortCallback* callback) const {
  if (!op_count_)
    return;
  if (callback && callback->abort())
    return;

#if DCHECK_IS_ON()
  DCHECK(!range_starts.empty());   // Don't call this then.
  DCHECK(!range_indices.empty());  // Don't call this then.
  DCHECK_EQ(0u, range_starts[0]);
  for (size_t i = 1; i < range_starts.size(); ++i) {
    DCHECK_GT(range_starts[i], range_starts[i - 1]);
    DCHECK_LT(range_starts[i], op_count_);
  }
  DCHECK_LT(range_indices[0], range_starts.size());
  for (size_t i = 1; i < range_indices.size(); ++i) {
    DCHECK_GT(range_indices[i], range_indices[i - 1]);
    DCHECK_LT(range_indices[i], range_starts.size());
  }
#endif

  // TODO(enne): a PaintRecord that contains a SetMatrix assumes that the
  // SetMatrix is local to that PaintRecord itself.  Said differently, if you
  // translate(x, y), then draw a paint record with a SetMatrix(identity),
  // the translation should be preserved instead of clobbering the top level
  // transform.  This could probably be done more efficiently.
  SkMatrix original = canvas->getTotalMatrix();

  // FIFO queue of paint ops that have been peeked at.
  base::StackVector<const PaintOp*, 3> stack;

  // The current offset into range_indices. range_indices[range_index] is the
  // current offset into range_starts.
  size_t range_index = 0;

  Iterator iter(this);
  while (iter.op_idx() < range_starts[range_indices[range_index]])
    ++iter;
  while (const PaintOp* op =
             NextOp(range_starts, range_indices, &stack, &iter, &range_index)) {
    // Optimize out save/restores or save/draw/restore that can be a single
    // draw.  See also: similar code in SkRecordOpts and cc's DisplayItemList.
    // TODO(enne): consider making this recursive?
    if (op->GetType() == PaintOpType::SaveLayerAlpha) {
      const PaintOp* second =
          NextOp(range_starts, range_indices, &stack, &iter, &range_index);
      const PaintOp* third = nullptr;
      if (second) {
        if (second->GetType() == PaintOpType::Restore) {
          continue;
        }
        if (second->IsDrawOp()) {
          third =
              NextOp(range_starts, range_indices, &stack, &iter, &range_index);
          if (third && third->GetType() == PaintOpType::Restore) {
            const SaveLayerAlphaOp* save_op =
                static_cast<const SaveLayerAlphaOp*>(op);
            second->RasterWithAlpha(canvas, save_op->alpha);
            continue;
          }
        }

        // Store deferred ops for later.
        stack->push_back(second);
        if (third)
          stack->push_back(third);
      }
    }
    // TODO(enne): skip SaveLayer followed by restore with nothing in
    // between, however SaveLayer with image filters on it (or maybe
    // other PaintFlags options) are not a noop.  Figure out what these
    // are so we can skip them correctly.

    op->Raster(canvas, original);
    if (callback && callback->abort())
      return;
  }
}

void PaintOpBuffer::ReallocBuffer(size_t new_size) {
  DCHECK_GE(new_size, used_);
  std::unique_ptr<char, base::AlignedFreeDeleter> new_data(
      static_cast<char*>(base::AlignedAlloc(new_size, PaintOpAlign)));
  memcpy(new_data.get(), data_.get(), used_);
  data_ = std::move(new_data);
  reserved_ = new_size;
}

std::pair<void*, size_t> PaintOpBuffer::AllocatePaintOp(size_t sizeof_op,
                                                        size_t bytes) {
  if (!op_count_) {
    if (bytes) {
      // Internal first_op buffer doesn't have room for extra data.
      // If the op wants extra bytes, then we'll just store a Noop
      // in the first_op and proceed from there.  This seems unlikely
      // to be a common case.
      push<NoopOp>();
    } else {
      op_count_++;
      return std::make_pair(first_op_.void_data(), 0);
    }
  }

  // We've filled |first_op_| by now so we need to allocate space in |data_|.
  DCHECK(op_count_);

  // Compute a skip such that all ops in the buffer are aligned to the
  // maximum required alignment of all ops.
  size_t skip = MathUtil::UncheckedRoundUp(sizeof_op + bytes, PaintOpAlign);
  DCHECK_LT(skip, static_cast<size_t>(1) << 24);
  if (used_ + skip > reserved_) {
    // Start reserved_ at kInitialBufferSize and then double.
    // ShrinkToFit can make this smaller afterwards.
    size_t new_size = reserved_ ? reserved_ : kInitialBufferSize;
    while (used_ + skip > new_size)
      new_size *= 2;
    ReallocBuffer(new_size);
  }
  DCHECK_LE(used_ + skip, reserved_);

  void* op = data_.get() + used_;
  used_ += skip;
  op_count_++;
  return std::make_pair(op, skip);
}

void PaintOpBuffer::ShrinkToFit() {
  if (!used_ || used_ == reserved_)
    return;
  ReallocBuffer(used_);
}

}  // namespace cc
