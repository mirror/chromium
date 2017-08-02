// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_PAINT_OP_BUFFER_TEST_UTILS_H_
#define CC_TEST_PAINT_OP_BUFFER_TEST_UTILS_H_

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
// using POBTest = PaintOpBufferTestUtils;
// ...
// for (int i = 0; i < 10; ++i) {
//   SCOPED_TRACE(base::Sprintf("Op#%d does not match", i));
//   POBTest::ExpectOpsEqual(actual_ops[i], expected_ops[i]);
// }
//
// This should provide sufficient contextual information to allow for easier
// debugging.
//
// This class also exposes some test variables vial Test*() functions that
// are used internally in the Push* functions, but can also be useful in other
// tests.
class PaintOpBufferTestUtils {
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

  static void ExpectEqual(SkFlattenable* expected, SkFlattenable* actual);
  static void ExpectEqual(const PaintShader* one, const PaintShader* two);
  static void ExpectEqual(const PaintFlags& expected, const PaintFlags& actual);
  static void ExpectEqual(const PaintImage& original,
                          const PaintImage& written);
  static void ExpectEqual(const AnnotateOp* original,
                          const AnnotateOp* written);
  static void ExpectEqual(const ClipPathOp* original,
                          const ClipPathOp* written);
  static void ExpectEqual(const ClipRectOp* original,
                          const ClipRectOp* written);
  static void ExpectEqual(const ClipRRectOp* original,
                          const ClipRRectOp* written);
  static void ExpectEqual(const ConcatOp* original, const ConcatOp* written);
  static void ExpectEqual(const DrawArcOp* original, const DrawArcOp* written);
  static void ExpectEqual(const DrawCircleOp* original,
                          const DrawCircleOp* written);
  static void ExpectEqual(const DrawColorOp* original,
                          const DrawColorOp* written);
  static void ExpectEqual(const DrawDRRectOp* original,
                          const DrawDRRectOp* written);
  static void ExpectEqual(const DrawImageOp* original,
                          const DrawImageOp* written);
  static void ExpectEqual(const DrawImageRectOp* original,
                          const DrawImageRectOp* written);
  static void ExpectEqual(const DrawIRectOp* original,
                          const DrawIRectOp* written);
  static void ExpectEqual(const DrawLineOp* original,
                          const DrawLineOp* written);
  static void ExpectEqual(const DrawOvalOp* original,
                          const DrawOvalOp* written);
  static void ExpectEqual(const DrawPathOp* original,
                          const DrawPathOp* written);
  static void ExpectEqual(const DrawPosTextOp* original,
                          const DrawPosTextOp* written);
  static void ExpectEqual(const DrawRectOp* original,
                          const DrawRectOp* written);
  static void ExpectEqual(const DrawRRectOp* original,
                          const DrawRRectOp* written);
  static void ExpectEqual(const DrawTextOp* original,
                          const DrawTextOp* written);
  static void ExpectEqual(const DrawTextBlobOp* original,
                          const DrawTextBlobOp* written);
  static void ExpectEqual(const NoopOp* original, const NoopOp* written);
  static void ExpectEqual(const RestoreOp* original, const RestoreOp* written);
  static void ExpectEqual(const RotateOp* original, const RotateOp* written);
  static void ExpectEqual(const SaveOp* original, const SaveOp* written);
  static void ExpectEqual(const SaveLayerOp* original,
                          const SaveLayerOp* written);
  static void ExpectEqual(const SaveLayerAlphaOp* original,
                          const SaveLayerAlphaOp* written);
  static void ExpectEqual(const ScaleOp* original, const ScaleOp* written);
  static void ExpectEqual(const SetMatrixOp* original,
                          const SetMatrixOp* written);
  static void ExpectEqual(const TranslateOp* original,
                          const TranslateOp* written);

  static void FillArbitraryShaderValues(PaintShader* shader, bool use_matrix);
};

}  // namespace cc

#endif  // CC_TEST_PAINT_OP_BUFFER_TEST_UTILS_H_
