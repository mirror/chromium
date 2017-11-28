// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_TOOLS_FUZZERS_FILTER_PROTO_CONVERTER_H_
#define SKIA_TOOLS_FUZZERS_FILTER_PROTO_CONVERTER_H_

#include "skia/tools/fuzzers/filter.pb.h"

#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "third_party/skia/include/core/SkPoint.h"

using google::protobuf::Message;
using google::protobuf::FieldDescriptor;

namespace filter_proto_converter {
class Converter {
 public:
  Converter();
  Converter(const Converter&);
  ~Converter();
  std::string Convert(const Input&);

 private:
  static const uint32_t kProfileLookupTable[];
  static const uint32_t kInputColorSpaceLookupTable[];
  static const uint32_t kPCSLookupTable[];
  static const uint32_t kTagLookupTable[];

  static const char kPictureMagicString[];
  static const char kSkPictReaderTag[];
  static const char kSkPictEOFTag[];
  static const uint8_t kCountNibBits[];
  static constexpr uint8_t kICC_Flag = 1 << 1;
  static constexpr size_t kICCTagTableEntrySize = 12;
  static constexpr uint32_t kMatrix_Flag = 1 << 0;
  static constexpr uint8_t k0_Version = 0;
  static constexpr uint8_t kTransferFn_Flag = 1 << 3;

  // The size of kColorTableBuffer.
  static const int kColorTableBufferLength;
  // Used to bound obj_depth_.
  static const int kObjDepthLimit;
  // Only used for CHECK code.
  static const uint32_t kMax3ByteUInt;
  // Also only used for CHECK code.
  static const uint32_t kUInt32WithAllLastByteSet;
  // Used by ColorTableToArray to store a ColorTable Message as an array.
  static uint8_t kColorTableBuffer[];
  // char vector that will be used to create the string output that is returned
  // by Convert().
  std::vector<char> output_;
  // Stores the size of output_ when a skia object is being written (since they
  // need to store their size).
  std::vector<size_t> start_sizes_;
  // Only allow kStrokeAndFill_Style and kStroke_Style to be used once since
  // too many will cause OOMs or timeouts.
  bool stroke_style_used_;
  // Used to keep track of how nested skia objects are so that we can avoid OOMs
  // and timeouts that are caused by being too nested.
  int obj_depth_;
  // Not used yet. Will be used by WriteTagHeader once ICC tags are supported.
  int tag_offset_;
  static const int kNumBound;
  // RNG used to replace NaN values in fields where they make fuzzing difficult.
  std::mt19937 nan_substitutor_;

