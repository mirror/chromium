// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_PAINT_OP_UTIL_H_
#define CC_TEST_PAINT_OP_UTIL_H_

#include <vector>

#include "cc/paint/paint_op_buffer.h"

namespace cc {

// A convenience class that provides several static functions that operate with
// paint op buffers. Specifically, it can add arbitrary[1] ops to the buffer and
// compare whether two PaintOps are equivalent.
//
// [1] arbitrary here means that it will populate the buffer with several of the
// given ops based on some predefined input. When working with these tests,
// you're encouraged to add more cases in the .cc file so that the tests cover
// more permutations of flags on the ops.
//
// Note that because the comparison function asserts/expects the results to be
// equal, it is best practice to provide some sort of contextual information
// with SCOPED_TRACEs. For example:
//
// for (int i = 0; i < 10; ++i) {
//   SCOPED_TRACE(base::Sprintf("Op#%d does not match", i));
//   PaintOpUtil::ExpectOpsEqual(actual_ops[i], expected_ops[i]);
// }
//
// This should provide sufficient contextual information to allow for easier
// debugging.
//
// This class also exposes some test variables vial Test*() functions that
// are used internally in the Push* functions, but can also be useful in other
// tests.
class PaintOpUtil {
 public:
  static const std::vector<float>& TestFloats();
  static const std::vector<uint8_t>& TestUint8s();
  static const std::vector<SkRect>& TestRects();
  static const std::vector<SkRRect>& TestRRects();
  static const std::vector<SkIRect>& TestIRects();
  static const std::vector<SkMatrix>& TestMatrices();
  static const std::vector<SkPath>& TestPaths();
  static const std::vector<PaintFlags>& TestFlags();
  static const std::vector<SkColor>& TestColors();
  static const std::vector<std::string>& TestStrings();
  static const std::vector<std::vector<SkPoint>>& TestPointArrays();
  static const std::vector<sk_sp<SkTextBlob>>& TestBlobs();
  static const std::vector<PaintImage>& TestImages();

  static void PushAnnotateOps(PaintOpBuffer* buffer);
  static void PushClipDeviceRectOps(PaintOpBuffer* buffer);
  static void PushClipPathOps(PaintOpBuffer* buffer);
  static void PushClipRectOps(PaintOpBuffer* buffer);
  static void PushClipRRectOps(PaintOpBuffer* buffer);
  static void PushConcatOps(PaintOpBuffer* buffer);
  static void PushDrawArcOps(PaintOpBuffer* buffer);
  static void PushDrawCircleOps(PaintOpBuffer* buffer);
  static void PushDrawColorOps(PaintOpBuffer* buffer);
  static void PushDrawDRRectOps(PaintOpBuffer* buffer);
  static void PushDrawImageOps(PaintOpBuffer* buffer);
  static void PushDrawImageRectOps(PaintOpBuffer* buffer);
  static void PushDrawIRectOps(PaintOpBuffer* buffer);
  static void PushDrawLineOps(PaintOpBuffer* buffer);
  static void PushDrawOvalOps(PaintOpBuffer* buffer);
  static void PushDrawPathOps(PaintOpBuffer* buffer);
  static void PushDrawPosTextOps(PaintOpBuffer* buffer);
  static void PushDrawRectOps(PaintOpBuffer* buffer);
  static void PushDrawRRectOps(PaintOpBuffer* buffer);
  static void PushDrawTextOps(PaintOpBuffer* buffer);
  static void PushDrawTextBlobOps(PaintOpBuffer* buffer);
  static void PushNoopOps(PaintOpBuffer* buffer);
  static void PushRestoreOps(PaintOpBuffer* buffer);
  static void PushRotateOps(PaintOpBuffer* buffer);
  static void PushSaveOps(PaintOpBuffer* buffer);
  static void PushSaveLayerOps(PaintOpBuffer* buffer);
  static void PushSaveLayerAlphaOps(PaintOpBuffer* buffer);
  static void PushScaleOps(PaintOpBuffer* buffer);
  static void PushSetMatrixOps(PaintOpBuffer* buffer);
  static void PushTranslateOps(PaintOpBuffer* buffer);

  static void ExpectOpsEqual(const PaintOp* original, const PaintOp* written);

  static void ExpectEqual(SkFlattenable* one, SkFlattenable* two);
  static void ExpectEqual(const PaintShader* one, const PaintShader* two);
  static void ExpectEqual(const PaintFlags& one, const PaintFlags& two);
  static void ExpectEqual(const PaintImage& one, const PaintImage& two);
  static void ExpectEqual(const AnnotateOp* one, const AnnotateOp* two);
  static void ExpectEqual(const ClipDeviceRectOp* one,
                          const ClipDeviceRectOp* two);
  static void ExpectEqual(const ClipPathOp* one, const ClipPathOp* two);
  static void ExpectEqual(const ClipRectOp* one, const ClipRectOp* two);
  static void ExpectEqual(const ClipRRectOp* one, const ClipRRectOp* two);
  static void ExpectEqual(const ConcatOp* one, const ConcatOp* two);
  static void ExpectEqual(const DrawArcOp* one, const DrawArcOp* two);
  static void ExpectEqual(const DrawCircleOp* one, const DrawCircleOp* two);
  static void ExpectEqual(const DrawColorOp* one, const DrawColorOp* two);
  static void ExpectEqual(const DrawDRRectOp* one, const DrawDRRectOp* two);
  static void ExpectEqual(const DrawImageOp* one, const DrawImageOp* two);
  static void ExpectEqual(const DrawImageRectOp* one,
                          const DrawImageRectOp* two);
  static void ExpectEqual(const DrawIRectOp* one, const DrawIRectOp* two);
  static void ExpectEqual(const DrawLineOp* one, const DrawLineOp* two);
  static void ExpectEqual(const DrawOvalOp* one, const DrawOvalOp* two);
  static void ExpectEqual(const DrawPathOp* one, const DrawPathOp* two);
  static void ExpectEqual(const DrawPosTextOp* one, const DrawPosTextOp* two);
  static void ExpectEqual(const DrawRectOp* one, const DrawRectOp* two);
  static void ExpectEqual(const DrawRRectOp* one, const DrawRRectOp* two);
  static void ExpectEqual(const DrawTextOp* one, const DrawTextOp* two);
  static void ExpectEqual(const DrawTextBlobOp* one, const DrawTextBlobOp* two);
  static void ExpectEqual(const NoopOp* one, const NoopOp* two);
  static void ExpectEqual(const RestoreOp* one, const RestoreOp* two);
  static void ExpectEqual(const RotateOp* one, const RotateOp* two);
  static void ExpectEqual(const SaveOp* one, const SaveOp* two);
  static void ExpectEqual(const SaveLayerOp* one, const SaveLayerOp* two);
  static void ExpectEqual(const SaveLayerAlphaOp* one,
                          const SaveLayerAlphaOp* two);
  static void ExpectEqual(const ScaleOp* one, const ScaleOp* two);
  static void ExpectEqual(const SetMatrixOp* one, const SetMatrixOp* two);
  static void ExpectEqual(const TranslateOp* one, const TranslateOp* two);

  static void FillArbitraryShaderValues(PaintShader* shader, bool use_matrix);
};

}  // namespace cc

#endif  // CC_TEST_PAINT_OP_UTIL_H_
