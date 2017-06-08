// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_op_buffer.h"

#include "base/containers/stack_container.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_op_reader.h"
#include "cc/paint/paint_op_writer.h"
#include "cc/paint/paint_record.h"
#include "third_party/skia/include/core/SkAnnotation.h"
#include "third_party/skia/include/core/SkCanvas.h"

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
                                             const SkRect& bounds,
                                             uint8_t alpha) {
  bool unset = bounds.x() == PaintOp::kUnsetRect.x();
  canvas->saveLayerAlpha(unset ? nullptr : &bounds, alpha);
  SkMatrix unused_matrix;
  raster_fn(op, canvas, unused_matrix);
  canvas->restore();
}

// Helper template to share common code for RasterWithAlpha when paint ops
// have or don't have PaintFlags.
template <typename T, bool HasFlags>
struct Rasterizer {
  static void RasterWithAlpha(const T* op,
                              SkCanvas* canvas,
                              const SkRect& bounds,
                              uint8_t alpha) {
    static_assert(
        !T::kHasPaintFlags,
        "This function should not be used for a PaintOp that has PaintFlags");
    DCHECK(T::kIsDrawOp);
    RasterWithAlphaInternal(&T::Raster, op, canvas, bounds, alpha);
  }
};

NOINLINE static void RasterWithAlphaInternalForFlags(
    RasterWithFlagsFunction raster_fn,
    const PaintOpWithFlags* op,
    SkCanvas* canvas,
    const SkRect& bounds,
    uint8_t alpha) {
  SkMatrix unused_matrix;
  if (alpha == 255) {
    raster_fn(op, &op->flags, canvas, unused_matrix);
  } else if (op->flags.SupportsFoldingAlpha()) {
    PaintFlags flags = op->flags;
    flags.setAlpha(SkMulDiv255Round(flags.getAlpha(), alpha));
    raster_fn(op, &flags, canvas, unused_matrix);
  } else {
    bool unset = bounds.x() == PaintOp::kUnsetRect.x();
    canvas->saveLayerAlpha(unset ? nullptr : &bounds, alpha);
    raster_fn(op, &op->flags, canvas, unused_matrix);
    canvas->restore();
  }
}

template <typename T>
struct Rasterizer<T, true> {
  static void RasterWithAlpha(const T* op,
                              SkCanvas* canvas,
                              const SkRect& bounds,
                              uint8_t alpha) {
    static_assert(T::kHasPaintFlags,
                  "This function expects the PaintOp to have PaintFlags");
    DCHECK(T::kIsDrawOp);
    RasterWithAlphaInternalForFlags(&T::RasterWithFlags, op, canvas, bounds,
                                    alpha);
  }
};

// These should never be used, as we should recurse into them to draw their
// contained op with alpha instead.
template <bool HasFlags>
struct Rasterizer<DrawRecordOp, HasFlags> {
  static void RasterWithAlpha(const DrawRecordOp* op,
                              SkCanvas* canvas,
                              const SkRect& bounds,
                              uint8_t alpha) {
    NOTREACHED();
  }
};
template <bool HasFlags>
struct Rasterizer<DrawDisplayItemListOp, HasFlags> {
  static void RasterWithAlpha(const DrawDisplayItemListOp* op,
                              SkCanvas* canvas,
                              const SkRect& bounds,
                              uint8_t alpha) {
    NOTREACHED();
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

using DeserializeFunction = const PaintOp* (*)(const void* input,
                                               size_t input_size,
                                               void* output,
                                               size_t output_size);

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
                                     const SkRect& bounds,
                                     uint8_t alpha);
#define M(T) \
  T::kIsDrawOp ? [](const PaintOp* op, SkCanvas* canvas, const SkRect& bounds, \
                    uint8_t alpha) { \
    Rasterizer<T, T::kHasPaintFlags>::RasterWithAlpha( \
        static_cast<const T*>(op), canvas, bounds, alpha); \
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
  helper.Write(op->count);
  helper.Write(op->bytes);
  helper.WriteArray(op->count, op->GetArray());
  helper.WriteData(op->bytes, op->GetData());
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
  helper.Write(op->bytes);
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

const PaintOp* AnnotateOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(AnnotateOp));
  AnnotateOp* op = reinterpret_cast<AnnotateOp*>(output);
  op->type = static_cast<uint8_t>(PaintOpType::Annotate);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->annotation_type);
  helper.Read(&op->rect);
  helper.Read(&op->data);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* ClipPathOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(ClipPathOp));
  ClipPathOp* op = reinterpret_cast<ClipPathOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->path);
  helper.Read(&op->op);
  helper.Read(&op->antialias);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* ClipRectOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  if (input_size < sizeof(ClipRectOp))
    return nullptr;
  return reinterpret_cast<const ClipRectOp*>(input);
}

