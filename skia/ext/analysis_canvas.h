// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_ANALYSIS_CANVAS_H_
#define SKIA_EXT_ANALYSIS_CANVAS_H_

#include "base/compiler_specific.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace skia {

// Does not render anything, but gathers statistics about a region
// (specified as a clip rectangle) of an SkPicture as the picture is
// played back through it.
// To use: play a picture into the canvas, and then check result.
class SK_API AnalysisCanvas : public SkCanvas, public SkDrawPictureCallback {
 public:
  AnalysisCanvas(int width, int height);
  virtual ~AnalysisCanvas();

  // Returns true when a SkColor can be used to represent result.
  bool GetColorIfSolid(SkColor* color) const;

  void SetForceNotSolid(bool flag);
  void SetForceNotTransparent(bool flag);

  // SkDrawPictureCallback override.
  virtual bool abortDrawing() override;

  // SkCanvas overrides.
  virtual void clear(SkColor) override;
  virtual void drawPaint(const SkPaint& paint) override;
  virtual void drawPoints(PointMode,
                          size_t count,
                          const SkPoint pts[],
                          const SkPaint&) override;
  virtual void drawOval(const SkRect&, const SkPaint&) override;
  virtual void drawRect(const SkRect&, const SkPaint&) override;
  virtual void drawRRect(const SkRRect&, const SkPaint&) override;
  virtual void drawPath(const SkPath& path, const SkPaint&) override;
  virtual void drawBitmap(const SkBitmap&,
                          SkScalar left,
                          SkScalar top,
                          const SkPaint* paint = NULL) override;
  virtual void drawBitmapRectToRect(const SkBitmap&,
                                    const SkRect* src,
                                    const SkRect& dst,
                                    const SkPaint* paint,
                                    DrawBitmapRectFlags flags) override;
  virtual void drawBitmapMatrix(const SkBitmap&,
                                const SkMatrix&,
                                const SkPaint* paint = NULL) override;
  virtual void drawBitmapNine(const SkBitmap& bitmap,
                              const SkIRect& center,
                              const SkRect& dst,
                              const SkPaint* paint = NULL) override;
  virtual void drawSprite(const SkBitmap&, int left, int top,
                          const SkPaint* paint = NULL) override;
  virtual void drawVertices(VertexMode,
                            int vertexCount,
                            const SkPoint vertices[],
                            const SkPoint texs[],
                            const SkColor colors[],
                            SkXfermode*,
                            const uint16_t indices[],
                            int indexCount,
                            const SkPaint&) override;

 protected:
  virtual void willSave() override;
  virtual SaveLayerStrategy willSaveLayer(const SkRect*,
                                          const SkPaint*,
                                          SaveFlags) override;
  virtual void willRestore() override;

  virtual void onClipRect(const SkRect& rect,
                          SkRegion::Op op,
                          ClipEdgeStyle edge_style) override;
  virtual void onClipRRect(const SkRRect& rrect,
                           SkRegion::Op op,
                           ClipEdgeStyle edge_style) override;
  virtual void onClipPath(const SkPath& path,
                          SkRegion::Op op,
                          ClipEdgeStyle edge_style) override;
  virtual void onClipRegion(const SkRegion& deviceRgn,
                            SkRegion::Op op) override;

  virtual void onDrawText(const void* text,
                          size_t byteLength,
                          SkScalar x,
                          SkScalar y,
                          const SkPaint&) override;
  virtual void onDrawPosText(const void* text,
                             size_t byteLength,
                             const SkPoint pos[],
                             const SkPaint&) override;
  virtual void onDrawPosTextH(const void* text,
                              size_t byteLength,
                              const SkScalar xpos[],
                              SkScalar constY,
                              const SkPaint&) override;
  virtual void onDrawTextOnPath(const void* text,
                                size_t byteLength,
                                const SkPath& path,
                                const SkMatrix* matrix,
                                const SkPaint&) override;
  virtual void onDrawTextBlob(const SkTextBlob* blob,
                              SkScalar x,
                              SkScalar y,
                              const SkPaint& paint) override;
  virtual void onDrawDRRect(const SkRRect& outer,
                            const SkRRect& inner,
                            const SkPaint&) override;

  void OnComplexClip();

 private:
  typedef SkCanvas INHERITED;

  int saved_stack_size_;
  int force_not_solid_stack_level_;
  int force_not_transparent_stack_level_;

  bool is_forced_not_solid_;
  bool is_forced_not_transparent_;
  bool is_solid_color_;
  SkColor color_;
  bool is_transparent_;
  int draw_op_count_;
};

}  // namespace skia

#endif  // SKIA_EXT_ANALYSIS_CANVAS_H_

