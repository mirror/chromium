// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi_jpeg_encoder.h"

#include <stddef.h>
#include <string.h>
#include <array>

#include "base/logging.h"
#include "base/macros.h"
#include "media/filters/jpeg_parser.h"

namespace media {

namespace {

static const size_t DCTSIZE2 = 64;
static const size_t NUM_DC_RUN_SIZE_BITS = 16;
static const size_t NUM_AC_RUN_SIZE_BITS = 16;
static const size_t NUM_DC_CODE_WORDS_HUFFVAL = 12;
static const size_t NUM_AC_CODE_WORDS_HUFFVAL = 162;
static const size_t JPEG_HEADER_SIZE =
    83 + (DCTSIZE2 * 2) + (NUM_DC_RUN_SIZE_BITS * 2) +
    (NUM_DC_CODE_WORDS_HUFFVAL * 2) + (NUM_AC_RUN_SIZE_BITS * 2) +
    (NUM_AC_CODE_WORDS_HUFFVAL * 2);

static const std::array<uint8_t, 64> kZigZag8x8 = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

static const JpegQuantizationTable kDefaultQuantTable[2] = {
    // Table K.1 Luminance quantization table values.
    {
        true,
        {16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
         14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
         18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
         49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99},
    },
    // Table K.2 Chrominance quantization table values.
    {
        true,
        {17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
         24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
         99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99},
    },
};

}  // namespace

typedef std::array<uint8_t, JPEG_HEADER_SIZE> JPEGHeader;

static void FillPictureParameters(
    const scoped_refptr<media::VideoFrame>& video_frame,
    int quality,
    VABufferID output_buffer_id,
    VAEncPictureParameterBufferJPEG* pic_param) {
  memset(pic_param, 0, sizeof(*pic_param));
  pic_param->picture_width = video_frame->coded_size().width();
  pic_param->picture_height = video_frame->coded_size().height();
  pic_param->num_components = 3;

  // Output buffer.
  pic_param->coded_buf = output_buffer_id;

  // TODO(shenghao): Using quality other than 50 needs to adjust QMatrix
  // accordingly, or let libva fill QMatrix.
  pic_param->quality = 50;
  // Profile = Baseline.
  pic_param->pic_flags.bits.profile = 0;
  // Sequential encoding.
  pic_param->pic_flags.bits.progressive = 0;
  // Uses Huffman coding.
  pic_param->pic_flags.bits.huffman = 1;
  // Input format is interleaved (YUV).
  pic_param->pic_flags.bits.interleaved = 0;
  // Non-differential Encoding.
  pic_param->pic_flags.bits.differential = 0;
  // Only 8 bit sample depth is currently supported.
  pic_param->sample_bit_depth = 8;
  pic_param->num_scan = 1;
}

static void FillQMatrix(VAQMatrixBufferJPEG* q_matrix) {
  memset(q_matrix, 0, sizeof(*q_matrix));

  const JpegQuantizationTable& luminance = kDefaultQuantTable[0];
  q_matrix->load_lum_quantiser_matrix = 1;
  for (size_t i = 0; i < DCTSIZE2; i++) {
    q_matrix->lum_quantiser_matrix[i] = luminance.value[kZigZag8x8[i]];
  }

  const JpegQuantizationTable& chrominance = kDefaultQuantTable[1];
  q_matrix->load_chroma_quantiser_matrix = 1;
  for (size_t i = 0; i < DCTSIZE2; i++) {
    q_matrix->chroma_quantiser_matrix[i] = chrominance.value[kZigZag8x8[i]];
  }
}

static void FillHuffTableParameters(
    VAHuffmanTableBufferJPEGBaseline* huff_table_param) {
  memset(huff_table_param, 0, sizeof(*huff_table_param));

  for (size_t i = 0; i < kJpegMaxHuffmanTableNumBaseline; ++i) {
    const JpegHuffmanTable& dcTable = kDefaultDcTable[i];
    const JpegHuffmanTable& acTable = kDefaultAcTable[i];
    huff_table_param->load_huffman_table[i] = true;

    // Load DC Table.
    memcpy(huff_table_param->huffman_table[i].num_dc_codes,
           &dcTable.code_length[0],
           sizeof(huff_table_param->huffman_table[i].num_dc_codes));
    memcpy(huff_table_param->huffman_table[i].dc_values, &dcTable.code_value[0],
           sizeof(huff_table_param->huffman_table[i].dc_values));

    // Load AC Table.
    memcpy(huff_table_param->huffman_table[i].num_ac_codes,
           &acTable.code_length[0],
           sizeof(huff_table_param->huffman_table[i].num_ac_codes));
    memcpy(huff_table_param->huffman_table[i].ac_values, &acTable.code_value[0],
           sizeof(huff_table_param->huffman_table[i].ac_values));

    memset(huff_table_param->huffman_table[i].pad, 0,
           sizeof(huff_table_param->huffman_table[i].pad));
  }
}

static void FillSliceParameters(
    const scoped_refptr<media::VideoFrame>& video_frame,
    VAEncSliceParameterBufferJPEG* slice_param) {
  memset(slice_param, 0, sizeof(*slice_param));
  slice_param->restart_interval = 0;
  slice_param->num_components = 3;

  slice_param->components[0].component_selector = 1;
  slice_param->components[0].dc_table_selector = 0;
  slice_param->components[0].ac_table_selector = 0;

  slice_param->components[1].component_selector = 2;
  slice_param->components[1].dc_table_selector = 1;
  slice_param->components[1].ac_table_selector = 1;

  slice_param->components[2].component_selector = 3;
  slice_param->components[2].dc_table_selector = 1;
  slice_param->components[2].ac_table_selector = 1;
}

static unsigned int FillJpegHeader(
    const scoped_refptr<media::VideoFrame>& video_frame,
    JPEGHeader& header) {
  unsigned int width = video_frame->coded_size().width();
  unsigned int height = video_frame->coded_size().height();

  size_t idx = 0;

  // Start Of Input.
  header[idx] = 0xFF;
  header[++idx] = JPEG_SOI;

  // Application Segment - JFIF standard 1.01.
  // TODO(shenghao): Use Exif (JPEG_APP1) instead.
  header[++idx] = 0xFF;
  header[++idx] = JPEG_APP0;
  header[++idx] = 0x00;
  header[++idx] = 0x10;  // Segment length:16 (2-byte).
  header[++idx] = 0x4A;  // J
  header[++idx] = 0x46;  // F
  header[++idx] = 0x49;  // I
  header[++idx] = 0x46;  // F
  header[++idx] = 0x00;  // 0
  header[++idx] = 0x01;  // Major version.
  header[++idx] = 0x01;  // Minor version.
  header[++idx] = 0x01;  // Density units 0:no units, 1:pixels per inch,
                         // 2: pixels per cm.
  header[++idx] = 0x00;
  header[++idx] = 0x48;  // X density (2-byte).
  header[++idx] = 0x00;
  header[++idx] = 0x48;  // Y density (2-byte).
  header[++idx] = 0x00;  // Thumbnail width.
  header[++idx] = 0x00;  // Thumbnail height.

  // Quantization Tables.
  for (size_t i = 0; i < 2; ++i) {
    const JpegQuantizationTable& quant_table = kDefaultQuantTable[i];
    header[++idx] = 0xFF;
    header[++idx] = JPEG_DQT;
    header[++idx] = 0x00;
    header[++idx] = 0x03 + DCTSIZE2;  // Segment length:67 (2-byte).

    // Precision (4-bit high) = 0, Index (4-bit low) = i.
    header[++idx] = static_cast<uint8_t>(i);

    for (size_t j = 0; j < DCTSIZE2; ++j) {
      header[++idx] = quant_table.value[kZigZag8x8[j]];
    }
  }

  // Start of Frame - Baseline.
  header[++idx] = 0xFF;
  header[++idx] = JPEG_SOF0;  // Baseline.
  header[++idx] = 0x00;
  header[++idx] = 0x11;  // Segment length:17 (2-byte).
  header[++idx] = 8;     // Data precision.
  header[++idx] = static_cast<uint8_t>((height >> 8) & 0xFF);
  header[++idx] = static_cast<uint8_t>(height & 0xFF);
  header[++idx] = static_cast<uint8_t>((width >> 8) & 0xFF);
  header[++idx] = static_cast<uint8_t>(width & 0xFF);
  header[++idx] = 0x03;  // Number of Components.
  for (uint8_t i = 0; i < 3; ++i) {
    // These are the values for U and V planes.
    uint8_t h_sample_factor = 1;
    uint8_t v_sample_factor = 1;
    uint8_t quant_table_number = 1;
    if (i == 0) {
      // These are the values for Y plane.
      h_sample_factor = 2;
      v_sample_factor = 2;
      quant_table_number = 0;
    }

    header[++idx] = i + 1;
    // Horizontal Sample Factor (4-bit high),
    // Vertical Sample Factor (4-bit low).
    header[++idx] = (h_sample_factor << 4) | v_sample_factor;
    header[++idx] = quant_table_number;
  }

  // Huffman Tables.
  for (size_t i(0); i < 2; ++i) {
    // DC Table.
    const JpegHuffmanTable& dcTable = kDefaultDcTable[i];
    header[++idx] = 0xFF;
    header[++idx] = JPEG_DHT;
    header[++idx] = 0x00;
    header[++idx] = 0x1F;  // Segment length:31 (2-byte).

    // Type (4-bit high) = 0:DC, Index (4-bit low).
    header[++idx] = static_cast<uint8_t>(i);

    for (size_t j(0); j < NUM_DC_RUN_SIZE_BITS; ++j)
      header[++idx] = dcTable.code_length[j];
    for (size_t j(0); j < NUM_DC_CODE_WORDS_HUFFVAL; ++j)
      header[++idx] = dcTable.code_value[j];

    // AC Table.
    const JpegHuffmanTable& acTable = kDefaultAcTable[i];
    header[++idx] = 0xFF;
    header[++idx] = JPEG_DHT;
    header[++idx] = 0x00;
    header[++idx] = 0xB5;  // Segment length:181 (2-byte).

    // Type (4-bit high) = 1:AC, Index (4-bit low).
    header[++idx] = 0x10 | static_cast<uint8_t>(i);

    for (size_t j(0); j < NUM_AC_RUN_SIZE_BITS; ++j)
      header[++idx] = acTable.code_length[j];
    for (size_t j(0); j < NUM_AC_CODE_WORDS_HUFFVAL; ++j)
      header[++idx] = acTable.code_value[j];
  }

  // Start of Scan.
  header[++idx] = 0xFF;
  header[++idx] = JPEG_SOS;
  header[++idx] = 0x00;
  header[++idx] = 0x0C;  // Segment Length:12 (2-byte).
  header[++idx] = 0x03;  // Number of components in scan.
  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t dc_table_number = 1;
    uint8_t ac_table_number = 1;
    if (i == 0) {
      dc_table_number = 0;
      ac_table_number = 0;
    }

    header[++idx] = i + 1;
    // DC Table Selector (4-bit high), AC Table Selector (4-bit low).
    header[++idx] = (dc_table_number << 4) | ac_table_number;
  }
  header[++idx] = 0x00;  // 0 for Baseline.
  header[++idx] = 0x3F;  // 63 for Baseline.
  header[++idx] = 0x00;  // 0 for Baseline.