const PaintOp* ClipRRectOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  if (input_size < sizeof(ClipRRectOp))
    return nullptr;
  return reinterpret_cast<const ClipRRectOp*>(input);
}

const PaintOp* ConcatOp::Deserialize(const void* input,
                                     size_t input_size,
                                     void* output,
                                     size_t output_size) {
  if (input_size < sizeof(ConcatOp))
    return nullptr;
  return reinterpret_cast<const ConcatOp*>(input);
}

const PaintOp* DrawArcOp::Deserialize(const void* input,
                                      size_t input_size,
                                      void* output,
                                      size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawArcOp));
  DrawArcOp* op = reinterpret_cast<DrawArcOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->oval);
  helper.Read(&op->start_angle);
  helper.Read(&op->sweep_angle);
  helper.Read(&op->use_center);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawCircleOp::Deserialize(const void* input,
                                         size_t input_size,
                                         void* output,
                                         size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawCircleOp));
  DrawCircleOp* op = reinterpret_cast<DrawCircleOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->cx);
  helper.Read(&op->cy);
  helper.Read(&op->radius);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawColorOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  if (input_size < sizeof(DrawColorOp))
    return nullptr;
  return reinterpret_cast<const DrawColorOp*>(input);
}

const PaintOp* DrawDisplayItemListOp::Deserialize(const void* input,
                                                  size_t input_size,
                                                  void* output,
                                                  size_t output_size) {
  // TODO(enne): see comments in serialize.  Should this get flattened
  // and never deserialized?
  return nullptr;
}

const PaintOp* DrawDRRectOp::Deserialize(const void* input,
                                         size_t input_size,
                                         void* output,
                                         size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawDRRectOp));
  DrawDRRectOp* op = reinterpret_cast<DrawDRRectOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->outer);
  helper.Read(&op->inner);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawImageOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawImageOp));
  DrawImageOp* op = reinterpret_cast<DrawImageOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->image);
  helper.Read(&op->left);
  helper.Read(&op->top);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawImageRectOp::Deserialize(const void* input,
                                            size_t input_size,
                                            void* output,
                                            size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawImageRectOp));
  DrawImageRectOp* op = reinterpret_cast<DrawImageRectOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->image);
  helper.Read(&op->src);
  helper.Read(&op->dst);
  helper.Read(&op->constraint);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawIRectOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawIRectOp));
  DrawIRectOp* op = reinterpret_cast<DrawIRectOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->rect);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawLineOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawLineOp));
  DrawLineOp* op = reinterpret_cast<DrawLineOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->x0);
  helper.Read(&op->y0);
  helper.Read(&op->x1);
  helper.Read(&op->y1);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawOvalOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawOvalOp));
  DrawOvalOp* op = reinterpret_cast<DrawOvalOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->oval);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawPathOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawPathOp));
  DrawPathOp* op = reinterpret_cast<DrawPathOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->path);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawPosTextOp::Deserialize(const void* input,
                                          size_t input_size,
                                          void* output,
                                          size_t output_size) {
  // TODO(enne): This is a bit of a weird condition, but to avoid the code
  // complexity of every Deserialize function being able to (re)allocate
  // an aligned buffer of the right size, this function asserts that it
  // will have enough size for the extra data.  It's guaranteed that any extra
  // memory is at most |input_size| so that plus the op size is an upper bound.
  // The caller has to awkwardly do this allocation though, sorry.
  CHECK_GE(output_size, sizeof(DrawPosTextOp) + input_size);
  DrawPosTextOp* op = reinterpret_cast<DrawPosTextOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->count);
  helper.Read(&op->bytes);
  helper.ReadArray(op->count, op->GetArray());
  helper.ReadData(op->bytes, op->GetData());
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawRecordOp::Deserialize(const void* input,
                                         size_t input_size,
                                         void* output,
                                         size_t output_size) {
  // TODO(enne): see comments in draw display item list op etc
  return nullptr;
}

const PaintOp* DrawRectOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawRectOp));
  DrawRectOp* op = reinterpret_cast<DrawRectOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->rect);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawRRectOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawRRectOp));
  DrawRRectOp* op = reinterpret_cast<DrawRRectOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->rrect);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawTextOp::Deserialize(const void* input,
                                       size_t input_size,
                                       void* output,
                                       size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawTextOp) + input_size);
  DrawTextOp* op = reinterpret_cast<DrawTextOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->bytes);
  helper.ReadData(op->bytes, op->GetData());
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* DrawTextBlobOp::Deserialize(const void* input,
                                           size_t input_size,
                                           void* output,
                                           size_t output_size) {
  CHECK_GE(output_size, sizeof(DrawTextBlobOp));
  DrawTextBlobOp* op = reinterpret_cast<DrawTextBlobOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->blob);
  helper.Read(&op->x);
  helper.Read(&op->y);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* NoopOp::Deserialize(const void* input,
                                   size_t input_size,
                                   void* output,
                                   size_t output_size) {
  if (input_size < sizeof(NoopOp))
    return nullptr;
  return reinterpret_cast<const NoopOp*>(input);
}

