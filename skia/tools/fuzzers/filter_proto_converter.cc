// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/tools/fuzzers/filter_proto_converter.h"

#include <ctype.h>
#include <math.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "base/logging.h"
#include "third_party/protobuf/src/google/protobuf/descriptor.h"
#include "third_party/protobuf/src/google/protobuf/message.h"
#include "third_party/protobuf/src/google/protobuf/repeated_field.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkRect.h"

using google::protobuf::Reflection;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;

namespace filter_proto_converter {

enum FlatFlags {
  kHasTypeface_FlatFlag = 0x1,
  kHasEffects_FlatFlag = 0x2,
  kFlatFlagMask = 0x3,
};

enum ReadBufferFlags {
  kCrossProcess_Flag = 1 << 0,
  kScalarIsFloat_Flag = 1 << 1,
  kPtrIs64Bit_Flag = 1 << 2,
  kValidation_Flag = 1 << 3,
};

enum LightType {
  kDistant_LightType,
  kPoint_LightType,
  kSpot_LightType,
};

// Visit an optional FIELD if it was set on MSG.
#define VISIT_OPT(MSG, FIELD) \
  if (MSG.has_##FIELD()) {    \
    Visit(MSG.FIELD());       \
  }

// Visit a Skia Object that is named by OBJ_NAME and is stored on the FIELD
// field of MSG.
#define VISIT_NAMED_OPT_OBJ(MSG, FIELD, OBJ_NAME) \
  if (MSG.has_##FIELD()) {                        \
    VisitObj(MSG.FIELD(), std::string(OBJ_NAME)); \
  }

// Visit optional object and determine the name based on FIELD.
#define VISIT_OPT_OBJ(MSG, FIELD) \
  VISIT_NAMED_OPT_OBJ(MSG, FIELD, FieldToObjName(#FIELD));

// TODO(metzman): Transition version from VISIT_NAMED_OPT_OBJ to VISIT_OPT_OBJ.
#define VISIT_OPT_OBJ2(MSG, FIELD, OBJ_NAME) \
  VISIT_OPT_OBJ(MSG, FIELD);                 \
  CHECK(std::string(FieldToObjName(#FIELD)) == OBJ_NAME);

// Visit FIELD if it is set on MSG, or write a NULL to indictate it is not
// present.
#define VISIT_OR_NULL(MSG, FIELD) \
  VISIT_OPT(MSG, FIELD)           \
  else MarkNotPresent();

// Copied From third_party/skia/include/core/SkTypes.h:SkSetFourByteTag.
#define SET_FOUR_BYTE_TAG(A, B, C, D) \
  (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

// Copied from various parts of the skia codebase.
const char Converter::kSkPictEOFTag[] = {'e', 'o', 'f', ' '};
const char Converter::kSkPictReaderTag[] = {'r', 'e', 'a', 'd'};
const char Converter::kPictureMagicString[] = {'s', 'k', 'i', 'a',
                                               'p', 'i', 'c', 't'};

// 2^(3*8) = 2^24 = 16777216
static const uint32_t kMax3ByteUInt = 16777216;
static const uint32_t kUInt32WithAllLastByteSet =
    0b11111111000000000000000000000000;

static int num_bound_ = 200;

const uint32_t Converter::kProfileLookupTable[] = {
    SET_FOUR_BYTE_TAG('m', 'n', 't', 'r'),
    SET_FOUR_BYTE_TAG('s', 'c', 'n', 'r'),
    SET_FOUR_BYTE_TAG('p', 'r', 't', 'r'),
    SET_FOUR_BYTE_TAG('s', 'p', 'a', 'c'),
};

const uint32_t Converter::kInputColorSpaceLookupTable[] = {
    SET_FOUR_BYTE_TAG('R', 'G', 'B', ' '),
    SET_FOUR_BYTE_TAG('C', 'M', 'Y', 'K'),
    SET_FOUR_BYTE_TAG('G', 'R', 'A', 'Y'),
};

const uint32_t Converter::kPCSLookupTable[] = {
    SET_FOUR_BYTE_TAG('X', 'Y', 'Z', ' '),
    SET_FOUR_BYTE_TAG('L', 'a', 'b', ' '),
};

const uint32_t Converter::kTagLookupTable[] = {
    SET_FOUR_BYTE_TAG('r', 'X', 'Y', 'Z'),
    SET_FOUR_BYTE_TAG('g', 'X', 'Y', 'Z'),
    SET_FOUR_BYTE_TAG('b', 'X', 'Y', 'Z'),
    SET_FOUR_BYTE_TAG('r', 'T', 'R', 'C'),
    SET_FOUR_BYTE_TAG('g', 'T', 'R', 'C'),
    SET_FOUR_BYTE_TAG('b', 'T', 'R', 'C'),
    SET_FOUR_BYTE_TAG('k', 'T', 'R', 'C'),
    SET_FOUR_BYTE_TAG('A', '2', 'B', '0'),
    SET_FOUR_BYTE_TAG('c', 'u', 'r', 'v'),
    SET_FOUR_BYTE_TAG('p', 'a', 'r', 'a'),
    SET_FOUR_BYTE_TAG('m', 'l', 'u', 'c'),
};

uint8_t Converter::kColorTableBuffer[kColorTableBufferLength];

// const int Converter::kObjDepthLimit = 3;

Converter::Converter() {}

Converter::~Converter() {}

Converter::Converter(const Converter& other) {
  for (int idx = 0; other.output_.size(); idx++)
    output_.push_back(other.output_[idx]);
}

std::string Converter::FieldToObjName(const std::string& field_name) {
  enum State { WORD_START, IN_WORD, IN_WORD_STARTED_BY_NUM };

  State state = WORD_START;
  std::string obj_name = "Sk";
  const char* field_name_arr = field_name.c_str();
  for (size_t idx = 0; idx < field_name.size(); idx++) {
    const char ch = field_name_arr[idx];
    if (state == WORD_START || state == IN_WORD_STARTED_BY_NUM) {
      if (islower(ch)) {
        obj_name += toupper(ch);
        state = IN_WORD;
      } else if (isdigit(ch)) {
        // This state allows 2d to be converted to 2D.
        obj_name += ch;
        state = IN_WORD_STARTED_BY_NUM;
      } else if (ch != '_') {
        obj_name += ch;
      }
    } else {
      if (ch == '_')
        state = WORD_START;
      else
        obj_name += ch;
    }
  }
  CHECK(state > WORD_START);
  return obj_name;
}

void Converter::Reset() {
  obj_depth_ = 0;
  obj_count_ = 0;
  tag_offset_ = 0;
  output_.clear();
  stroke_style_used_ = false;
}

std::string Converter::Convert(const Filter& filter) {
  Reset();
  nan_substitutor_ = std::mt19937(filter.nan_substitor_seed());
  Visit(filter);
  CheckAlignment();
  if (!output_.size())
    return std::string("");
  return std::string(&output_[0], output_.size());
}

void Converter::Visit(const Filter& filter) {
  Visit(filter.image_filter());
}

// TODO(metzman): Can we get rid of the parent
void Converter::Visit(const LightParent& light_parent) {
  if (light_parent.light_child().has_point_light())
    WriteNum(kPoint_LightType);
  else if (light_parent.light_child().has_spot_light())
    WriteNum(kSpot_LightType);
  else  // Assume we have distant light
    WriteNum(kDistant_LightType);
  Visit(light_parent.light_child());
}

void Converter::Visit(const ImageFilterChild& image_filter_child) {
  VISIT_OPT_OBJ(image_filter_child, specular_lighting_image_filter);
  VISIT_OPT_OBJ(image_filter_child, matrix_image_filter);
  VISIT_OPT_OBJ(image_filter_child, paint_image_filter);
  VISIT_OPT_OBJ(image_filter_child, arithmetic_image_filter_impl);
  VISIT_OPT_OBJ(image_filter_child, alpha_threshold_filter);
  VISIT_OPT_OBJ(image_filter_child, blur_image_filter_impl);
  VISIT_OPT_OBJ(image_filter_child, color_filter_image_filter);
  VISIT_OPT_OBJ(image_filter_child, compose_image_filter);
  VISIT_OPT_OBJ(image_filter_child, displacement_map_effect);
  VISIT_OPT_OBJ(image_filter_child, drop_shadow_image_filter);
  VISIT_OPT_OBJ(image_filter_child, local_matrix_image_filter);
  VISIT_OPT_OBJ(image_filter_child, magnifier_image_filter);
  VISIT_OPT_OBJ(image_filter_child, matrix_convolution_image_filter);
  VISIT_OPT_OBJ(image_filter_child, merge_image_filter);
  VISIT_OPT_OBJ(image_filter_child, dilate_image_filter);
  VISIT_OPT_OBJ(image_filter_child, offset_image_filter);
  VISIT_OPT_OBJ(image_filter_child, picture_image_filter);
  VISIT_OPT_OBJ(image_filter_child, tile_image_filter);
  VISIT_NAMED_OPT_OBJ(image_filter_child, xfermode_image_filter__base,
                      "SkXfermodeImageFilter_Base");
  VISIT_OPT_OBJ(image_filter_child, xfermode_image_filter);
  VISIT_OPT_OBJ(image_filter_child, diffuse_lighting_image_filter);
  VISIT_OPT_OBJ(image_filter_child, image_source);
}

void Converter::Visit(
    const DiffuseLightingImageFilter& diffuse_lighting_image_filter) {
  Visit(diffuse_lighting_image_filter.parent(), 1);
  Visit(diffuse_lighting_image_filter.light());
  WriteNum(diffuse_lighting_image_filter.surface_scale());
  // Can't be negative, see:
  // https://www.w3.org/TR/SVG/filters.html#feDiffuseLightingElement
  const float kd = abs(diffuse_lighting_image_filter.kd());
  WriteNum(kd);
}

void Converter::Visit(const XfermodeImageFilter& xfermode_image_filter) {
  Visit(xfermode_image_filter.parent(), 2);
  WriteNum(xfermode_image_filter.mode());
}

void Converter::Visit(
    const XfermodeImageFilter_Base& xfermode_image_filter__base) {
  Visit(xfermode_image_filter__base.parent(), 2);
  WriteNum(xfermode_image_filter__base.mode());
}

void Converter::Visit(const TileImageFilter& tile_image_filter) {
  Visit(tile_image_filter.parent(), 1);
  WriteFields(tile_image_filter, 2);
}

void Converter::Visit(const OffsetImageFilter& offset_image_filter) {
  Visit(offset_image_filter.parent(), 1);
  Visit(offset_image_filter.offset());
}

void Converter::Visit(const MergeImageFilter& merge_image_filter) {
  Visit(merge_image_filter.parent(), merge_image_filter.parent().inputs_size());
}

void Converter::Visit(const ErodeImageFilter& erode_image_filter) {
  Visit(erode_image_filter.parent(), 1);
  WriteFields(erode_image_filter, 2);
}

template <typename T>
T Converter::BoundNum(T num, int num_bound) {
  if (num >= 0)
    return num % num_bound;
  // Bound negative numbers.
  return num % -num_bound;
}

template <typename T>
T Converter::BoundNum(T num) {
  return BoundNum(num, num_bound_);
}

float Converter::BoundNum(float num) {
  // Don't allow NaNs, they can cause ooms.
  if (std::isnan(num))
    num = GetRandomFloat(&nan_substitutor_);

  if (num >= 0)
    return fmod(num, static_cast<float>(num_bound_));
  // Bound negative numbers.
  return fmod(num, static_cast<float>(-num_bound_));
}

void Converter::Visit(const DilateImageFilter& dilate_image_filter) {
  Visit(dilate_image_filter.parent(), 1);
  WriteFields(dilate_image_filter, 2);
}

void Converter::Visit(
    const MatrixConvolutionImageFilter& matrix_convolution_image_filter) {
  Visit(matrix_convolution_image_filter.parent(), 1);
  // Avoid timeouts from having to generate too many random numbers.
  // TODO(metzman): actually calculate the limit based on this bound (eg 31 x 1
  // probably doesn't need to be bounded).
  const int num_bound = 30;
  const int32_t width =
      BoundNum(abs(matrix_convolution_image_filter.width()), num_bound);

  WriteNum(width);
  const int32_t height =
      BoundNum(abs(matrix_convolution_image_filter.height()), num_bound);

  WriteNum(height);
  std::mt19937 rand_gen(matrix_convolution_image_filter.kernel_seed());
  const uint32_t kernel_size = width * height;
  WriteNum(kernel_size);
  for (uint32_t kernel_counter = 0; kernel_counter < kernel_size;
       kernel_counter++) {
    float kernel_element = GetRandomFloat(&rand_gen);
    WriteNum(kernel_element);
  }
  WriteFields(matrix_convolution_image_filter, 5);
}

void Converter::Visit(const MagnifierImageFilter& magnifier_image_filter) {
  Visit(magnifier_image_filter.parent(), 1);
  Visit(magnifier_image_filter.src());
}

void Converter::Visit(const LocalMatrixImageFilter& local_matrix_image_filter) {
  Visit(local_matrix_image_filter.parent(), 1);
  Visit(local_matrix_image_filter.matrix());
}

void Converter::Visit(const ImageSource& image_source) {
  WriteFields(image_source, 1, 3);
  Visit(image_source.image());
}

void Converter::Visit(const DropShadowImageFilter& drop_shadow_image_filter) {
  Visit(drop_shadow_image_filter.parent(), 1);
  WriteFields(drop_shadow_image_filter, 2);
}

void Converter::Visit(const DisplacementMapEffect& displacement_map_effect) {
  Visit(displacement_map_effect.parent(), 2);
  WriteFields(displacement_map_effect, 2);
}

void Converter::Visit(const ComposeImageFilter& compose_image_filter) {
  Visit(compose_image_filter.parent(), 2);
}

void Converter::Visit(const ColorFilterImageFilter& color_filter_image_filter) {
  Visit(color_filter_image_filter.parent(), 1);
  Visit(color_filter_image_filter.color_filter());
}

void Converter::Visit(const BlurImageFilterImpl& blur_image_filter_impl) {
  Visit(blur_image_filter_impl.parent(), 1);
  WriteFields(blur_image_filter_impl, 2);
}

void Converter::Visit(const AlphaThresholdFilter& alpha_threshold_filter) {
  Visit(alpha_threshold_filter.parent(), 1);
  WriteFields(alpha_threshold_filter, 2, 3);
  VisitObj(alpha_threshold_filter.rgn(), "SkRegion");
}

void Converter::Visit(const Region& region) {
  if (region.is_rect()) {
    WriteNum(0);
    Visit(region.bounds());
  } else if (region.has_region_complex()) {
    WriteNum(region.region_complex().runs_size() + 1);
    Visit(region.bounds());
    WriteNum(region.region_complex().first_run());
    for (auto run : region.region_complex().runs())
      WriteNum(run);
  } else {
    WriteNum(-1);
  }
}

void Converter::WriteIndex(const uint32_t index) {
  // ~Line 266 of SkValidatingReadBuffer.cpp says the first byte must be zero
  // and that the number will be shifted down right by 8 as a result of this.
  CHECK_LE(index, kMax3ByteUInt);
  CHECK_EQ((kMax3ByteUInt >> 8) << 8, kMax3ByteUInt);
  const uint32_t writable_index = index >> 8;
  CHECK_EQ(writable_index << 8, index);
  // Ensure that the first byte is zero.
  CHECK_EQ(kUInt32WithAllLastByteSet & writable_index,
           static_cast<unsigned>(0));
  WriteNum(writable_index);
}

void Converter::MarkNotPresent() {
  WriteIndex(0);
}

void Converter::Visit(const ImageFilterParent& image_filter,
                      const int num_inputs_required) {
  CHECK_GE(num_inputs_required, -1);
  if (!num_inputs_required) {
    WriteNum(0);
  } else if (num_inputs_required == 1) {
    WriteNum(1);
    WriteBool(true);
    Visit(image_filter.default_input());
  } else if (num_inputs_required == -1) {
    WriteNum(image_filter.inputs_size() + 1);
    WriteBool(true);
    Visit(image_filter.default_input());
    for (const auto& input : image_filter.inputs()) {
      WriteBool(true);
      Visit(input);
    }
  } else {
    WriteNum(num_inputs_required);
    WriteBool(true);
    Visit(image_filter.default_input());
    for (const auto& input : image_filter.inputs()) {
      WriteBool(true);
      Visit(input);
    }
  }
  Visit(image_filter.crop_rectangle());
}

void Converter::Visit(
    const ArithmeticImageFilterImpl& arithmetic_image_filter_impl) {
  Visit(arithmetic_image_filter_impl.parent(), 2);
  WriteFields(arithmetic_image_filter_impl, 2);
}

void Converter::Visit(
    const SpecularLightingImageFilter& specular_lighting_image_filter) {
  Visit(specular_lighting_image_filter.image_filter_parent(), 1);
  Visit(specular_lighting_image_filter.light());
  WriteFields(specular_lighting_image_filter, 3, 4);
  WriteNum(specular_lighting_image_filter.surface_scale() * 255);
}

void Converter::RecordSize() {
  // Reserve space to overwrite when we are done writing whaterver size we are
  // recording.
  WriteInt32(0);
  start_sizes_.push_back(output_.size());
}

size_t Converter::PopStartSize() {
  CHECK_GT(start_sizes_.size(), static_cast<size_t>(0));
  const size_t back = start_sizes_.back();
  start_sizes_.pop_back();
  return back;
}

void Converter::WriteInt32(uint32_t num) {
  char num_arr[sizeof(uint32_t)];
  memcpy(num_arr, &num, sizeof(uint32_t));

  for (size_t idx = 0; idx < sizeof(uint32_t); idx++)
    output_.push_back(num_arr[idx]);
}

template <typename T>
void Converter::WriteBigEndian(const T num) {
  CHECK_LE(sizeof(T), static_cast<size_t>(4));
  uint8_t num_arr[sizeof(T)];
  memcpy(num_arr, &num, sizeof(T));
  uint8_t tmp1 = num_arr[0];
  uint8_t tmp2 = num_arr[3];
  num_arr[3] = tmp1;
  num_arr[0] = tmp2;

  tmp1 = num_arr[1];
  tmp2 = num_arr[2];
  num_arr[2] = tmp1;
  num_arr[1] = tmp2;

  for (size_t idx = 0; idx < sizeof(uint32_t); idx++)
    output_.push_back(num_arr[idx]);
}

template <typename T>
void Converter::WriteNum(const T num) {
  CHECK_LE(sizeof(T), static_cast<size_t>(4));
  char num_arr[sizeof(T)];
  memcpy(num_arr, &num, sizeof(T));
  for (size_t idx = 0; idx < sizeof(T); idx++)
    output_.push_back(num_arr[idx]);
}

void Converter::WriteNum(const char (&num_arr)[4]) {
  for (size_t idx = 0; idx < 4; idx++)
    output_.push_back(num_arr[idx]);
}

void Converter::InsertSize(const size_t size, const uint32_t position) {
  char size_arr[sizeof(uint32_t)];
  memcpy(size_arr, &size, sizeof(uint32_t));

  for (size_t idx = 0; idx < sizeof(uint32_t); idx++) {
    const size_t output__idx = position + idx - sizeof(uint32_t);
    CHECK(output__idx < output_.size());
    output_[output__idx] = size_arr[idx];
  }
}

void Converter::WriteBytesWritten() {
  const size_t start_size = PopStartSize();
  CHECK(start_size < std::numeric_limits<uint32_t>::max());
  const size_t end_size = output_.size();
  CHECK(start_size <= end_size);
  const size_t bytes_written = end_size - start_size;
  CHECK(bytes_written < std::numeric_limits<uint32_t>::max());
  InsertSize(bytes_written, start_size);
}

void Converter::WriteString(const std::string str) {
  WriteInt32(str.size());
  const char* c_str = str.c_str();
  for (size_t idx = 0; idx < str.size(); idx++)
    output_.push_back(c_str[idx]);

  output_.push_back('\0');  // Add trailing NULL.

  Pad(str.size() + 1);
}

void Converter::WriteArray(
    const google::protobuf::RepeatedField<uint32_t>& repeated_field,
    const size_t size) {
  WriteInt32(size * sizeof(uint32_t));  // Array size.
  for (uint32_t element : repeated_field)
    WriteInt32(element);
}

void Converter::WriteArray(const char* arr, const size_t size) {
  WriteInt32(size * sizeof(uint32_t));  // Array size.
  for (size_t idx = 0; idx < size; idx++)
    output_.push_back(arr[idx]);

  for (unsigned idx = 0; idx < size % 4; idx++)
    output_.push_back('\0');
}

void Converter::WriteBool(const bool bool_val) {
  WriteNum(static_cast<uint32_t>(bool_val));
}

void Converter::WriteTagSize(const char (&tag)[4], const size_t size) {
  WriteNum(tag);
  WriteNum(static_cast<uint32_t>(size));
}

void Converter::Visit(const google::protobuf::Message& msg) {
  WriteFields(msg);
}

template <class T>
void Converter::Visit(
    const google::protobuf::RepeatedPtrField<T>& repeated_field) {
  for (const T& single_field : repeated_field)
    Visit(single_field);
}

void Converter::Visit(const PictureImageFilter& picture_image_filter) {
  // See comments on PictureImageFilter in filter.proto for why code here is
  // commented out.
  // WriteBool(picture_image_filter.has_picture());
  WriteBool(false);
  // VISIT_OPT(picture_image_filter, picture);
  Visit(picture_image_filter.crop_rectangle());
  // WriteFields(picture_image_filter, 2);
}

void Converter::Visit(const PictureData& picture_data) {
  size_t op_data_size = picture_data.op_data_size();
  WriteTagSize(kSkPictReaderTag, op_data_size);
  WriteArray(picture_data.bytes(), picture_data.bytes_size());
  Visit(picture_data.recording_data());
  WriteNum(kSkPictEOFTag);
}

// Copied from SkPaint.cpp:1805.
static uint32_t pack_4(unsigned a, unsigned b, unsigned c, unsigned d) {
  CHECK(a == (uint8_t)a);
  CHECK(b == (uint8_t)b);
  CHECK(c == (uint8_t)c);
  CHECK(d == (uint8_t)d);
  return (a << 24) | (b << 16) | (c << 8) | d;
}

// Copied from SkPaint.cpp:1843
static uint32_t pack_paint_flags(unsigned flags,
                                 unsigned hint,
                                 unsigned align,
                                 unsigned filter,
                                 unsigned flatFlags) {
  // left-align the fields of "known" size, and right-align the last (flatFlags)
  // so it can easly add more bits in the future.
  return (flags << 16) | (hint << 14) | (align << 12) | (filter << 10) |
         flatFlags;
}

void Converter::Visit(const Typeface& typeface) {
  WriteNum(1);
}

void Converter::Visit(const Paint& paint) {
  WriteFields(paint, 1, 6);

  uint8_t flat_flags = 0;
  // if (paint.has_typeface())
  //   flat_flags |= kHasTypeface_FlatFlag;
  if (paint.has_effects())
    flat_flags |= kHasEffects_FlatFlag;

  WriteNum(pack_paint_flags(paint.flags(), paint.hinting(), paint.align(),
                            paint.filter_quality(), flat_flags));

  int style = paint.style();
  if (stroke_style_used_)
    style = Paint::kFill_Style;
  else if (style == Paint::kStrokeAndFill_Style ||
           style == Paint::kStroke_Style)
    stroke_style_used_ = true;

  uint32_t tmp =
      pack_4(paint.stroke_cap(), paint.stroke_join(),
             (style << 4) | paint.text_encoding(), paint.blend_mode());

  WriteNum(tmp);  // See SkPaint.cpp:1898
  // VISIT_OPT(paint, typeface);
  VISIT_OPT(paint, effects);
}

void Converter::Visit(const PaintEffects& paint_effects) {
  VISIT_OR_NULL(paint_effects, path_effect);
  VISIT_OR_NULL(paint_effects, shader);
  VISIT_OR_NULL(paint_effects, mask_filter);
  VISIT_OR_NULL(paint_effects, color_filter);
  VISIT_OR_NULL(paint_effects, rasterizer);
  VISIT_OR_NULL(paint_effects, looper);
  VISIT_OR_NULL(paint_effects, image_filter);
}

void Converter::Visit(const ColorFilterChild& color_filter_child) {
  VISIT_OPT_OBJ2(color_filter_child, mode_color_filter, "SkModeColorFilter");
  VISIT_OPT_OBJ2(color_filter_child, color_matrix_filter_row_major_255,
                 "SkColorMatrixFilterRowMajor255");

  VISIT_OPT_OBJ2(color_filter_child, compose_color_filter,
                 "SkComposeColorFilter");

  VISIT_NAMED_OPT_OBJ(color_filter_child, srgb_gamma_color_filter,
                      "SkSRGBGammaColorFilter");

  // TODO(metzman): figure out why this isn't deserializable.
  // VISIT_OPT_OBJ2(color_filter_child, gaussian_color_filter,
  //              "SkGaussianColorFilter");

  VISIT_NAMED_OPT_OBJ(color_filter_child, high_contrast__filter,
                      "SkHighContrast_Filter");

  VISIT_OPT_OBJ2(color_filter_child, luma_color_filter, "SkLumaColorFilter");
  VISIT_OPT_OBJ2(color_filter_child, overdraw_color_filter,
                 "SkOverdrawColorFilter");

  VISIT_NAMED_OPT_OBJ(color_filter_child, table__color_filter,
                      "SkTable_ColorFilter");

  VISIT_NAMED_OPT_OBJ(color_filter_child, to_srgb_color_filter,
                      "SkToSRGBColorFilter");
}

void Converter::Visit(const Color4f& color_4f) {
  WriteFields(color_4f);
}

void Converter::Visit(const GradientDescriptor& gradient_descriptor) {
  // See SkGradientShaderBase::Descriptor::flatten in SkGradientShader.cpp.
  enum GradientSerializationFlags {
    // Bits 29:31 used for various boolean flags
    kHasPosition_GSF = 0x80000000,
    kHasLocalMatrix_GSF = 0x40000000,
    kHasColorSpace_GSF = 0x20000000,

    // Bits 12:28 unused

    // Bits 8:11 for fTileMode
    kTileModeShift_GSF = 8,
    kTileModeMask_GSF = 0xF,

    // Bits 0:7 for fGradFlags (note that kForce4fContext_PrivateFlag is 0x80)
    kGradFlagsShift_GSF = 0,
    kGradFlagsMask_GSF = 0xFF,
  };

  uint32_t flags = 0;
  if (gradient_descriptor.has_pos())
    flags |= kHasPosition_GSF;
  if (gradient_descriptor.has_local_matrix())
    flags |= kHasLocalMatrix_GSF;
  if (gradient_descriptor.has_color_space())
    flags |= kHasColorSpace_GSF;
  flags |= (gradient_descriptor.tile_mode() << kTileModeShift_GSF);
  uint32_t grad_flags =
      (gradient_descriptor.grad_flags() % (kGradFlagsMask_GSF + 1));
  CHECK(grad_flags <= kGradFlagsMask_GSF);
  WriteNum(flags);

  const uint32_t count = gradient_descriptor.colors_size();

  WriteNum(count);
  for (auto& color : gradient_descriptor.colors())
    Visit(color);

  Visit(gradient_descriptor.color_space());

  WriteNum(count);
  for (uint32_t counter = 0; counter < count; counter++)
    WriteNum(gradient_descriptor.pos());

  Visit(gradient_descriptor.local_matrix());
}

void Converter::Visit(const GradientParent& gradient_parent) {
  Visit(gradient_parent.gradient_descriptor());
}

void Converter::Visit(const ToSRGBColorFilter& to_srgb_color_filter) {
  Visit(to_srgb_color_filter.color_space());
}

void Converter::Visit(const LooperChild& looper) {
  VISIT_OPT_OBJ2(looper, layer_draw_looper, "SkLayerDrawLooper");
  CHECK(looper.has_layer_draw_looper());
}

// Copied from SkPackBits.cpp.
static uint8_t* flush_diff8(uint8_t* dst, const uint8_t* src, size_t count) {
  while (count > 0) {
    size_t n = count > 128 ? 128 : count;
    *dst++ = (uint8_t)(n + 127);
    memcpy(dst, src, n);
    src += n;
    dst += n;
    count -= n;
  }
  return dst;
}

// Copied from SkPackBits.cpp.
static uint8_t* flush_same8(uint8_t dst[], uint8_t value, size_t count) {
  while (count > 0) {
    size_t n = count > 128 ? 128 : count;
    *dst++ = (uint8_t)(n - 1);
    *dst++ = (uint8_t)value;
    count -= n;
  }
  return dst;
}

// Copied from SkPackBits.cpp.
size_t compute_max_size8(size_t srcSize) {
  // Worst case is the number of 8bit values + 1 byte per (up to) 128 entries.
  return ((srcSize + 127) >> 7) + srcSize;
}

// Copied from SkPackBits.cpp.
static size_t pack8(const uint8_t* src,
                    size_t srcSize,
                    uint8_t* dst,
                    size_t dstSize) {
  if (dstSize < compute_max_size8(srcSize)) {
    return 0;
  }

  uint8_t* const origDst = dst;
  const uint8_t* stop = src + srcSize;

  for (intptr_t count = stop - src; count > 0; count = stop - src) {
    if (1 == count) {
      *dst++ = 0;
      *dst++ = *src;
      break;
    }

    unsigned value = *src;
    const uint8_t* s = src + 1;

    if (*s == value) {  // accumulate same values...
      do {
        s++;
        if (s == stop) {
          break;
        }
      } while (*s == value);
      dst = flush_same8(dst, value, (size_t)(s - src));
    } else {  // accumulate diff values...
      do {
        if (++s == stop) {
          goto FLUSH_DIFF;
        }
        // only stop if we hit 3 in a row,
        // otherwise we get bigger than compuatemax
      } while (*s != s[-1] || s[-1] != s[-2]);
      s -= 2;  // back up so we don't grab the "same" values that follow
    FLUSH_DIFF:
      dst = flush_diff8(dst, src, (size_t)(s - src));
    }
    src = s;
  }
  return dst - origDst;
}

static const uint8_t kCountNibBits[] = {0, 1, 1, 2, 1, 2, 2, 3,
                                        1, 2, 2, 3, 2, 3, 3, 4};

const uint8_t* Converter::ColorTableToArray(const ColorTable& color_table) {
  float* dst = reinterpret_cast<float*>(kColorTableBuffer);
  const int array_size = 64;
  // Now write the 256 fields.
  const Descriptor* descriptor = color_table.GetDescriptor();
  const Reflection* reflection = color_table.GetReflection();
  CHECK(descriptor && reflection);
  for (int field_num = 1; field_num <= array_size; field_num++, dst++) {
    const FieldDescriptor* field_descriptor =
        descriptor->FindFieldByNumber(field_num);
    CHECK(field_descriptor);
    *dst = BoundNum(reflection->GetFloat(color_table, field_descriptor));
  }
  return kColorTableBuffer;
}

void Converter::Visit(const Table_ColorFilter& table__color_filter) {
  enum {
    kA_Flag = 1 << 0,
    kR_Flag = 1 << 1,
    kG_Flag = 1 << 2,
    kB_Flag = 1 << 3,
  };

  // See SkTable_ColorFilter::SkTable_ColorFilter
  unsigned flags = 0;
  uint8_t f_storage[4 * kColorTableBufferLength];
  uint8_t* dst = f_storage;

  if (table__color_filter.has_table_a()) {
    memcpy(dst, ColorTableToArray(table__color_filter.table_a()),
           kColorTableBufferLength);

    dst += kColorTableBufferLength;
    flags |= kA_Flag;
  }
  if (table__color_filter.has_table_r()) {
    memcpy(dst, ColorTableToArray(table__color_filter.table_r()),
           kColorTableBufferLength);

    dst += kColorTableBufferLength;
    flags |= kR_Flag;
  }
  if (table__color_filter.has_table_g()) {
    memcpy(dst, ColorTableToArray(table__color_filter.table_g()),
           kColorTableBufferLength);

    dst += kColorTableBufferLength;
    flags |= kG_Flag;
  }
  if (table__color_filter.has_table_b()) {
    memcpy(dst, ColorTableToArray(table__color_filter.table_b()),
           kColorTableBufferLength);

    dst += kColorTableBufferLength;
    flags |= kB_Flag;
  }
  uint8_t storage[5 * kColorTableBufferLength];
  const int count = kCountNibBits[flags & 0xF];
  const size_t size = pack8(f_storage, count * kColorTableBufferLength, storage,
                            sizeof(storage));

  CHECK(flags <= UINT32_MAX);
  const uint32_t flags_32 = (uint32_t)flags;
  WriteNum(flags_32);
  WriteNum((uint32_t)size);
  for (size_t idx = 0; idx < size; idx++)
    output_.push_back(storage[idx]);
  Pad(output_.size());
}

void Converter::Visit(const ComposeColorFilter& compose_color_filter) {
  Visit(compose_color_filter.outer());
  Visit(compose_color_filter.inner());
}

void Converter::Visit(const OverdrawColorFilter& overdraw_color_filter) {
  const uint32_t num_fields = 6;
  const uint32_t arr_size = num_fields * sizeof(uint32_t);
  WriteNum(arr_size);
  WriteFields(overdraw_color_filter);
}

void Converter::Visit(
    const ColorMatrixFilterRowMajor255& color_matrix_filter_row_major_255) {
  Visit(color_matrix_filter_row_major_255.color_filter_matrix());
}

void Converter::Visit(const ColorFilterMatrix& color_filter_matrix) {
  WriteNum(20);
  WriteFields(color_filter_matrix);
}

void Converter::Visit(const LayerDrawLooper& layer_draw_looper) {
  WriteNum(layer_draw_looper.layer_infos_size());
  for (auto& layer_info : layer_draw_looper.layer_infos()) {
    Visit(layer_info);
  }
}

void Converter::Visit(const LayerInfo& layer_info) {
  WriteNum(0);
  WriteFields(layer_info, 1, 4);
  Visit(layer_info.paint());
}

void Converter::Visit(const PairPathEffect& pair) {
  std::string name;
  if (pair.type() == PairPathEffect::SUM)
    name = "SkSumPathEffect";
  else
    name = "SkComposePathEffect";
  WriteString(name);
  RecordSize();

  Visit(pair.path_effect_1());
  Visit(pair.path_effect_2());

  WriteBytesWritten();  // Object size.
  CheckAlignment();
}

enum SerializationOffsets {
  kType_SerializationShift = 28,
  kDirection_SerializationShift = 26,
  kIsVolatile_SerializationShift = 25,
  kConvexity_SerializationShift = 16,
  kFillType_SerializationShift = 8,
};

// Hack
enum SerializationOffsets2 {
  kLegacyRRectOrOvalStartIdx_SerializationShift = 28,
  kLegacyRRectOrOvalIsCCW_SerializationShift = 27,
  kLegacyIsRRect_SerializationShift = 26,
  kIsFinite_SerializationShift = 25,
  kLegacyIsOval_SerializationShift = 24,
  kSegmentMask_SerializationShift = 0
};

// See SkPathRef::writeToBuffer
void Converter::Visit(const PathRef& path_ref) {
  // Bound segment_mask to avoid timeouts and for proper behavior.
  const int32_t packed =
      (((path_ref.is_finite() & 1) << kIsFinite_SerializationShift) |
       (ToUInt8(path_ref.segment_mask()) << kSegmentMask_SerializationShift));

  WriteNum(packed);
  WriteNum(0);
  std::vector<SkPoint> points;
  if (path_ref.verbs_size()) {
    WriteNum(path_ref.verbs_size() + 1);
    uint32_t num_points = 1;  // The last move will add 1 point.
    uint32_t num_conics = 0;
    for (auto& verb : path_ref.verbs()) {
      switch (verb.value()) {
        case ValidVerb::kMove_Verb:
        case ValidVerb::kLine_Verb:
          num_points += 1;
          break;
        case ValidVerb::kConic_Verb:
          num_conics += 1;
        // fall-through
        case ValidVerb::kQuad_Verb:
          num_points += 2;
          break;
        case ValidVerb::kCubic_Verb:
          num_points += 3;
          break;
        case ValidVerb::kClose_Verb:
          break;
        default:
          NOTREACHED();
      }
    }
    WriteNum(num_points);
    WriteNum(num_conics);
  } else {
    WriteNum(0);
    WriteNum(0);
    WriteNum(0);
  }

  for (auto& verb : path_ref.verbs()) {
    const uint8_t value = verb.value();
    WriteNum(value);
  }
  // verbs must start (they are written backwards) with kMove_Verb (0).
  if (path_ref.verbs_size()) {
    uint8_t value = ValidVerb::kMove_Verb;
    WriteNum(value);
  }

  // Write points
  for (auto& verb : path_ref.verbs()) {
    switch (verb.value()) {
      case ValidVerb::kMove_Verb:
      case ValidVerb::kLine_Verb: {
        Visit(verb.point1());
        AppendSkPoint(points, verb.point1());
        break;
      }
      case ValidVerb::kConic_Verb:
      case ValidVerb::kQuad_Verb: {
        Visit(verb.point1());
        Visit(verb.point2());
        AppendSkPoint(points, verb.point1());
        AppendSkPoint(points, verb.point2());
        break;
      }
      case ValidVerb::kCubic_Verb:
        Visit(verb.point1());
        Visit(verb.point2());
        Visit(verb.point3());
        AppendSkPoint(points, verb.point1());
        AppendSkPoint(points, verb.point2());
        AppendSkPoint(points, verb.point3());
        break;
      default:
        break;
    }
  }
  // Write point of the Move Verb we put at the end.
  if (path_ref.verbs_size()) {
    Visit(path_ref.first_verb().point1());
    AppendSkPoint(points, path_ref.first_verb().point1());
  }

  // Write conic weights.
  for (auto& verb : path_ref.verbs()) {
    if (verb.value() == ValidVerb::kConic_Verb)
      WriteNum(verb.conic_weight());
  }

  SkRect skrect;
  skrect.setBoundsCheck(&points[0], points.size());
  WriteNum(skrect.fLeft);
  WriteNum(skrect.fTop);
  WriteNum(skrect.fRight);
  WriteNum(skrect.fBottom);
}

void Converter::AppendSkPoint(std::vector<SkPoint>& points,
                              const Point& point) {
  SkPoint sk_point;
  sk_point.fX = point.x();
  sk_point.fY = point.y();
  points.push_back(sk_point);
}

void Converter::Visit(const Path& path) {
  enum SerializationVersions {
    kPathPrivFirstDirection_Version = 1,
    kPathPrivLastMoveToIndex_Version = 2,
    kPathPrivTypeEnumVersion = 3,
    kCurrent_Version = 3
  };

  enum FirstDirection {
    kCW_FirstDirection,
    kCCW_FirstDirection,
    kUnknown_FirstDirection,
  };

  int32_t packed = (path.convexity() << kConvexity_SerializationShift) |
                   (path.fill_type() << kFillType_SerializationShift) |
                   (path.first_direction() << kDirection_SerializationShift) |
                   (path.is_volatile() << kIsVolatile_SerializationShift) |
                   kCurrent_Version;

  // TODO(metzman): write as RRect.
  WriteNum(packed);
  WriteNum(path.last_move_to_index());
  Visit(path.path_ref());
  Pad(output_.size());
  CheckAlignment();
}

void Converter::CheckAlignment() {
  CHECK_EQ(output_.size() % 4, static_cast<size_t>(0));
}

void Converter::Visit(const ShaderChild& shader) {
  VISIT_OPT_OBJ2(shader, color_shader, "SkColorShader");
  VISIT_OPT_OBJ2(shader, color_4_shader, "SkColor4Shader");
  VISIT_OPT_OBJ2(shader, color_filter_shader, "SkColorFilterShader");
  VISIT_OPT_OBJ2(shader, image_shader, "SkImageShader");
  VISIT_OPT_OBJ2(shader, compose_shader, "SkComposeShader");
  VISIT_OPT_OBJ2(shader, empty_shader, "SkEmptyShader");
  VISIT_OPT_OBJ2(shader, picture_shader, "SkPictureShader");
  // TODO(metzman): Uncomment when PerlinNoiseShaderImpl is fixed and this code
  // can be run without causing crashes.
  // VISIT_OPT_OBJ(shader, perlin_noise_shader_impl);

  VISIT_OPT_OBJ2(shader, local_matrix_shader, "SkLocalMatrixShader");
  // TODO(metzman): figure out why this won't deserialize on skia side.
  // VISIT_NAMED_OPT_OBJ(shader, _3d_shader, "Sk3DShader");
  VISIT_OPT_OBJ(shader, linear_gradient);
  VISIT_OPT_OBJ(shader, radial_gradient);
  VISIT_OPT_OBJ(shader, sweep_gradient);
  VISIT_OPT_OBJ(shader, two_point_conical_gradient);
}

void Converter::Visit(
    const TwoPointConicalGradient& two_point_conical_gradient) {
  Visit(two_point_conical_gradient.parent());
  WriteFields(two_point_conical_gradient, 2, 5);
}

void Converter::Visit(const LinearGradient& linear_gradient) {
  Visit(linear_gradient.parent());
  WriteFields(linear_gradient, 2, 3);
}

void Converter::Visit(const SweepGradient& sweep_gradient) {
  Visit(sweep_gradient.parent());
  WriteFields(sweep_gradient, 2, 4);
}

void Converter::Visit(const RadialGradient& radial_gradient) {
  Visit(radial_gradient.parent());
  WriteFields(radial_gradient, 2, 3);
}

void Converter::WriteIgnoredFields(const int num_fields) {
  CHECK_GE(num_fields, 1);
  for (int counter = 0; counter < num_fields; counter++)
    WriteNum(0);
}

int32_t Converter::BoundIlluminant(float illuminant, const float bound) {
  while (abs(illuminant) >= 1) {
    illuminant /= 10;
  }
  const float result = bound + 0.01f * illuminant;
  CHECK(abs(bound - result) < .01f);
  return round(result / 1.52587890625e-5f);
}

static constexpr uint8_t kICC_Flag = 1 << 1;
static constexpr size_t kICCTagTableEntrySize = 12;
void Converter::Visit(const ICC& icc) {
  const uint32_t orig_size = output_.size();
  const uint32_t header_size = sizeof(uint8_t) * 4;
  const uint32_t tag_count = icc.tags_size() % 100;
  const uint32_t tags_size = tag_count * kICCTagTableEntrySize;
  const uint32_t profile_size = sizeof(float) * 33 + tags_size;
  const uint32_t size = profile_size + sizeof(profile_size) + header_size;
  WriteNum(size);

  // Header.
  WriteColorSpaceVersion();
  WriteNum(ToUInt8(icc.named()));
  WriteNum(ToUInt8(GammaNamed::kNonStandard_SkGammaNamed));
  WriteNum(kICC_Flag);

  WriteNum(profile_size);
  WriteBigEndian(profile_size);
  WriteIgnoredFields(1);
  uint32_t version = icc.version() % 5;
  version <<= 24;
  WriteBigEndian(version);
  WriteBigEndian(kProfileLookupTable[icc.profile_class()]);
  WriteBigEndian(kInputColorSpaceLookupTable[icc.input_color_space()]);
  WriteBigEndian(kPCSLookupTable[icc.pcs()]);
  WriteIgnoredFields(3);
  WriteBigEndian(SET_FOUR_BYTE_TAG('a', 'c', 's', 'p'));
  WriteIgnoredFields(6);
  WriteBigEndian(icc.rendering_intent());
  WriteBigEndian(BoundIlluminant(icc.illuminant_x(), 0.96420f));
  WriteBigEndian(BoundIlluminant(icc.illuminant_y(), 1.00000f));
  WriteBigEndian(BoundIlluminant(icc.illuminant_z(), 0.82491f));
  WriteIgnoredFields(12);
  // TODO(metzman): Implement Tags (use colorspaceicc)
  WriteBigEndian(tag_count);
  int offset = 0;
  for (uint32_t tag_num = 0; tag_num < tag_count; tag_num++) {
    WriteBigEndian(kTagLookupTable[icc.tags()[tag_num]]);
    WriteBigEndian(offset);
    WriteBigEndian(static_cast<uint32_t>(kICCTagTableEntrySize));
    offset += kICCTagTableEntrySize;
  }
  const unsigned new_size = output_.size();
  CHECK(new_size - orig_size == size + sizeof(size));
}

void Converter::WriteTagHeader(const uint32_t tag, const uint32_t len) {
  WriteBigEndian(kTagLookupTable[tag]);
  WriteBigEndian(tag_offset_);
  WriteBigEndian(len);
  tag_offset_ += 12;
}

static const uint32_t kMatrix_Flag = 1 << 0;

static const uint8_t k0_Version = 0;

void Converter::Visit(const ColorSpace& color_space) {
  if (color_space.has_icc()) {
    Visit(color_space.icc());
  } else if (color_space.has_transfer_fn()) {
    Visit(color_space.transfer_fn());
  } else if (color_space.has_color_space__xyz()) {
    Visit(color_space.color_space__xyz());
  } else {
    Visit(color_space.named());
  }
}

static constexpr uint8_t kTransferFn_Flag = 1 << 3;
void Converter::Visit(const TransferFn& transfer_fn) {
  const size_t size_64 =
      (12 * sizeof(float) + 7 * sizeof(float) + 4 * sizeof(uint8_t));
  CHECK(size_64 < UINT32_MAX);
  WriteNum((uint32_t)size_64);
  // Header
  WriteColorSpaceVersion();
  WriteNum(ToUInt8(transfer_fn.named()));
  WriteNum(ToUInt8(GammaNamed::kNonStandard_SkGammaNamed));
  WriteNum(ToUInt8(kTransferFn_Flag));

  WriteFields(transfer_fn, 2);
}

void Converter::WriteColorSpaceVersion() {
  WriteNum(k0_Version);
}

void Converter::Visit(const ColorSpace_XYZ& color_space__xyz) {
  const uint32_t size = 12 * sizeof(float) + sizeof(uint8_t) * 4;
  WriteNum(size);
  // Header
  WriteColorSpaceVersion();
  WriteNum(ToUInt8(Named::kSRGB_Named));
  WriteNum(ToUInt8(color_space__xyz.gamma_named()));
  WriteNum(ToUInt8(kMatrix_Flag));  // See SkColorSpace.cpp:441

  Visit(color_space__xyz.three_by_four());
}

void Converter::Visit(const ColorSpaceNamed& color_space_named) {
  const uint32_t size = sizeof(uint8_t) * 4;
  WriteNum(size);
  // Header
  WriteColorSpaceVersion();
  WriteNum(ToUInt8(color_space_named.named()));
  WriteNum(ToUInt8(color_space_named.gamma_named()));
  WriteNum(ToUInt8(0));
}

// Copied From SkImageInfo.h
static int SkColorTypeBytesPerPixel(uint8_t ct) {
  static const uint8_t gSize[] = {
      0,  // Unknown
      1,  // Alpha_8
      2,  // RGB_565
      2,  // ARGB_4444
      4,  // RGBA_8888
      4,  // BGRA_8888
      1,  // kGray_8
      8,  // kRGBA_F16
  };
  return gSize[ct];
}

size_t Converter::ComputeMinByteSize(int32_t width,
                                     int32_t height,
                                     ImageInfo::AlphaType alpha_type) {
  width = abs(width);
  height = abs(height);

  if (!height)
    return 0;
  uint32_t bytes_per_pixel = SkColorTypeBytesPerPixel(alpha_type);
  uint64_t bytes_per_row_64 = width * bytes_per_pixel;
  // TODO(metzman): Fail gracefully when untrue.
  CHECK(bytes_per_row_64 <= INT32_MAX);
  int32_t bytes_per_row = bytes_per_row_64;
  size_t num_bytes = (height - 1) * bytes_per_row + bytes_per_pixel * width;
  // TODO(metzman): use SkSafeMath?
  return num_bytes;
}

std::tuple<int32_t, int32_t, int32_t> Converter::GetNumPixelBytes(
    const ImageInfo& image_info,
    int32_t width,
    int32_t height) {
  // Returns a value for pixel bytes that is divisible by four by modifying
  // image_info.width() as needed until the computed min byte size is divisible
  // by four.
  size_t num_bytes_64 =
      ComputeMinByteSize(width, height, image_info.alpha_type());
  CHECK(num_bytes_64 <= INT32_MAX);
  int32_t num_bytes = num_bytes_64;
  bool subtract = (num_bytes >= 5);
  while (num_bytes % 4) {
    if (subtract)
      width -= 1;
    else
      width += 1;
    num_bytes_64 = ComputeMinByteSize(width, height, image_info.alpha_type());
    CHECK(num_bytes_64 <= INT32_MAX);
    num_bytes = num_bytes_64;
  }
  return std::make_tuple(num_bytes, width, height);
}

void Converter::Visit(const Bitmap& bitmap) {
  // These are written outside of the bitmap (in the image) but the values are
  // set to the correct ones for a bitmap.
  WriteNum(1);  // encoded_size
  if (!bitmap.image_info().width() || !bitmap.image_info().height()) {
    WriteNum(0);
    return;
  }
  int32_t width = BoundNum(bitmap.image_info().width());
  int32_t height = BoundNum(bitmap.image_info().height());
  auto result = GetNumPixelBytes(bitmap.image_info(), width, height);

  int32_t num_bytes = std::get<0>(result);
  width = std::get<1>(result);
  height = std::get<2>(result);

  WriteNum(num_bytes);
  Visit(bitmap.image_info(), width, height);
  std::mt19937 rand_gen(bitmap.pixel_seed());
  WriteNum(num_bytes);
  for (int byte_num = 1; byte_num <= num_bytes; byte_num++) {
    uint8_t b = rand_gen();
    WriteNum(b);
  }
  WriteBool(false);
}

void Converter::Visit(const ImageInfo& image_info,
                      const int32_t width,
                      const int32_t height) {
  WriteNum(width);
  WriteNum(height);
  uint32_t packed = (image_info.alpha_type() << 8) | image_info.color_type();
  WriteNum(packed);
  Visit(image_info.color_space());
}

void Converter::Visit(const ImageData& image_data) {
  WriteNum(4 * image_data.data_size());
  for (uint32_t element : image_data.data())
    WriteNum(element);
}

void Converter::Visit(const Image& image) {
  // Width and height must be greater than 0.
  const uint32_t width = BoundNum(abs(image.width()));
  if (width == 0)
    WriteNum(1);
  else
    WriteNum(width);
  const uint32_t height = BoundNum(abs(image.width()));
  if (height == 0)
    WriteNum(1);
  else
    WriteNum(height);

  if (image.has_bitmap()) {
    Visit(image.bitmap());
    return;
  } else {
    Visit(image.data());
  }
  // origin_x and origin_y need to be positive.
  WriteNum(abs(image.origin_x()));
  WriteNum(abs(image.origin_y()));
}

void Converter::Visit(const ImageShader& image_shader) {
  WriteFields(image_shader, 1, 3);
  Visit(image_shader.image());
}

void Converter::Visit(const _3DShader& _3d_shader) {
  Visit(_3d_shader.proxy());
}

void Converter::Visit(const RasterizerChild& rasterizer) {
  VISIT_OPT_OBJ2(rasterizer, layer_rasterizer, "SkLayerRasterizer");
  CHECK(rasterizer.has_layer_rasterizer());
}

void Converter::Visit(const LayerRasterizer& layer_rasterizer) {
  WriteNum(layer_rasterizer.layers_size());
  for (auto& layer : layer_rasterizer.layers()) {
    Visit(layer.paint());
    Visit(layer.point());
  }
}

void Converter::Visit(const ColorFilterShader& color_filter_shader) {
  Visit(color_filter_shader.shader());
  Visit(color_filter_shader.filter());
}

void Converter::Visit(const ComposeShader& compose_shader) {
  Visit(compose_shader.dst());
  Visit(compose_shader.src());
  WriteFields(compose_shader, 3, 4);
}

void Converter::Visit(const LocalMatrixShader& local_matrix_shader) {
  Visit(local_matrix_shader.matrix());
  Visit(local_matrix_shader.proxy_shader());
}

void Converter::Visit(const PictureShader& picture_shader) {
  WriteFields(picture_shader, 1, 4);
  WriteBool(false);
}

void Converter::Visit(const Color4Shader& color_4_shader) {
  WriteNum(color_4_shader.color());
  // TOOO(metzman): Implement colorspaces when skia does. See
  // ~SkColor4Shader.cpp:143.
  WriteBool(false);
}

void Converter::Pad(const size_t write_size) {
  if (write_size % 4 == 0)
    return;
  for (size_t padding_count = 0; (padding_count + write_size) % 4 != 0;
       padding_count++)
    output_.push_back('\0');
}

void Converter::Visit(const Path1DPathEffect& path_1d_path_effect) {
  WriteNum(path_1d_path_effect.advance());
  if (path_1d_path_effect.advance()) {
    Visit(path_1d_path_effect.path());
    WriteFields(path_1d_path_effect, 3, 4);
  }
}

template <class T>
void Converter::VisitObj(const T& skia_object, const std::string& object_name) {
  // if (obj_depth_ > kObjDepthLimit)
  //   return;
  obj_depth_ += 1;
  obj_count_ += 1;

  WriteString(object_name);
  RecordSize();
  Visit(skia_object);
  WriteBytesWritten();  // Object size.
  CheckAlignment();
  obj_depth_ -= 1;
}

void Converter::Visit(const DashImpl& dash_impl) {
  WriteNum(dash_impl.phase());
  WriteInt32(dash_impl.intervals_size());
  for (auto interval : dash_impl.intervals())
    WriteNum(interval);
}

void Converter::Visit(const Path2DPathEffect& path_2d_path_effect) {
  Visit(path_2d_path_effect.matrix());
  Visit(path_2d_path_effect.path());
}

void Converter::Visit(const PathEffectChild& path_effect) {
  // Visit(pair_path_effect) will call VisitObj with the correct name.
  VISIT_OPT(path_effect, pair_path_effect);
  VISIT_OPT_OBJ2(path_effect, path_2d_path_effect, "SkPath2DPathEffect");
  VISIT_OPT_OBJ2(path_effect, line_2d_path_effect, "SkLine2DPathEffect");
  VISIT_OPT_OBJ2(path_effect, corner_path_effect, "SkCornerPathEffect");
  VISIT_OPT_OBJ2(path_effect, dash_impl, "SkDashImpl");
  VISIT_OPT_OBJ2(path_effect, discrete_path_effect, "SkDiscretePathEffect");
  VISIT_OPT_OBJ2(path_effect, path_1d_path_effect, "SkPath1DPathEffect");
}

void Converter::Visit(const DiscretePathEffect& discrete_path_effect) {
  // float seg_length = discrete_path_effect.seg_length();
  // while (0 < seg_length && seg_length < 1)
  //   seg_length *= 10;
  // WriteNum(seg_length);

  // Don't write seg_length because it causes too many timeouts.
  WriteNum(0);
  WriteNum(discrete_path_effect.perterb());
  WriteNum(discrete_path_effect.seed_assist());
}

// void Converter::Visit(const TableMaskFilter& table_mask_filter) {
//   // Write this message as an array of uint8_ts.
//   // Start with the length.
//   const int32_t array_size = 256;
//   WriteNum(array_size);

//   // Now write the 256 fields.
//   const Descriptor* descriptor = table_mask_filter.GetDescriptor();
//   const Reflection* reflection = table_mask_filter.GetReflection();
//   CHECK(descriptor && reflection);

//   for (int field_num = 1; field_num <= array_size; field_num++) {
//     const FieldDescriptor* field_descriptor =
//         descriptor->FindFieldByNumber(field_num);
//     CHECK(field_descriptor);
//     uint32_t uint32_val =
//         BoundNum(reflection->GetUInt32(table_mask_filter, field_descriptor));
//     uint8_t uint8_val = ToUInt8(uint32_val);
//     WriteNum(uint8_val);
//   }
// }

void Converter::Visit(const MaskFilterChild& mask_filter) {
  // Default.
  VISIT_OPT_OBJ2(mask_filter, blur_mask_filter_impl, "SkBlurMaskFilterImpl");
  VISIT_OPT_OBJ2(mask_filter, emboss_mask_filter, "SkEmbossMaskFilter");
  VISIT_OPT_OBJ(mask_filter, r_rects_gaussian_edge_mask_filter_impl);
  // TODO(metzman): Figure out why SkTableMaskFilter can't be unflattened
  // (type isn't found).
  // VISIT_OPT_OBJ(mask_filter, table_mask_filter);
}

template <typename T>
uint8_t Converter::ToUInt8(const T input_num) {
  return input_num % (UINT8_MAX + 1);
}

void Converter::Visit(const EmbossMaskFilterLight& emboss_mask_filter_light) {
  // This is written as a byte array, so first write its size, direction_* are
  // floats, fPad is uint16_t and ambient and specular are uint8_ts.
  const uint32_t byte_array_size =
      (3 * sizeof(float) + sizeof(uint16_t) + (2 * sizeof(uint8_t)));
  WriteNum(byte_array_size);
  WriteFields(emboss_mask_filter_light, 3);
  const uint16_t pad = 0;
  WriteNum(pad);  // fPad = 0;
  WriteNum(ToUInt8(emboss_mask_filter_light.ambient()));
  WriteNum(ToUInt8(emboss_mask_filter_light.specular()));
}

void Converter::Visit(const EmbossMaskFilter& emboss_mask_filter) {
  Visit(emboss_mask_filter.light());
  WriteNum(emboss_mask_filter.blur_sigma());
}

void Converter::Visit(const RecordingData& recording_data) {
  WriteNum(kSkPictReaderTag);
  Visit(recording_data.paints());
}

void Converter::Visit(const Picture& picture) {
  // Write header stuff.
  WriteArray(kPictureMagicString, 8);
  WriteFields(picture.header(), 1, 2);
  // WriteNum(picture.header().version());
  // Visit(picture.header().rectangle());

  // Write flags.
  uint32_t flags = kCrossProcess_Flag;
  flags |= kScalarIsFloat_Flag;
  if (picture.header().is_ptr_64_bit())
    flags |= kPtrIs64Bit_Flag;
  WriteNum(flags);

  if (picture.has_backport()) {
    WriteBool(true);
    Visit(picture.backport());
  } else {
    WriteBool(false);
  }
}

void Converter::Visit(const Matrix& matrix) {
  // Avoid OOMs by making sure that matrix fields aren't tiny fractions.
  WriteMatrixField(matrix.val1());
  WriteMatrixField(matrix.val2());
  WriteMatrixField(matrix.val3());
  WriteMatrixField(matrix.val4());
  WriteMatrixField(matrix.val5());
  WriteMatrixField(matrix.val6());
  WriteMatrixField(matrix.val7());
  WriteMatrixField(matrix.val8());
  WriteMatrixField(matrix.val9());
}

void Converter::Visit(const MatrixImageFilter& matrix_image_filter) {
  Visit(matrix_image_filter.image_filter_parent(), 1);
  Visit(matrix_image_filter.transform());
  WriteNum(matrix_image_filter.filter_quality());
}

void Converter::WriteMatrixField(float field_value) {
  // Don't let the field values be tiny fractions.
  field_value = BoundNum(field_value);
  while ((field_value > 0 && field_value < 1e-5) ||
         (field_value < 0 && field_value > -1e-5))
    field_value /= 10.0;
  WriteNum(field_value);
}

void Converter::Visit(const PaintImageFilter& paint_image_filter) {
  Visit(paint_image_filter.image_filter_parent(), 0);
  Visit(paint_image_filter.paint());
}

float Converter::GetRandomFloat(std::mt19937* gen) {
  CHECK(gen);
  const float positive_random_float = static_cast<float>((*gen)());
  const bool is_negative = (*gen)() % 2 == 1;
  if (is_negative)
    return -positive_random_float;
  return positive_random_float;
}

void Converter::WriteFields(const Message& msg,
                            const unsigned start,
                            const unsigned end) {
  CHECK(start >= 1 && end >= 0 && (start <= end || end == 0));
  const Descriptor* descriptor = msg.GetDescriptor();
  const Reflection* reflection = msg.GetReflection();
  CHECK(descriptor && reflection);
  int field_count = descriptor->field_count();
  CHECK(end <= static_cast<unsigned>(field_count));
  bool write_until_last = end == 0;
  unsigned last_field_to_write = write_until_last ? field_count : end;

  for (auto field_num = start; field_num <= last_field_to_write; field_num++) {
    const FieldDescriptor* field_descriptor =
        descriptor->FindFieldByNumber(field_num);
    CHECK(field_descriptor);
    if (field_descriptor->is_repeated()) {
      const auto& tp = field_descriptor->cpp_type();
      switch (tp) {
        case FieldDescriptor::CPPTYPE_UINT32: {
          const size_t num_elements =
              reflection->FieldSize(msg, field_descriptor);
          for (size_t idx = 0; idx < num_elements; idx++) {
            WriteNum(reflection->GetRepeatedUInt32(msg, field_descriptor, idx));
          }
          break;
        }
        case FieldDescriptor::CPPTYPE_FLOAT: {
          const size_t num_elements =
              reflection->FieldSize(msg, field_descriptor);
          for (size_t idx = 0; idx < num_elements; idx++) {
            WriteNum(reflection->GetRepeatedFloat(msg, field_descriptor, idx));
          }
          break;
        }
        case FieldDescriptor::CPPTYPE_MESSAGE: {
          Visit(reflection->GetRepeatedPtrField<google::protobuf::Message>(
              msg, field_descriptor));
          break;
        }
        default: { NOTREACHED(); }
      }
      continue;
    } else if (!field_descriptor->is_required() &&
               !reflection->HasField(msg, field_descriptor)) {
      continue;
    }

    const auto& tp = field_descriptor->cpp_type();
    switch (tp) {
      case FieldDescriptor::CPPTYPE_INT32:
        WriteNum(BoundNum(reflection->GetInt32(msg, field_descriptor)));
        break;
      // case CPPTYPE_INT64:
      //   Write(reflection->GetInt64(field_descriptor));
      //   break;
      case FieldDescriptor::CPPTYPE_UINT32:
        WriteNum(BoundNum(reflection->GetUInt32(msg, field_descriptor)));
        break;
      // case FieldDescriptor::CPPTYPE_UINT64:
      //   Write(reflection->GetUInt64(msg, field_descriptor));
      //   break;
      // case FieldDescriptor::CPPTYPE_DOUBLE:
      //   Write(reflection->GetDouble(msg, field_descriptor));
      //   break;
      case FieldDescriptor::CPPTYPE_FLOAT: {
        WriteNum(BoundNum(reflection->GetFloat(msg, field_descriptor)));
        break;
      }
      case FieldDescriptor::CPPTYPE_BOOL:
        WriteBool(reflection->GetBool(msg, field_descriptor));
        break;
      case FieldDescriptor::CPPTYPE_ENUM:
        WriteNum(reflection->GetEnumValue(msg, field_descriptor));
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        WriteString(reflection->GetString(msg, field_descriptor));
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        Visit(reflection->GetMessage(msg, field_descriptor));
        break;
      default:
        NOTREACHED();
    }
  }
}

};  // namespace filter_proto_converter