  return ++idx << 3;
}

// static
size_t VaapiJpegEncoder::GetMaxCodedBufferSize(int width, int height) {
  return width * height * 3 / 2 + JPEG_HEADER_SIZE;
}

// static
bool VaapiJpegEncoder::Encode(
    VaapiWrapper* vaapi_wrapper,
    const scoped_refptr<media::VideoFrame>& video_frame,
    int quality,
    VASurfaceID surface_id,
    VABufferID output_buffer_id) {
  DCHECK_NE(surface_id, VA_INVALID_SURFACE);

  // Set picture parameters.
  VAEncPictureParameterBufferJPEG pic_param;
  FillPictureParameters(video_frame, quality, output_buffer_id, &pic_param);
  if (!vaapi_wrapper->SubmitBuffer(VAEncPictureParameterBufferType,
                                   sizeof(pic_param), &pic_param))
    return false;

  // TODO(shenghao): may need to set QMatrix and HuffTable.

  VAQMatrixBufferJPEG q_matrix;
  FillQMatrix(&q_matrix);
  if (!vaapi_wrapper->SubmitBuffer(VAQMatrixBufferType, sizeof(q_matrix),
                                   &q_matrix))
    return false;

  VAHuffmanTableBufferJPEGBaseline huff_table_param;
  FillHuffTableParameters(&huff_table_param);
  if (!vaapi_wrapper->SubmitBuffer(VAHuffmanTableBufferType,
                                   sizeof(huff_table_param), &huff_table_param))
    return false;

  // Set slice parameters.
  VAEncSliceParameterBufferJPEG slice_param;
  FillSliceParameters(video_frame, &slice_param);
  if (!vaapi_wrapper->SubmitBuffer(VAEncSliceParameterBufferType,
                                   sizeof(slice_param), &slice_param))
    return false;

  JPEGHeader header_data;
  unsigned int length_in_bits = FillJpegHeader(video_frame, header_data);

  VAEncPackedHeaderParameterBuffer header_param;
  memset(&header_param, 0, sizeof(header_param));
  header_param.type = VAEncPackedHeaderRawData;
  header_param.bit_length = length_in_bits;
  header_param.has_emulation_bytes = 0;
  if (!vaapi_wrapper->SubmitBuffer(VAEncPackedHeaderParameterBufferType,
                                   sizeof(header_param), &header_param))
    return false;

  if (!vaapi_wrapper->SubmitBuffer(VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8,
                                   header_data.data()))
    return false;

  // Submit the |surface_id| which contains input YUV frame and begin encoding.
  if (!vaapi_wrapper->ExecuteAndDestroyPendingBuffers(surface_id))
    return false;

  return true;
}

}  // namespace media