const PaintOp* RestoreOp::Deserialize(const void* input,
                                      size_t input_size,
                                      void* output,
                                      size_t output_size) {
  if (input_size < sizeof(RestoreOp))
    return nullptr;
  return reinterpret_cast<const RestoreOp*>(input);
}

const PaintOp* RotateOp::Deserialize(const void* input,
                                     size_t input_size,
                                     void* output,
                                     size_t output_size) {
  if (input_size < sizeof(RotateOp))
    return nullptr;
  return reinterpret_cast<const RotateOp*>(input);
}

const PaintOp* SaveOp::Deserialize(const void* input,
                                   size_t input_size,
                                   void* output,
                                   size_t output_size) {
  if (input_size < sizeof(SaveOp))
    return nullptr;
  return reinterpret_cast<const SaveOp*>(input);
}

const PaintOp* SaveLayerOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  CHECK_GE(output_size, sizeof(SaveLayerOp));
  SaveLayerOp* op = reinterpret_cast<SaveLayerOp*>(output);

  PaintOpReader helper(input, input_size);
  helper.Read(&op->flags);
  helper.Read(&op->bounds);
  if (!helper.valid())
    return nullptr;
  return op;
}

const PaintOp* SaveLayerAlphaOp::Deserialize(const void* input,
                                             size_t input_size,
                                             void* output,
                                             size_t output_size) {
  if (input_size < sizeof(SaveLayerAlphaOp))
    return nullptr;
  return reinterpret_cast<const SaveLayerAlphaOp*>(input);
}

const PaintOp* ScaleOp::Deserialize(const void* input,
                                    size_t input_size,
                                    void* output,
                                    size_t output_size) {
  if (input_size < sizeof(ScaleOp))
    return nullptr;
  return reinterpret_cast<const ScaleOp*>(input);
}

const PaintOp* SetMatrixOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  if (input_size < sizeof(SetMatrixOp))
    return nullptr;
  return reinterpret_cast<const SetMatrixOp*>(input);
}