  void Reset();
  void Visit(const PictureImageFilter&);
  void Visit(const Picture&);
  void Visit(const Message&);
  void Visit(const PictureData&);
  void Visit(const RecordingData&);
  void Visit(const LightParent&);
  void Visit(const ImageFilterChild&);
  void Visit(const ImageFilterParent&, const int num_inputs_required = -1);
  void Visit(const MatrixImageFilter&);
  void Visit(const Matrix&);
  void Visit(const SpecularLightingImageFilter&);
  void Visit(const PaintImageFilter&);
  void Visit(const Paint&);
  void Visit(const Typeface&);
  void Visit(const PaintEffects&);
  void Visit(const PathEffectChild&);
  void Visit(const LooperChild&);
  void Visit(const LayerDrawLooper&);
  void Visit(const LayerInfo&);
  void Visit(const ColorFilterChild&);
  void Visit(const ComposeColorFilter&);
  void Visit(const OverdrawColorFilter&);
  void Visit(const ToSRGBColorFilter&);
  void Visit(const ColorFilterMatrix&);
  void Visit(const ColorMatrixFilterRowMajor255&);
  void Visit(const MergeImageFilter&);
  void Visit(const XfermodeImageFilter&);
  void Visit(const DiffuseLightingImageFilter&);
  void Visit(const XfermodeImageFilter_Base&);
  void Visit(const TileImageFilter&);
  void Visit(const OffsetImageFilter&);
  void Visit(const ErodeImageFilter&);
  void Visit(const DilateImageFilter&);
  void Visit(const DiscretePathEffect&);
  void Visit(const MatrixConvolutionImageFilter&);
  void Visit(const MagnifierImageFilter&);
  void Visit(const LocalMatrixImageFilter&);
  void Visit(const ImageSource&);
  void Visit(const Path&);
  void Visit(const PathRef&);
  void Visit(const DropShadowImageFilter&);
  void Visit(const DisplacementMapEffect&);
  void Visit(const ComposeImageFilter&);
  void Visit(const ColorFilterImageFilter&);
  void Visit(const BlurImageFilterImpl&);
  void Visit(const AlphaThresholdFilter&);
  void Visit(const Region&);
  void Visit(const Path1DPathEffect&);
  void Visit(const PairPathEffect&);
  void Visit(const ShaderChild&);
  void Visit(const Color4Shader&);
  void Visit(const GradientDescriptor&);
  void Visit(const GradientParent&);
  void Visit(const TwoPointConicalGradient&);
  void Visit(const LinearGradient&);
  void Visit(const SweepGradient&);
  void Visit(const RadialGradient&);
  void Visit(const PictureShader&);
  void Visit(const LocalMatrixShader&);
  void Visit(const ComposeShader&);
  void Visit(const ColorFilterShader&);
  void Visit(const ImageShader&);
  void Visit(const Color4f&);
  void Visit(const Image&);
  void Visit(const ImageData&);
  void Visit(const Bitmap&);
  void Visit(const ICC&);
  void WriteIgnoredFields(const int num_fields);
  void Visit(const ColorSpace&);
  void Visit(const ColorSpace_XYZ&);
  void Visit(const ColorSpaceNamed&);
  void WriteColorSpaceVersion();
  void Visit(const TransferFn&);
  void Visit(const ImageInfo&, const int32_t width, const int32_t height);
  int32_t BoundIlluminant(float illuminant, const float bound) const;
  void Visit(const RasterizerChild&);
  void Visit(const LayerRasterizer&);
  void Visit(const MaskFilterChild&);
  void Visit(const Table_ColorFilter&);
  const uint8_t* ColorTableToArray(const ColorTable&);
  void Visit(const EmbossMaskFilter&);
  void Visit(const EmbossMaskFilterLight&);
  void Visit(const DashImpl&);
  void Visit(const Path2DPathEffect&);
  void Visit(const ArithmeticImageFilterImpl&);
  void WriteString(std::string str);
  void WriteBytesWritten();
  void RecordSize();
  void InsertSize(const size_t, const uint32_t position);
  void Pad(const size_t write_size);
  size_t ComputeMinByteSize(int32_t width,
                            int32_t height,
                            ImageInfo::AlphaType alpha_type);

  void WriteArray(
      const google::protobuf::RepeatedField<uint32_t>& repeated_field,
      const size_t size);

  void WriteArray(const char* arr, const size_t size);
  void WriteBool(const bool bool_val);
  void WriteTagSize(const char (&tag)[4], const size_t size);
  void WriteFields(const Message&,
                   const unsigned start = 1,
                   const unsigned end = 0);

  void WriteIndex(const uint32_t index);

  std::tuple<int32_t, int32_t, int32_t>
  GetNumPixelBytes(const ImageInfo& image_info, int32_t width, int32_t height);

  void MarkNotPresent();
  size_t PopStartSize();

  std::string FieldToObjName(const std::string& field_name) const;
  void CheckAlignment() const;
  void AppendSkPoint(std::vector<SkPoint>&, const Point&);

  void WriteInt32(uint32_t);
  void WriteNum(const char (&num)[4]);

  template <class T>
  void WriteNum(T num);

  template <typename T>
  void WriteBigEndian(const T num);

  template <class T>
  void VisitFlattenable(const T& skia_object, const std::string& object_name);

  template <typename T>
  T BoundNum(T num, int num_bound) const;

  template <typename T>
  T BoundNum(T num);
  float BoundFloat(float num);

  template <typename T>
  uint8_t ToUInt8(const T input_num) const;

  template <class T>
  void Visit(const google::protobuf::RepeatedPtrField<T>& repeated_field);

  float GetRandomFloat(std::mt19937* gen);
  void WriteMatrixField(float field_value);
  void WriteTagHeader(const uint32_t tag, const uint32_t len);
  int Abs(const int val) const;
};
};      // namespace filter_proto_converter
#endif  // SKIA_TOOLS_FUZZERS_FILTER_PROTO_CONVERTER_H_
