// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/containers/hash_tables.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "skia/ext/benchmarking_canvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/utils/SkProxyCanvas.h"

namespace skia {

class AutoStamper {
public:
  AutoStamper(TimingCanvas* timing_canvas);
  ~AutoStamper();

private:
  TimingCanvas* timing_canvas_;
  base::TimeTicks start_ticks_;
};

class TimingCanvas : public SkProxyCanvas {
public:
  TimingCanvas(int width, int height, const BenchmarkingCanvas* track_canvas)
      : tracking_canvas_(track_canvas) {
    surface_ = skia::AdoptRef(SkSurface::NewRasterN32Premul(width, height));

    setProxy(surface_->getCanvas());
  }

  ~TimingCanvas() override {}

  double GetTime(size_t index) {
    TimingsMap::const_iterator timing_info = timings_map_.find(index);
    return timing_info != timings_map_.end()
        ? timing_info->second.InMillisecondsF()
        : 0.0;
  }

  // SkCanvas overrides.
  void willSave() override {
    AutoStamper stamper(this);
    SkProxyCanvas::willSave();
  }

  SaveLayerStrategy willSaveLayer(const SkRect* bounds,
                                  const SkPaint* paint,
                                  SaveFlags flags) override {
    AutoStamper stamper(this);
    return SkProxyCanvas::willSaveLayer(bounds, paint, flags);
  }

  void willRestore() override {
    AutoStamper stamper(this);
    SkProxyCanvas::willRestore();
  }

  void onDrawPaint(const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawPaint(paint);
  }

  void onDrawPoints(PointMode mode,
                    size_t count,
                    const SkPoint pts[],
                    const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawPoints(mode, count, pts, paint);
  }

  void onDrawOval(const SkRect& rect, const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawOval(rect, paint);
  }

  void onDrawRect(const SkRect& rect, const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawRect(rect, paint);
  }

  void onDrawRRect(const SkRRect& rrect, const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawRRect(rrect, paint);
  }

  void onDrawPath(const SkPath& path, const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawPath(path, paint);
  }

  void onDrawBitmap(const SkBitmap& bitmap,
                    SkScalar left,
                    SkScalar top,
                    const SkPaint* paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawBitmap(bitmap, left, top, paint);
  }

  void onDrawBitmapRect(const SkBitmap& bitmap,
                        const SkRect* src,
                        const SkRect& dst,
                        const SkPaint* paint,
                        DrawBitmapRectFlags flags) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawBitmapRect(bitmap, src, dst, paint, flags);
  }

  void onDrawSprite(const SkBitmap& bitmap,
                    int left,
                    int top,
                    const SkPaint* paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawSprite(bitmap, left, top, paint);
  }

  void onDrawVertices(VertexMode vmode,
                      int vertexCount,
                      const SkPoint vertices[],
                      const SkPoint texs[],
                      const SkColor colors[],
                      SkXfermode* xmode,
                      const uint16_t indices[],
                      int indexCount,
                      const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawVertices(vmode, vertexCount, vertices, texs, colors,
                                  xmode, indices, indexCount, paint);
  }

protected:
 void onDrawText(const void* text,
                 size_t byteLength,
                 SkScalar x,
                 SkScalar y,
                 const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawText(text, byteLength, x, y, paint);
  }

  void onDrawPosText(const void* text,
                     size_t byteLength,
                     const SkPoint pos[],
                     const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawPosText(text, byteLength, pos, paint);
  }

  void onDrawPosTextH(const void* text,
                      size_t byteLength,
                      const SkScalar xpos[],
                      SkScalar constY,
                      const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawPosTextH(text, byteLength, xpos, constY, paint);
  }

  void onDrawTextOnPath(const void* text,
                        size_t byteLength,
                        const SkPath& path,
                        const SkMatrix* matrix,
                        const SkPaint& paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawTextOnPath(text, byteLength, path, matrix, paint);
  }

  void onClipRect(const SkRect& rect,
                  SkRegion::Op op,
                  ClipEdgeStyle edge_style) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onClipRect(rect, op, edge_style);
  }

  void onClipRRect(const SkRRect& rrect,
                   SkRegion::Op op,
                   ClipEdgeStyle edge_style) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onClipRRect(rrect, op, edge_style);
  }

  void onClipPath(const SkPath& path,
                  SkRegion::Op op,
                  ClipEdgeStyle edge_style) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onClipPath(path, op, edge_style);
  }

  void onClipRegion(const SkRegion& region, SkRegion::Op op) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onClipRegion(region, op);
  }

  void onDrawPicture(const SkPicture* picture,
                     const SkMatrix* matrix,
                     const SkPaint* paint) override {
    AutoStamper stamper(this);
    SkProxyCanvas::onDrawPicture(picture, matrix, paint);
  }

private:
  typedef base::hash_map<size_t, base::TimeDelta> TimingsMap;
  TimingsMap timings_map_;

  skia::RefPtr<SkSurface> surface_;

  friend class AutoStamper;
  const BenchmarkingCanvas* tracking_canvas_;
};

AutoStamper::AutoStamper(TimingCanvas *timing_canvas)
    : timing_canvas_(timing_canvas) {
  start_ticks_ = base::TimeTicks::HighResNow();
}

AutoStamper::~AutoStamper() {
  base::TimeDelta delta = base::TimeTicks::HighResNow() - start_ticks_;
  int command_index = timing_canvas_->tracking_canvas_->CommandCount() - 1;
  DCHECK_GE(command_index, 0);
  timing_canvas_->timings_map_[command_index] = delta;
}

BenchmarkingCanvas::BenchmarkingCanvas(int width, int height)
    : SkNWayCanvas(width, height) {
  debug_canvas_ = skia::AdoptRef(SkNEW_ARGS(SkDebugCanvas, (width, height)));
  timing_canvas_ = skia::AdoptRef(SkNEW_ARGS(TimingCanvas, (width, height, this)));

  addCanvas(debug_canvas_.get());
  addCanvas(timing_canvas_.get());
}

BenchmarkingCanvas::~BenchmarkingCanvas() {
  removeAll();
}

size_t BenchmarkingCanvas::CommandCount() const {
  return debug_canvas_->getSize();
}

SkDrawCommand* BenchmarkingCanvas::GetCommand(size_t index) {
  DCHECK_LT(index, static_cast<size_t>(debug_canvas_->getSize()));
  return debug_canvas_->getDrawCommandAt(index);
}

double BenchmarkingCanvas::GetTime(size_t index) {
  DCHECK_LT(index,  static_cast<size_t>(debug_canvas_->getSize()));
  return timing_canvas_->GetTime(index);
}

} // namespace skia