const PaintOp* TranslateOp::Deserialize(const void* input,
                                        size_t input_size,
                                        void* output,
                                        size_t output_size) {
  if (input_size < sizeof(TranslateOp))
    return nullptr;
  return reinterpret_cast<const TranslateOp*>(input);
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
  if (op->preserve_lcd_text_requests) {
    SkPaint paint;
    paint.setAlpha(op->alpha);
    canvas->saveLayerPreserveLCDTextRequests(unset ? nullptr : &op->bounds,
                                             &paint);
  } else {
    canvas->saveLayerAlpha(unset ? nullptr : &op->bounds, op->alpha);
  }
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

void PaintOp::RasterWithAlpha(SkCanvas* canvas,
                              const SkRect& bounds,
                              uint8_t alpha) const {
  g_raster_alpha_functions[type](this, canvas, bounds, alpha);
}

size_t PaintOp::Serialize(void* memory, size_t size) const {
  // Need at least enough room for a skip/type header.
  if (size < 4)
    return 0u;

  // TODO(enne): dcheck that memory is aligned

  size_t written = g_serialize_functions[type](this, memory, size);
  if (!written)
    return 0u;

  size_t skip =
      MathUtil::UncheckedRoundUp(written, PaintOpBuffer::PaintOpAlign);
  DCHECK_LE(written, size);
  DCHECK_LT(skip, static_cast<size_t>(1) << 24);
  // If rounding up overflows size, then skip exactly to the end.
  // The op itself will still fit, but the next op wouldn't be aligned
  // if there was more room.
  skip = std::min(size, skip);

  // Update skip and type now that the size is known.
  static_cast<uint32_t*>(memory)[0] = type | skip << 8;

  return skip;
}

const PaintOp* PaintOp::Deserialize(const void* input,
                                    size_t input_size,
                                    void* output,
                                    size_t output_size) {
  const PaintOp* serialized = reinterpret_cast<const PaintOp*>(input);
  uint32_t skip = serialized->skip;
  CHECK_GE(input_size, skip);
  return g_deserialize_functions[serialized->type](input, skip, output,
                                                   output_size);
}

int ClipPathOp::CountSlowPaths() const {
  return antialias && !path.isConvex() ? 1 : 0;
}

int DrawDisplayItemListOp::CountSlowPaths() const {
  return list->NumSlowPaths();
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
  return list->BytesUsed();
}

bool DrawDisplayItemListOp::HasDiscardableImages() const {
  return list->HasDiscardableImages();
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

  const size_t active_range = range_indices[*range_index];
  DCHECK_GE(iter->op_idx(), range_starts[active_range]);

  // This grabs the PaintOp from the current iterator position, and advances it
  // to the next position immediately. We'll see we reached the end of the
  // buffer on the next call to this method.
  const PaintOp* op = **iter;
  ++*iter;

  if (active_range + 1 == range_starts.size()) {
    // In the last possible range, so let the iter go right to the end of the
    // buffer.
    return op;
  }

  const size_t range_end = range_starts[active_range + 1];
  DCHECK_LE(iter->op_idx(), range_end);
  if (iter->op_idx() < range_end) {
    // Still inside the range, so let the iter be.
    return op;
  }

  if (*range_index + 1 == range_indices.size()) {
    // We're now past the last range that we want to iterate.
    *iter = iter->end();
    return op;
  }

  // Move to the next range.
  ++(*range_index);
  size_t next_range_start = range_starts[range_indices[*range_index]];
  while (iter->op_idx() < next_range_start)
    ++(*iter);
  return op;
}

// When |op| is a nested PaintOpBuffer, this returns the PaintOp inside
// that buffer if the buffer contains a single drawing op, otherwise it
// returns null. This searches recursively if the PaintOpBuffer contains only
// another PaintOpBuffer.
static const PaintOp* GetNestedSingleDrawingOp(const PaintOp* op) {
  if (!op->IsDrawOp())
    return nullptr;

  while (op->GetType() == PaintOpType::DrawRecord ||
         op->GetType() == PaintOpType::DrawDisplayItemList) {
    if (op->GetType() == PaintOpType::DrawDisplayItemList) {
      // TODO(danakj): If we could inspect the PaintOpBuffer here, then
      // we could see if it is a single draw op.
      return nullptr;
    }
    auto* draw_record_op = static_cast<const DrawRecordOp*>(op);
    if (draw_record_op->record->size() > 1) {
      // If there's more than one op, then we need to keep the
      // SaveLayer.
      return nullptr;
    }

    // Recurse into the single-op DrawRecordOp and make sure it's a
    // drawing op.
    op = draw_record_op->record->GetFirstOp();
    if (!op->IsDrawOp())
      return nullptr;
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
  // This function could be wrapped in some sort of iterator?

  size_t remaining_bytes = size;
  const char* ptr = static_cast<const char*>(memory);

  // Single allocation on the heap to deserialize into temporarily.
  size_t buffer_size = sizeof(LargestPaintOp) * 2;
  std::unique_ptr<char, base::AlignedFreeDeleter> buffer(
      static_cast<char*>(base::AlignedAlloc(buffer_size, PaintOpAlign)));

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

    // Make sure the temporary buffer can fit the deserialized op.
    // See comments in DrawPosTextOp.
    size_t min_size = sizeof(LargestPaintOp) + skip;
    if (buffer_size < min_size) {
      while (buffer_size < min_size)
        buffer_size *= 2;
      buffer.reset(
          static_cast<char*>(base::AlignedAlloc(buffer_size, PaintOpAlign)));
    }

    if (const PaintOp* op = g_deserialize_functions[type](
            ptr, skip, buffer.get(), buffer_size)) {
      g_raster_functions[type](op, canvas, original_ctm);
    } else {
      // Failure to deserialize.
      return false;
    }

    ptr += skip;
    remaining_bytes -= skip;
  }

  return true;
}

void PaintOpBuffer::playback(SkCanvas* canvas,
                             SkPicture::AbortCallback* callback) const {
  static auto* zero = new std::vector<size_t>({0});
  // Treats the entire PaintOpBuffer as a single range.
  PlaybackRanges(*zero, *zero, canvas, callback);
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

  // Prevent PaintOpBuffers from having side effects back into the canvas.
  SkAutoCanvasRestore save_restore(canvas, true);

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
    // draw.  See also: similar code in SkRecordOpts.
    // TODO(enne): consider making this recursive?
    // TODO(enne): should we avoid this if the SaveLayerAlphaOp has bounds?
    if (op->GetType() == PaintOpType::SaveLayerAlpha) {
      const PaintOp* second =
          NextOp(range_starts, range_indices, &stack, &iter, &range_index);
      const PaintOp* third = nullptr;
      if (second) {
        if (second->GetType() == PaintOpType::Restore) {
          continue;
        }

        // Find a nested drawing PaintOp to replace |second| if possible, while
        // holding onto the pointer to |second| in case we can't find a nested
        // drawing op to replace it with.
        const PaintOp* draw_op = GetNestedSingleDrawingOp(second);

        if (draw_op) {
          third =
              NextOp(range_starts, range_indices, &stack, &iter, &range_index);
          if (third && third->GetType() == PaintOpType::Restore) {
            auto* save_op = static_cast<const SaveLayerAlphaOp*>(op);
            draw_op->RasterWithAlpha(canvas, save_op->bounds, save_op->alpha);
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
