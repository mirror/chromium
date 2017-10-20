// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef MEDIA_FILTERS_JPEG_PARSER_H_
#define MEDIA_FILTERS_JPEG_PARSER_H_

#include <stddef.h>
#include <stdint.h>
#include "media/base/media_export.h"

namespace media {

// It's not a full featured JPEG parser implememtation. It only parses JPEG
// baseline sequential process. For explanations of each struct and its
// members, see JPEG specification at
// http://www.w3.org/Graphics/JPEG/itu-t81.pdf.

enum JpegMarker {
  JPEG_SOF0 = 0xC0,   // start of frame (baseline)
  JPEG_SOF1 = 0xC1,   // start of frame (extended sequential)
  JPEG_SOF2 = 0xC2,   // start of frame (progressive)
  JPEG_SOF3 = 0xC3,   // start of frame (lossless))
  JPEG_DHT = 0xC4,    // define huffman table
  JPEG_SOF5 = 0xC5,   // start of frame (differential, sequential)
  JPEG_SOF6 = 0xC6,   // start of frame (differential, progressive)
  JPEG_SOF7 = 0xC7,   // start of frame (differential, lossless)
  JPEG_SOF9 = 0xC9,   // start of frame (arithmetic coding, extended)
  JPEG_SOF10 = 0xCA,  // start of frame (arithmetic coding, progressive)
  JPEG_SOF11 = 0xCB,  // start of frame (arithmetic coding, lossless)
  JPEG_SOF13 = 0xCD,  // start of frame (differential, arithmetic, sequential)
  JPEG_SOF14 = 0xCE,  // start of frame (differential, arithmetic, progressive)
  JPEG_SOF15 = 0xCF,  // start of frame (differential, arithmetic, lossless)
  JPEG_RST0 = 0xD0,   // restart
  JPEG_RST1 = 0xD1,   // restart
  JPEG_RST2 = 0xD2,   // restart
  JPEG_RST3 = 0xD3,   // restart
  JPEG_RST4 = 0xD4,   // restart
  JPEG_RST5 = 0xD5,   // restart
  JPEG_RST6 = 0xD6,   // restart
  JPEG_RST7 = 0xD7,   // restart
  JPEG_SOI = 0xD8,    // start of image
  JPEG_EOI = 0xD9,    // end of image
  JPEG_SOS = 0xDA,    // start of scan
  JPEG_DQT = 0xDB,    // define quantization table
  JPEG_DRI = 0xDD,    // define restart internal
  JPEG_APP0 = 0xE0,   // start of application segment
  JPEG_MARKER_PREFIX = 0xFF,  // jpeg marker prefix
};

const size_t kJpegMaxHuffmanTableNumBaseline = 2;
const size_t kJpegMaxComponents = 4;
const size_t kJpegMaxQuantizationTableNum = 4;

// Parsing result of JPEG DHT marker.
struct JpegHuffmanTable {
  bool valid;
  uint8_t code_length[16];
  uint8_t code_value[256];
};

// K.3.3.1 "Specification of typical tables for DC difference coding"
static const JpegHuffmanTable kDefaultDcTable[kJpegMaxHuffmanTableNumBaseline] =
    {
        // luminance DC coefficients
        {
            true,
            {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
            {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
             0x0b},
        },
        // chrominance DC coefficients
        {
            true,
            {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb},
        },
};

// K.3.3.2 "Specification of typical tables for AC coefficient coding"
static const JpegHuffmanTable kDefaultAcTable[kJpegMaxHuffmanTableNumBaseline] =
    {
        // luminance AC coefficients
        {
            true,
            {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d},
            {0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41,
             0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91,
             0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24,
             0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a,
             0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38,
             0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53,
             0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
             0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
             0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93,
             0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
             0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
             0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
             0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1,
             0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2,
             0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa},
        },
        // chrominance AC coefficients
        {
            true,
            {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77},
            {0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12,
             0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14,
             0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15,
             0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17,
             0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37,
             0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
             0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65,
             0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
             0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
             0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
             0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5,
             0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
             0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
             0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2,
             0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa},
        },
};

// Parsing result of JPEG DQT marker.
struct JpegQuantizationTable {
  bool valid;
  uint8_t value[64];  // baseline only supports 8 bits quantization table
};

// Parsing result of a JPEG component.
struct JpegComponent {
  uint8_t id;
  uint8_t horizontal_sampling_factor;
  uint8_t vertical_sampling_factor;
  uint8_t quantization_table_selector;
};

// Parsing result of a JPEG SOF marker.
struct JpegFrameHeader {
  uint16_t visible_width;
  uint16_t visible_height;
  uint16_t coded_width;
  uint16_t coded_height;
  uint8_t num_components;
  JpegComponent components[kJpegMaxComponents];
};

// Parsing result of JPEG SOS marker.
struct JpegScanHeader {
  uint8_t num_components;
  struct Component {
    uint8_t component_selector;
    uint8_t dc_selector;
    uint8_t ac_selector;
  } components[kJpegMaxComponents];
};

struct JpegParseResult {
  JpegFrameHeader frame_header;
  JpegHuffmanTable dc_table[kJpegMaxHuffmanTableNumBaseline];
  JpegHuffmanTable ac_table[kJpegMaxHuffmanTableNumBaseline];
  JpegQuantizationTable q_table[kJpegMaxQuantizationTableNum];
  uint16_t restart_interval;
  JpegScanHeader scan;
  const char* data;
  // The size of compressed data of the first image.
  size_t data_size;
  // The size of the first entire image including header.
  size_t image_size;
};

// Parses JPEG picture in |buffer| with |length|.  Returns true iff header is
// valid and JPEG baseline sequential process is present. If parsed
// successfully, |result| is the parsed result.
MEDIA_EXPORT bool ParseJpegPicture(const uint8_t* buffer,
                                   size_t length,
                                   JpegParseResult* result);

// Parses the first image of JPEG stream in |buffer| with |length|.  Returns
// true iff header is valid and JPEG baseline sequential process is present.
// If parsed successfully, |result| is the parsed result.
MEDIA_EXPORT bool ParseJpegStream(const uint8_t* buffer,
                                  size_t length,
                                  JpegParseResult* result);

}  // namespace media

#endif  // MEDIA_FILTERS_JPEG_PARSER_H_
