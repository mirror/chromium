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
namespace vaapi_jpeg_encoder {

namespace {

// JPEG header only uses 2 bytes to represent width and height.
const int kMaxDimension = 65535;
const size_t kDctSize2 = 64;
const size_t kNumDcRunSizeBits = 16;
const size_t kNumAcRunSizeBits = 16;
const size_t kNumDcCodeWordsHuffVal = 12;
const size_t kNumAcCodeWordsHuffVal = 162;
const size_t kJpegHeaderSize = 83 + (kDctSize2 * 2) + (kNumDcRunSizeBits * 2) +
                               (kNumDcCodeWordsHuffVal * 2) +
                               (kNumAcRunSizeBits * 2) +
                               (kNumAcCodeWordsHuffVal * 2);

const uint8_t kZigZag8x8[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

const JpegQuantizationTable kDefaultQuantTable[2] = {
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

using JPEGHeader = uint8_t[kJpegHeaderSize];

void FillPictureParameters(const gfx::Size& input_size,
                           int quality,
                           VABufferID output_buffer_id,
                           VAEncPictureParameterBufferJPEG* pic_param) {
  pic_param->picture_width = input_size.width();
  pic_param->picture_height = input_size.height();
  pic_param->num_components = 3;

  // Output buffer.
  pic_param->coded_buf = output_buffer_id;
  pic_param->quality = quality;
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

void FillQMatrix(VAQMatrixBufferJPEG* q_matrix) {
  // Fill the raw, unscaled quantization tables for libva. The VAAPI driver is
  // responsible for scaling the quantization tables based on picture
  // parameter quality.
  const JpegQuantizationTable& luminance = kDefaultQuantTable[0];
  q_matrix->load_lum_quantiser_matrix = 1;
  for (size_t i = 0; i < kDctSize2; i++) {
    q_matrix->lum_quantiser_matrix[i] = luminance.value[kZigZag8x8[i]];
  }

  const JpegQuantizationTable& chrominance = kDefaultQuantTable[1];
  q_matrix->load_chroma_quantiser_matrix = 1;
  for (size_t i = 0; i < kDctSize2; i++) {
    q_matrix->chroma_quantiser_matrix[i] = chrominance.value[kZigZag8x8[i]];
  }
}

void FillHuffTableParameters(
    VAHuffmanTableBufferJPEGBaseline* huff_table_param) {
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

void FillSliceParameters(VAEncSliceParameterBufferJPEG* slice_param) {
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

size_t FillJpegHeader(const gfx::Size& input_size,
                      int quality,
                      JPEGHeader& header) {
  unsigned int width = input_size.width();
  unsigned int height = input_size.height();

  size_t idx = 0;

  // Start Of Input.
  static const uint8_t kSOI[] = {0xFF, JPEG_SOI};
  memcpy(header, kSOI, sizeof(kSOI));
  idx += sizeof(kSOI);

  // Application Segment - JFIF standard 1.01.
  // TODO(shenghao): Use Exif (JPEG_APP1) instead.
  static const uint8_t kAppSegment[] = {
      0xFF, JPEG_APP0, 0x00,
      0x10,  // Segment length:16 (2-byte).
      0x4A,  // J
      0x46,  // F
      0x49,  // I
      0x46,  // F
      0x00,  // 0
      0x01,  // Major version.
      0x01,  // Minor version.
      0x01,  // Density units 0:no units, 1:pixels per inch,
             // 2: pixels per cm.
      0x00,
      0x48,  // X density (2-byte).
      0x00,
      0x48,  // Y density (2-byte).
      0x00,  // Thumbnail width.
      0x00   // Thumbnail height.
  };
  memcpy(header + idx, kAppSegment, sizeof(kAppSegment));
  idx += sizeof(kAppSegment);

  // Normalize quality factor.
  // Unlike VAQMatrixBufferJPEG, we have to scale quantization table in JPEG
  // header by ourselves.
  uint32_t quality_normalized = static_cast<uint32_t>(
      (quality < 50) ? (5000 / quality) : (200 - (quality * 2)));

  // Quantization Tables.
  for (size_t i = 0; i < 2; ++i) {
    const uint8_t kQuantSegment[] = {
        0xFF, JPEG_DQT, 0x00,
        0x03 + kDctSize2,        // Segment length:67 (2-byte).
        static_cast<uint8_t>(i)  // Precision (4-bit high) = 0,
                                 // Index (4-bit low) = i.
    };
    memcpy(header + idx, kQuantSegment, sizeof(kQuantSegment));
    idx += sizeof(kQuantSegment);

    const JpegQuantizationTable& quant_table = kDefaultQuantTable[i];
    for (size_t j = 0; j < kDctSize2; ++j) {
      uint32_t scaled_quant_value =
          (quant_table.value[kZigZag8x8[j]] * quality_normalized) / 100;
      scaled_quant_value = std::min(255u, std::max(1u, scaled_quant_value));
      header[idx++] = static_cast<uint8_t>(scaled_quant_value);
    }
  }

  // Start of Frame - Baseline.
  const uint8_t kStartOfFrame[] = {
      0xFF,
      JPEG_SOF0,  // Baseline.
      0x00,
      0x11,  // Segment length:17 (2-byte).
      8,     // Data precision.
      static_cast<uint8_t>((height >> 8) & 0xFF),
      static_cast<uint8_t>(height & 0xFF),
      static_cast<uint8_t>((width >> 8) & 0xFF),
      static_cast<uint8_t>(width & 0xFF),
      0x03,  // Number of Components.
  };
  memcpy(header + idx, kStartOfFrame, sizeof(kStartOfFrame));
  idx += sizeof(kStartOfFrame);
  for (uint8_t i = 0; i < 3; ++i) {
    // These are the values for U and V planes.
    uint8_t h_sample_factor = 1;
    uint8_t v_sample_factor = 1;
    uint8_t quant_table_number = 1;
    if (!i) {
      // These are the values for Y plane.
      h_sample_factor = 2;
      v_sample_factor = 2;
      quant_table_number = 0;
    }

    header[idx++] = i + 1;
    // Horizontal Sample Factor (4-bit high),
    // Vertical Sample Factor (4-bit low).
    header[idx++] = (h_sample_factor << 4) | v_sample_factor;
    header[idx++] = quant_table_number;
  }

  static const uint8_t kDcSegment[] = {
      0xFF, JPEG_DHT, 0x00,
      0x1F,  // Segment length:31 (2-byte).
  };
  static const uint8_t kAcSegment[] = {
      0xFF, JPEG_DHT, 0x00,
      0xB5,  // Segment length:181 (2-byte).
  };

  // Huffman Tables.
  for (size_t i = 0; i < 2; ++i) {
    // DC Table.
    memcpy(header + idx, kDcSegment, sizeof(kDcSegment));
    idx += sizeof(kDcSegment);

    // Type (4-bit high) = 0:DC, Index (4-bit low).
    header[idx++] = static_cast<uint8_t>(i);

    const JpegHuffmanTable& dcTable = kDefaultDcTable[i];
    for (size_t j = 0; j < kNumDcRunSizeBits; ++j)
      header[idx++] = dcTable.code_length[j];
    for (size_t j = 0; j < kNumDcCodeWordsHuffVal; ++j)
      header[idx++] = dcTable.code_value[j];

    // AC Table.
    memcpy(header + idx, kAcSegment, sizeof(kAcSegment));
    idx += sizeof(kAcSegment);

    // Type (4-bit high) = 1:AC, Index (4-bit low).
    header[idx++] = 0x10 | static_cast<uint8_t>(i);

    const JpegHuffmanTable& acTable = kDefaultAcTable[i];
    for (size_t j = 0; j < kNumAcRunSizeBits; ++j)
      header[idx++] = acTable.code_length[j];
    for (size_t j = 0; j < kNumAcCodeWordsHuffVal; ++j)
      header[idx++] = acTable.code_value[j];
  }

  // Start of Scan.
  static const uint8_t kStartOfScan[] = {
      0xFF, JPEG_SOS, 0x00,
      0x0C,  // Segment Length:12 (2-byte).
      0x03   // Number of components in scan.
  };
  memcpy(header + idx, kStartOfScan, sizeof(kStartOfScan));
  idx += sizeof(kStartOfScan);

  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t dc_table_number = 1;
    uint8_t ac_table_number = 1;
    if (!i) {
      dc_table_number = 0;
      ac_table_number = 0;
    }

    header[idx++] = i + 1;
    // DC Table Selector (4-bit high), AC Table Selector (4-bit low).
    header[idx++] = (dc_table_number << 4) | ac_table_number;
  }
  header[idx++] = 0x00;  // 0 for Baseline.
  header[idx++] = 0x3F;  // 63 for Baseline.
  header[idx++] = 0x00;  // 0 for Baseline.

  return idx << 3;
}

}  // namespace

size_t GetMaxCodedBufferSize(int width, int height) {
  return width * height * 3 / 2 + kJpegHeaderSize;
}

bool Encode(VaapiWrapper* vaapi_wrapper,
            const gfx::Size& input_size,
            int quality,
            VASurfaceID surface_id,
            VABufferID output_buffer_id) {
  DCHECK_NE(surface_id, VA_INVALID_SURFACE);

  if (input_size.width() > kMaxDimension ||
      input_size.height() > kMaxDimension) {
    return false;
  }

  // Set picture parameters.
  VAEncPictureParameterBufferJPEG pic_param;
  FillPictureParameters(input_size, quality, output_buffer_id, &pic_param);
  if (!vaapi_wrapper->SubmitBuffer(VAEncPictureParameterBufferType,
                                   sizeof(pic_param), &pic_param)) {
    return false;
  }

  VAQMatrixBufferJPEG q_matrix;
  FillQMatrix(&q_matrix);
  if (!vaapi_wrapper->SubmitBuffer(VAQMatrixBufferType, sizeof(q_matrix),
                                   &q_matrix)) {
    return false;
  }

  VAHuffmanTableBufferJPEGBaseline huff_table_param;
  FillHuffTableParameters(&huff_table_param);
  if (!vaapi_wrapper->SubmitBuffer(VAHuffmanTableBufferType,
                                   sizeof(huff_table_param),
                                   &huff_table_param)) {
    return false;
  }

  // Set slice parameters.
  VAEncSliceParameterBufferJPEG slice_param;
  FillSliceParameters(&slice_param);
  if (!vaapi_wrapper->SubmitBuffer(VAEncSliceParameterBufferType,
                                   sizeof(slice_param), &slice_param)) {
    return false;
  }

  JPEGHeader header_data;
  size_t length_in_bits = FillJpegHeader(input_size, quality, header_data);

  VAEncPackedHeaderParameterBuffer header_param;
  memset(&header_param, 0, sizeof(header_param));
  header_param.type = VAEncPackedHeaderRawData;
  header_param.bit_length = length_in_bits;
  header_param.has_emulation_bytes = 0;
  if (!vaapi_wrapper->SubmitBuffer(VAEncPackedHeaderParameterBufferType,
                                   sizeof(header_param), &header_param)) {
    return false;
  }

  if (!vaapi_wrapper->SubmitBuffer(VAEncPackedHeaderDataBufferType,
                                   (length_in_bits + 7) / 8, header_data)) {
    return false;
  }

  // Submit the |surface_id| which contains input YUV frame and begin encoding.
  return vaapi_wrapper->ExecuteAndDestroyPendingBuffers(surface_id);
}

}  // namespace vaapi_jpeg_encoder
}  // namespace media
