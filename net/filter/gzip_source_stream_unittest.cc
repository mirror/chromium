// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bit_cast.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/test_completion_callback.h"
#include "net/filter/filter_source_stream_test_util.h"
#include "net/filter/gzip_source_stream.h"
#include "net/filter/mock_source_stream.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/zlib.h"

namespace net {

namespace {

const int kBigBufferSize = 4096;
const int kSmallBufferSize = 1;

enum class ReadResultType {
  // Each call to AddReadResult is a separate read from the lower layer
  // SourceStream.
  EVERYTHING_AT_ONCE,
  // Whenever AddReadResult is called, each byte is actually a separate read
  // result.
  ONE_BYTE_AT_A_TIME,
};

// How many bytes to leave unused at the end of |source_data_|. This margin is
// present so that tests that need to append data after the zlib EOF do not run
// out of room in the output buffer.
const size_t kEOFMargin = 64;

struct GzipTestParam {
  GzipTestParam(int buf_size,
                MockSourceStream::Mode read_mode,
                ReadResultType read_result_type)
      : buffer_size(buf_size),
        mode(read_mode),
        read_result_type(read_result_type) {}

  const int buffer_size;
  const MockSourceStream::Mode mode;
  const ReadResultType read_result_type;
};

}  // namespace

class GzipSourceStreamTest : public ::testing::TestWithParam<GzipTestParam> {
 protected:
  GzipSourceStreamTest() : output_buffer_size_(GetParam().buffer_size) {}

  // Helpful function to initialize the test fixture.|type| specifies which type
  // of GzipSourceStream to create. It must be one of TYPE_GZIP,
  // TYPE_GZIP_FALLBACK and TYPE_DEFLATE.
  void Init(SourceStream::SourceType type) {
    EXPECT_TRUE(SourceStream::TYPE_GZIP == type ||
                SourceStream::TYPE_GZIP_FALLBACK == type ||
                SourceStream::TYPE_DEFLATE == type);
    source_data_len_ = kBigBufferSize - kEOFMargin;

    for (size_t i = 0; i < source_data_len_; i++)
      source_data_[i] = i % 256;

    encoded_data_len_ = kBigBufferSize;
    CompressGzip(source_data_, source_data_len_, encoded_data_,
                 &encoded_data_len_, type != SourceStream::TYPE_DEFLATE);

    output_buffer_ = new IOBuffer(output_buffer_size_);
    std::unique_ptr<MockSourceStream> source(new MockSourceStream());
    if (GetParam().read_result_type == ReadResultType::ONE_BYTE_AT_A_TIME)
      source->set_read_one_byte_at_a_time(true);
    source_ = source.get();
    stream_ = GzipSourceStream::Create(std::move(source), type);
  }

  // If MockSourceStream::Mode is ASYNC, completes reads from |mock_stream|
  // until there's no pending read, and then returns |callback|'s result, once
  // it's invoked. If Mode is not ASYNC, does nothing and returns
  // |previous_result|.
  int CompleteReadsIfAsync(int previous_result,
                           TestCompletionCallback* callback,
                           MockSourceStream* mock_stream) {
    if (GetParam().mode == MockSourceStream::ASYNC) {
      EXPECT_EQ(ERR_IO_PENDING, previous_result);
      while (mock_stream->awaiting_completion())
        mock_stream->CompleteNextRead();
      return callback->WaitForResult();
    }
    return previous_result;
  }

  char* source_data() { return source_data_; }
  size_t source_data_len() { return source_data_len_; }

  char* encoded_data() { return encoded_data_; }
  size_t encoded_data_len() { return encoded_data_len_; }

  IOBuffer* output_buffer() { return output_buffer_.get(); }
  char* output_data() { return output_buffer_->data(); }
  size_t output_buffer_size() { return output_buffer_size_; }

  MockSourceStream* source() { return source_; }
  GzipSourceStream* stream() { return stream_.get(); }

  // Reads from |stream_| until an error occurs or the EOF is reached.
  // When an error occurs, returns the net error code. When an EOF is reached,
  // returns the number of bytes read and appends data read to |output|.
  int ReadStream(std::string* output) {
    int bytes_read = 0;
    while (true) {
      TestCompletionCallback callback;
      int rv = stream_->Read(output_buffer(), output_buffer_size(),
                             callback.callback());
      if (rv == ERR_IO_PENDING)
        rv = CompleteReadsIfAsync(rv, &callback, source());
      if (rv == OK)
        break;
      if (rv < OK)
        return rv;
      EXPECT_GT(rv, OK);
      bytes_read += rv;
      output->append(output_data(), rv);
    }
    return bytes_read;
  }

 private:
  char source_data_[kBigBufferSize];
  size_t source_data_len_;

  char encoded_data_[kBigBufferSize];
  size_t encoded_data_len_;

  scoped_refptr<IOBuffer> output_buffer_;
  const int output_buffer_size_;

  MockSourceStream* source_;
  std::unique_ptr<GzipSourceStream> stream_;
};

INSTANTIATE_TEST_CASE_P(
    GzipSourceStreamTests,
    GzipSourceStreamTest,
    ::testing::Values(GzipTestParam(kBigBufferSize,
                                    MockSourceStream::SYNC,
                                    ReadResultType::EVERYTHING_AT_ONCE),
                      GzipTestParam(kSmallBufferSize,
                                    MockSourceStream::SYNC,
                                    ReadResultType::EVERYTHING_AT_ONCE),
                      GzipTestParam(kBigBufferSize,
                                    MockSourceStream::ASYNC,
                                    ReadResultType::EVERYTHING_AT_ONCE),
                      GzipTestParam(kSmallBufferSize,
                                    MockSourceStream::ASYNC,
                                    ReadResultType::EVERYTHING_AT_ONCE),
                      GzipTestParam(kBigBufferSize,
                                    MockSourceStream::SYNC,
                                    ReadResultType::ONE_BYTE_AT_A_TIME),
                      GzipTestParam(kSmallBufferSize,
                                    MockSourceStream::SYNC,
                                    ReadResultType::ONE_BYTE_AT_A_TIME),
                      GzipTestParam(kBigBufferSize,
                                    MockSourceStream::ASYNC,
                                    ReadResultType::ONE_BYTE_AT_A_TIME),
                      GzipTestParam(kSmallBufferSize,
                                    MockSourceStream::ASYNC,
                                    ReadResultType::ONE_BYTE_AT_A_TIME)));

TEST_P(GzipSourceStreamTest, EmptyStream) {
  Init(SourceStream::TYPE_DEFLATE);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  TestCompletionCallback callback;
  std::string actual_output;
  int result = ReadStream(&actual_output);
  EXPECT_EQ(OK, result);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, DeflateOneBlock) {
  Init(SourceStream::TYPE_DEFLATE);
  source()->AddReadResult(encoded_data(), encoded_data_len(), OK,
                          GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(source_data_len()), rv);
  EXPECT_EQ(std::string(source_data(), source_data_len()), actual_output);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipOneBloc) {
  Init(SourceStream::TYPE_GZIP);
  source()->AddReadResult(encoded_data(), encoded_data_len(), OK,
                          GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(source_data_len()), rv);
  EXPECT_EQ(std::string(source_data(), source_data_len()), actual_output);
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, DeflateTwoReads) {
  Init(SourceStream::TYPE_DEFLATE);
  source()->AddReadResult(encoded_data(), 10, OK, GetParam().mode);
  source()->AddReadResult(encoded_data() + 10, encoded_data_len() - 10, OK,
                          GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(source_data_len()), rv);
  EXPECT_EQ(std::string(source_data(), source_data_len()), actual_output);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, PassThroughAfterEOF) {
  Init(SourceStream::TYPE_DEFLATE);
  char test_data[] = "Hello, World!";
  std::string encoded_data_with_trailing_data(encoded_data(),
                                              encoded_data_len());
  encoded_data_with_trailing_data.append(test_data, sizeof(test_data));
  source()->AddReadResult(encoded_data_with_trailing_data.c_str(),
                          encoded_data_len() + sizeof(test_data), OK,
                          GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  // Compressed and uncompressed data get returned as separate Read() results,
  // so this test has to call Read twice.
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  std::string expected_output(source_data(), source_data_len());
  expected_output.append(test_data, sizeof(test_data));
  EXPECT_EQ(static_cast<int>(expected_output.size()), rv);
  EXPECT_EQ(expected_output, actual_output);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, MissingZlibHeader) {
  Init(SourceStream::TYPE_DEFLATE);
  const size_t kZlibHeaderLen = 2;
  source()->AddReadResult(encoded_data() + kZlibHeaderLen,
                          encoded_data_len() - kZlibHeaderLen, OK,
                          GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(source_data_len()), rv);
  EXPECT_EQ(std::string(source_data(), source_data_len()), actual_output);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, CorruptGzipHeader) {
  Init(SourceStream::TYPE_GZIP);
  encoded_data()[1] = 0;
  int read_len = encoded_data_len();
  // Needed to a avoid a DCHECK that all reads were consumed.
  if (GetParam().read_result_type == ReadResultType::ONE_BYTE_AT_A_TIME)
    read_len = 2;
  source()->AddReadResult(encoded_data(), read_len, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(ERR_CONTENT_DECODING_FAILED, rv);
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipFallback) {
  Init(SourceStream::TYPE_GZIP_FALLBACK);
  source()->AddReadResult(source_data(), source_data_len(), OK,
                          GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);

  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(source_data_len()), rv);
  EXPECT_EQ(std::string(source_data(), source_data_len()), actual_output);
  EXPECT_EQ("GZIP_FALLBACK", stream()->Description());
}

// This test checks that the gzip stream source works correctly on 'golden' data
// as produced by gzip(1).
TEST_P(GzipSourceStreamTest, GzipCorrectness) {
  Init(SourceStream::TYPE_GZIP);
  const char kDecompressedData[] = "Hello, World!";
  const unsigned char kGzipData[] = {
      // From:
      //   echo -n 'Hello, World!' | gzip | xxd -i | sed -e 's/^/  /'
      // The footer is the last 8 bytes.
      0x1f, 0x8b, 0x08, 0x00, 0x2b, 0x02, 0x84, 0x55, 0x00, 0x03, 0xf3,
      0x48, 0xcd, 0xc9, 0xc9, 0xd7, 0x51, 0x08, 0xcf, 0x2f, 0xca, 0x49,
      0x51, 0x04, 0x00, 0xd0, 0xc3, 0x4a, 0xec, 0x0d, 0x00, 0x00, 0x00};
  source()->AddReadResult(reinterpret_cast<const char*>(kGzipData),
                          sizeof(kGzipData), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(strlen(kDecompressedData)), rv);
  EXPECT_EQ(kDecompressedData, actual_output);
  EXPECT_EQ("GZIP", stream()->Description());
}

// Same as GzipCorrectness except that last 8 bytes are removed to test that the
// implementation can handle missing footer.
TEST_P(GzipSourceStreamTest, GzipCorrectnessWithoutFooter) {
  Init(SourceStream::TYPE_GZIP);
  const char kDecompressedData[] = "Hello, World!";
  const unsigned char kGzipData[] = {
      // From:
      //   echo -n 'Hello, World!' | gzip | xxd -i | sed -e 's/^/  /'
      // with the 8 footer bytes removed.
      0x1f, 0x8b, 0x08, 0x00, 0x2b, 0x02, 0x84, 0x55, 0x00,
      0x03, 0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0xd7, 0x51, 0x08,
      0xcf, 0x2f, 0xca, 0x49, 0x51, 0x04, 0x00};
  source()->AddReadResult(reinterpret_cast<const char*>(kGzipData),
                          sizeof(kGzipData), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(strlen(kDecompressedData)), rv);
  EXPECT_EQ(kDecompressedData, actual_output);
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipCorrectnessBad230) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[230] = {
      31,  139, 8,   0,   0,   0,   0,   0,   0,   0,   173, 142, 61,  110, 2,
      49,  16,  70,  123, 159, 194, 50,  109, 64,  222, 181, 2,   203, 86,  65,
      201, 9,   16,  105, 8,   20,  150, 61,  108, 44,  252, 151, 245, 56,  17,
      66,  220, 61,  182, 171, 28,  32,  197, 52,  239, 123, 51,  243, 221, 9,
      101, 87,  227, 53,  27,  41,  83,  210, 130, 215, 114, 94,  192, 55,  120,
      76,  236, 169, 100, 128, 114, 170, 217, 137, 69,  209, 15,  106, 163, 147,
      184, 126, 77,  56,  240, 233, 196, 154, 144, 178, 115, 114, 190, 85,  199,
      193, 5,   33,  97,  183, 126, 153, 156, 52,  118, 165, 130, 107, 74,  142,
      90,  34,  180, 23,  61,  239, 54,  75,  190, 94,  114, 113, 224, 195, 248,
      220, 141, 162, 95,  13,  91,  126, 108, 30,  26,  7,   199, 224, 161, 138,
      239, 135, 215, 198, 164, 82,  144, 210, 62,  216, 70,  195, 143, 135, 185,
      113, 13,  23,  153, 45,  238, 193, 149, 242, 48,  167, 146, 126, 16,  74,
      239, 101, 106, 17,  252, 12,  237, 95,  12,  49,  199, 186, 80,  169, 241,
      185, 244, 43,  88,  240, 2,   30,  132, 158, 255, 92,  218, 89,  251, 38,
      111, 255, 112, 207, 32,  184, 182, 126, 38,  15,  242, 11,  175, 205, 203,
      178, 95,  1,   0,   0,
  };
  unsigned char output_buffer[351] = {
      123, 10,  32,  34,  107, 105, 110, 100, 34,  58,  32,  34,  99,  97,  108,
      101, 110, 100, 97,  114, 35,  101, 118, 101, 110, 116, 115, 34,  44,  10,
      32,  34,  101, 116, 97,  103, 34,  58,  32,  34,  92,  34,  112, 51,  50,
      56,  99,  55,  100, 115, 51,  107, 113, 103, 116, 56,  48,  103, 92,  34,
      34,  44,  10,  32,  34,  115, 117, 109, 109, 97,  114, 121, 34,  58,  32,
      34,  109, 101, 102, 116, 101, 115, 116, 49,  54,  64,  103, 109, 97,  105,
      108, 46,  99,  111, 109, 34,  44,  10,  32,  34,  117, 112, 100, 97,  116,
      101, 100, 34,  58,  32,  34,  50,  48,  49,  55,  45,  48,  54,  45,  48,
      51,  84,  48,  56,  58,  53,  49,  58,  51,  50,  46,  56,  57,  48,  90,
      34,  44,  10,  32,  34,  116, 105, 109, 101, 90,  111, 110, 101, 34,  58,
      32,  34,  85,  84,  67,  34,  44,  10,  32,  34,  97,  99,  99,  101, 115,
      115, 82,  111, 108, 101, 34,  58,  32,  34,  111, 119, 110, 101, 114, 34,
      44,  10,  32,  34,  100, 101, 102, 97,  117, 108, 116, 82,  101, 109, 105,
      110, 100, 101, 114, 115, 34,  58,  32,  91,  10,  32,  32,  123, 10,  32,
      32,  32,  34,  109, 101, 116, 104, 111, 100, 34,  58,  32,  34,  112, 111,
      112, 117, 112, 34,  44,  10,  32,  32,  32,  34,  109, 105, 110, 117, 116,
      101, 115, 34,  58,  32,  51,  48,  10,  32,  32,  125, 10,  32,  93,  44,
      10,  32,  34,  100, 101, 102, 97,  117, 108, 116, 65,  108, 108, 68,  97,
      121, 82,  101, 109, 105, 110, 100, 101, 114, 115, 34,  58,  32,  91,  10,
      32,  32,  123, 10,  32,  32,  32,  34,  109, 101, 116, 104, 111, 100, 34,
      58,  32,  34,  112, 111, 112, 117, 112, 34,  44,  10,  32,  32,  32,  34,
      109, 105, 110, 117, 116, 101, 115, 34,  58,  32,  51,  48,  10,  32,  32,
      125, 10,  32,  93,  44,  10,  32,  34,  105, 116, 101, 109, 115, 34,  58,
      32,  91,  93,  10,  125, 10,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipCorrectnessBad171) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[171] = {
      31,  139, 8,   0,   0,   0,   0,   0,   2,   255, 93,  202, 193, 14,  130,
      32,  0,   128, 225, 87,  18,  156, 7,   143, 129, 64,  25,  218, 36,  177,
      242, 22,  176, 52,  83,  105, 205, 48,  124, 250, 90,  135, 14,  29,  254,
      195, 183, 253, 184, 105, 129, 134, 149, 215, 62,  158, 106, 209, 160, 237,
      8,   172, 89,  139, 89,  47,  214, 113, 248, 178, 153, 143, 70,  5,   227,
      81,  177, 234, 251, 157, 96,  252, 52,  172, 122, 26,  28,  117, 10,  6,
      142, 31,  17,  43,  130, 150, 152, 50,  223, 148, 144, 82,  30,  80,  84,
      200, 95,  233, 159, 209, 199, 137, 186, 165, 189, 100, 212, 233, 68,  92,
      36,  33,  243, 142, 82,  172, 89,  94,  86,  192, 128, 61,  76,  189, 146,
      211, 67,  193, 254, 92,  15,  218, 233, 133, 68,  217, 21,  116, 252, 0,
      28,  15,  145, 85,  161, 112, 124, 184, 207, 245, 178, 194, 111, 67,  230,
      207, 233, 188, 0,   0,   0,
  };
  unsigned char output_buffer[188] = {
      67,  103, 104, 49,  99,  50,  86,  121, 99,  121, 57,  116, 90,  82, 103,
      66,  75,  110, 49,  111, 100, 72,  82,  119, 99,  122, 111, 118, 76, 50,
      120, 111, 77,  121, 53,  110, 98,  50,  57,  110, 98,  71,  86,  49, 99,
      50,  86,  121, 89,  50,  57,  117, 100, 71,  86,  117, 100, 67,  53, 106,
      98,  50,  48,  118, 76,  88,  66,  71,  81,  48,  104, 69,  100, 84, 78,
      73,  84,  50,  70,  70,  76,  48,  70,  66,  81,  85,  70,  66,  81, 85,
      70,  66,  81,  85,  70,  74,  76,  48,  70,  66,  81,  85,  70,  66, 81,
      85,  70,  66,  81,  85,  70,  66,  76,  48,  70,  68,  98,  107, 74, 108,
      85,  71,  70,  118, 99,  68,  82,  102, 85,  69,  69,  119, 79,  70, 70,
      67,  99,  71,  78,  84,  86,  49,  100, 49,  83,  50,  74,  121, 98, 85,
      116, 114, 98,  50,  108, 97,  90,  109, 99,  118, 99,  122, 69,  53, 77,
      105, 49,  106, 76,  87,  49,  118, 76,  51,  66,  111, 98,  51,  82, 118,
      76,  109, 112, 119, 90,  122, 65,  67,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipCorrectnessBad368) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[368] = {
      31,  139, 8,   0,   0,   0,   0,   0,   2,   255, 227, 58,  199, 204, 117,
      154, 153, 131, 225, 211, 236, 235, 173, 204, 135, 152, 185, 246, 51,  115,
      137, 26,  26,  26,  25,  154, 91,  24,  24,  89,  24,  155, 24,  26,  91,
      26,  25,  154, 26,  153, 27,  88,  10,  101, 42,  97,  151, 208, 96,  200,
      96,  204, 96,  107, 96,  124, 213, 120, 117, 249, 35,  237, 77,  140, 142,
      66,  194, 185, 169, 105, 37,  169, 197, 37,  134, 102, 14,  233, 185, 137,
      153, 57,  122, 201, 249, 185, 82,  90,  28,  12,  66,  216, 13,  208, 226,
      85,  54,  73,  141, 170, 50,  210, 247, 44,  245, 117, 43,  183, 53,  96,
      148, 114, 228, 146, 231, 96,  20,  96,  144, 96,  244, 194, 174, 163, 130,
      81,  136, 195, 55,  53,  77,  33,  4,   104, 137, 18,  51,  144, 165, 197,
      2,   98,  102, 113, 130, 72,  29,  5,   160, 128, 210, 117, 70,  160, 25,
      12,  248, 205, 40,  203, 40,  41,  41,  40,  182, 210, 215, 207, 201, 48,
      214, 75,  207, 207, 79,  207, 73,  45,  45,  78,  45,  74,  206, 207, 43,
      73,  205, 43,  1,   185, 90,  95,  183, 192, 205, 217, 195, 165, 212, 216,
      195, 63,  209, 85,  223, 17,  14,  60,  145, 216, 142, 250, 142, 206, 121,
      78,  169, 1,   137, 249, 5,   38,  241, 1,   142, 6,   22,  129, 78,  5,
      201, 193, 225, 225, 165, 222, 73,  69,  185, 222, 217, 249, 153, 81,  105,
      233, 250, 185, 249, 250, 5,   25,  249, 37,  249, 122, 89,  5,   233, 18,
      140, 74,  6,   174, 25,  161, 21,  190, 33,  158, 21,  126, 85,  233, 229,
      190, 89,  233, 85,  126, 46,  174, 85,  254, 32,  126, 136, 167, 177, 175,
      75,  118, 100, 137, 69,  106, 101, 89,  89,  81,  60,  8,   56,  6,   167,
      59,  89,  48,  58,  177, 185, 153, 152, 26,  26,  186, 94,  98,  148, 85,
      117, 77,  247, 117, 114, 76,  247, 77,  116, 113, 116, 117, 118, 44,  15,
      116, 115, 74,  79,  246, 116, 14,  204, 247, 113, 113, 180, 181, 5,   0,
      103, 250, 1,   43,  209, 1,   0,   0,
  };
  unsigned char output_buffer[465] = {
      10,  206, 3,   10,  203, 3,   8,   0,   242, 155, 215, 133, 3,   194, 3,
      10,  191, 3,   10,  21,  49,  49,  50,  49,  55,  56,  48,  50,  56,  51,
      52,  49,  51,  57,  50,  49,  53,  50,  55,  48,  57,  18,  105, 34,  21,
      49,  49,  50,  49,  55,  56,  48,  50,  56,  51,  52,  49,  51,  57,  50,
      49,  53,  50,  55,  48,  57,  40,  0,   104, 1,   104, 6,   128, 1,   234,
      129, 213, 167, 226, 43,  178, 1,   65,  18,  19,  109, 101, 102, 116, 101,
      115, 116, 49,  54,  64,  103, 109, 97,  105, 108, 46,  99,  111, 109, 26,
      42,  8,   0,   18,  21,  49,  49,  50,  49,  55,  56,  48,  50,  56,  51,
      52,  49,  51,  57,  50,  49,  53,  50,  55,  48,  57,  42,  13,  35,  52,
      101, 90,  122, 50,  47,  73,  117, 77,  70,  119, 61,  48,  1,   26,  65,
      10,  31,  8,   1,   16,  0,   24,  1,   74,  21,  49,  49,  50,  49,  55,
      56,  48,  50,  56,  51,  52,  49,  51,  57,  50,  49,  53,  50,  55,  48,
      57,  120, 1,   18,  8,   77,  101, 102, 32,  84,  101, 115, 116, 34,  3,
      77,  101, 102, 42,  4,   84,  101, 115, 116, 106, 9,   84,  101, 115, 116,
      44,  32,  77,  101, 102, 34,  215, 1,   10,  31,  8,   0,   16,  0,   24,
      1,   74,  21,  49,  49,  50,  49,  55,  56,  48,  50,  56,  51,  52,  49,
      51,  57,  50,  49,  53,  50,  55,  48,  57,  120, 1,   18,  118, 104, 116,
      116, 112, 115, 58,  47,  47,  108, 104, 51,  46,  103, 111, 111, 103, 108,
      101, 117, 115, 101, 114, 99,  111, 110, 116, 101, 110, 116, 46,  99,  111,
      109, 47,  45,  112, 70,  67,  72,  68,  117, 51,  72,  79,  97,  69,  47,
      65,  65,  65,  65,  65,  65,  65,  65,  65,  65,  73,  47,  65,  65,  65,
      65,  65,  65,  65,  65,  65,  65,  65,  47,  65,  67,  110, 66,  101, 80,
      97,  111, 112, 52,  95,  80,  65,  48,  56,  81,  66,  112, 99,  83,  87,
      87,  117, 75,  98,  114, 109, 75,  107, 111, 105, 90,  102, 103, 47,  109,
      111, 47,  112, 104, 111, 116, 111, 46,  106, 112, 103, 24,  1,   34,  48,
      69,  104, 85,  120, 77,  84,  73,  120, 78,  122, 103, 119, 77,  106, 103,
      122, 78,  68,  69,  122, 79,  84,  73,  120, 78,  84,  73,  51,  77,  68,
      107, 89,  116, 56,  101, 121, 118, 118, 114, 95,  95,  95,  95,  95,  65,
      83,  103, 66,  56,  1,   66,  6,   70,  52,  53,  49,  49,  69,  210, 1,
      29,  37,  69,  103, 77,  66,  65,  103, 77,  97,  68,  65,  69,  67,  65,
      119, 81,  70,  66,  103, 99,  73,  67,  81,  111, 76,  68,  65,  61,  61,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipCorrectnessBad230_1) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[230] = {
      31,  139, 8,   0,   0,   0,   0,   0,   0,   0,   173, 142, 61,  110, 2,
      49,  16,  70,  123, 159, 194, 50,  109, 64,  222, 181, 2,   203, 86,  65,
      201, 9,   16,  105, 8,   20,  150, 61,  108, 44,  252, 151, 245, 56,  17,
      66,  220, 61,  182, 171, 28,  32,  197, 52,  239, 123, 51,  243, 221, 9,
      101, 87,  227, 53,  27,  41,  83,  210, 130, 215, 114, 94,  192, 55,  120,
      76,  236, 169, 100, 128, 114, 170, 217, 137, 69,  209, 15,  106, 163, 147,
      184, 126, 77,  56,  240, 233, 196, 154, 144, 178, 115, 114, 190, 85,  199,
      193, 5,   33,  97,  183, 126, 153, 156, 52,  118, 165, 130, 107, 74,  142,
      90,  34,  180, 23,  61,  239, 54,  75,  190, 94,  114, 113, 224, 195, 248,
      220, 141, 162, 95,  13,  91,  126, 108, 30,  26,  7,   199, 224, 161, 138,
      239, 135, 215, 198, 164, 82,  144, 210, 62,  216, 70,  195, 143, 135, 185,
      113, 13,  23,  153, 45,  238, 193, 149, 242, 48,  167, 146, 126, 16,  74,
      239, 101, 106, 17,  252, 12,  237, 95,  12,  49,  199, 186, 80,  169, 241,
      185, 244, 43,  88,  240, 2,   30,  132, 158, 255, 92,  218, 89,  251, 38,
      111, 255, 112, 207, 32,  184, 182, 126, 38,  15,  242, 11,  175, 205, 203,
      178, 95,  1,   0,   0,
  };
  unsigned char output_buffer[351] = {
      123, 10,  32,  34,  107, 105, 110, 100, 34,  58,  32,  34,  99,  97,  108,
      101, 110, 100, 97,  114, 35,  101, 118, 101, 110, 116, 115, 34,  44,  10,
      32,  34,  101, 116, 97,  103, 34,  58,  32,  34,  92,  34,  112, 51,  50,
      56,  99,  55,  100, 115, 51,  107, 113, 103, 116, 56,  48,  103, 92,  34,
      34,  44,  10,  32,  34,  115, 117, 109, 109, 97,  114, 121, 34,  58,  32,
      34,  109, 101, 102, 116, 101, 115, 116, 49,  54,  64,  103, 109, 97,  105,
      108, 46,  99,  111, 109, 34,  44,  10,  32,  34,  117, 112, 100, 97,  116,
      101, 100, 34,  58,  32,  34,  50,  48,  49,  55,  45,  48,  54,  45,  48,
      51,  84,  48,  56,  58,  53,  49,  58,  51,  50,  46,  56,  57,  48,  90,
      34,  44,  10,  32,  34,  116, 105, 109, 101, 90,  111, 110, 101, 34,  58,
      32,  34,  85,  84,  67,  34,  44,  10,  32,  34,  97,  99,  99,  101, 115,
      115, 82,  111, 108, 101, 34,  58,  32,  34,  111, 119, 110, 101, 114, 34,
      44,  10,  32,  34,  100, 101, 102, 97,  117, 108, 116, 82,  101, 109, 105,
      110, 100, 101, 114, 115, 34,  58,  32,  91,  10,  32,  32,  123, 10,  32,
      32,  32,  34,  109, 101, 116, 104, 111, 100, 34,  58,  32,  34,  112, 111,
      112, 117, 112, 34,  44,  10,  32,  32,  32,  34,  109, 105, 110, 117, 116,
      101, 115, 34,  58,  32,  51,  48,  10,  32,  32,  125, 10,  32,  93,  44,
      10,  32,  34,  100, 101, 102, 97,  117, 108, 116, 65,  108, 108, 68,  97,
      121, 82,  101, 109, 105, 110, 100, 101, 114, 115, 34,  58,  32,  91,  10,
      32,  32,  123, 10,  32,  32,  32,  34,  109, 101, 116, 104, 111, 100, 34,
      58,  32,  34,  112, 111, 112, 117, 112, 34,  44,  10,  32,  32,  32,  34,
      109, 105, 110, 117, 116, 101, 115, 34,  58,  32,  51,  48,  10,  32,  32,
      125, 10,  32,  93,  44,  10,  32,  34,  105, 116, 101, 109, 115, 34,  58,
      32,  91,  93,  10,  125, 10,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

TEST_P(GzipSourceStreamTest, GzipCorrectnessBad213) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[213] = {
      31,  139, 8,   0,   0,   0,   0,   0,   2,   255, 69,  205, 219, 110, 131,
      32,  0,   0,   208, 95,  18,  236, 146, 246, 81,  192, 209, 86,  192, 153,
      1,   10,  111, 42,  73,  241, 222, 172, 214, 58,  190, 126, 125, 219, 7,
      156, 28,  236, 69,  104, 233, 103, 111, 43,  190, 149, 178, 141, 52,  64,
      86,  170, 113, 169, 130, 199, 233, 109, 127, 181, 176, 88,  107, 120, 141,
      109, 7,   96,  77,  121, 125, 165, 62,  114, 103, 20,  242, 238, 184, 53,
      165, 30,  29,  254, 152, 27,  120, 154, 27,  170, 158, 6,   158, 86,  22,
      163, 96,  49,  248, 49,  179, 155, 88,  101, 23,  243, 155, 224, 204, 31,
      51,  130, 31,  187, 32,  201, 206, 67,  10,  114, 50,  236, 220, 95,  82,
      65,  212, 129, 163, 5,   107, 205, 59,  84,  234, 39,  211, 90,  22,  67,
      146, 17,  201, 99,  65,  210, 32,  130, 137, 115, 162, 94,  66,  170, 111,
      6,   255, 95,  71,  245, 131, 77,  96,  180, 85,  177, 181, 103, 49,  176,
      114, 237, 28,  52,  171, 155, 124, 255, 21,  163, 123, 211, 71,  111, 91,
      188, 175, 22,  138, 112, 3,   156, 40,  240, 7,   165, 77,  141, 89,  232,
      0,   0,   0,
  };
  unsigned char output_buffer[232] = {
      67,  104, 78,  122, 99,  71,  70,  106, 90,  88,  77,  118, 87,  84,  99,
      48,  86,  49,  66,  90,  84,  85,  108, 111, 88,  122, 104, 67,  69,  103,
      120, 119, 99,  50,  81,  116, 97,  50,  74,  51,  90,  105, 49,  50,  97,
      71,  77,  97,  74,  71,  104, 48,  100, 72,  66,  122, 79,  105, 56,  118,
      98,  87,  86,  108, 100, 67,  53,  110, 98,  50,  57,  110, 98,  71,  85,
      117, 89,  50,  57,  116, 76,  51,  66,  122, 90,  67,  49,  114, 89,  110,
      100, 109, 76,  88,  90,  111, 89,  121, 65,  67,  75,  104, 56,  75,  68,
      67,  115, 120, 78,  68,  65,  120, 77,  122, 69,  49,  79,  68,  107, 120,
      77,  104, 73,  69,  78,  68,  85,  52,  77,  66,  111, 67,  86,  86,  77,
      105, 66,  87,  86,  117, 76,  86,  86,  84,  81,  107, 65,  75,  68,  84,
      77,  51,  78,  68,  69,  122, 78,  122, 89,  51,  79,  68,  85,  119, 78,
      84,  85,  83,  76,  50,  104, 48,  100, 72,  66,  122, 79,  105, 56,  118,
      100, 71,  86,  115, 76,  109, 49,  108, 90,  88,  81,  118, 99,  72,  78,
      107, 76,  87,  116, 105, 100, 50,  89,  116, 100, 109, 104, 106, 80,  51,
      66,  112, 98,  106, 48,  122, 78,  122, 81,  120, 77,  122, 99,  50,  78,
      122, 103, 49,  77,  68,  85,  49,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

// Same as GzipCorrectness except that last 8 bytes are removed to test that the
// implementation can handle missing footer.
TEST_P(GzipSourceStreamTest, GzipCorrectnessBad1185) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[1185] = {
      31,  139, 8,   0,   0,   0,   0,   0,   0,   0,   237, 84,  91,  207, 155,
      56,  20,  252, 65,  125, 40,  129, 36,  251, 229, 209, 198, 198, 56,  193,
      16,  110, 38,  246, 27,  151, 214, 132, 75,  160, 95,  72,  8,   249, 245,
      235, 180, 221, 174, 42,  173, 86,  43,  245, 109, 213, 7,   36,  4,   115,
      230, 140, 206, 153, 51,  118, 205, 40,  136, 109, 222, 125, 206, 198, 251,
      193, 251, 114, 5,   195, 48,  239, 138, 240, 195, 135, 235, 59,  81,  84,
      255, 59,  20,  24,  12,  70,  148, 149, 235, 79,  120, 179, 77,  78,  220,
      231, 228, 58,  39,  100, 220, 87,  198, 102, 227, 187, 181, 159, 247, 190,
      12,  211, 8,   134, 237, 174, 43,  112, 101, 166, 109, 13,  165, 209, 9,
      193, 157, 168, 36,  81,  152, 175, 202, 77,  129, 91,  51,  134, 239, 28,
      42,  133, 241, 249, 74,  65,  61,  184, 149, 187, 31,  133, 57,  93,  101,
      76,  49,  79,  96,  26,  157, 7,   193, 205, 189, 100, 168, 74,  114, 188,
      191, 243, 75,  116, 72,  219, 29,  145, 246, 219, 157, 103, 35,  226, 164,
      218, 147, 38,  165, 197, 131, 226, 194, 130, 171, 242, 161, 224, 1,   128,
      89,  29,  75,  59,  0,   123, 27,  169, 129, 22,  89,  119, 43,  221, 104,
      44,  178, 52,  6,   13,  158, 195, 26,  31,  144, 203, 159, 82,  127, 23,
      189, 115, 147, 68,  118, 226, 65,  33,  139, 134, 200, 38,  101, 12,  113,
      151, 132, 41,  19,  32,  86,  128, 81,  184, 11,  0,   206, 113, 168, 246,
      88,  69,  174, 255, 164, 11,  1,   120, 0,   8,   129, 99,  163, 32,  169,
      49,  5,   144, 226, 20,  251, 62,  143, 190, 245, 158, 209, 122, 13,  94,
      92,  64,  197, 208, 129, 40,  73,  241, 55,  46,  6,   62,  188, 184, 208,
      92,  143, 88,  237, 81,  82,  127, 215, 234, 26,  47,  252, 209, 6,   70,
      12,  90,  63,  248, 153, 223, 160, 69,  77,  143, 21,  225, 87,  121, 130,
      67,  97,  110, 58,  47,  227, 166, 204, 54,  198, 55,  156, 13,  118, 51,
      125, 213, 151, 182, 139, 99,  100, 69,  93,  65,  248, 156, 147, 221, 77,
      198, 171, 174, 234, 249, 173, 130, 63,  245, 209, 184, 245, 191, 226, 190,
      235, 191, 216, 36,  213, 179, 80,  139, 223, 132, 2,   52,  118, 120, 159,
      185, 237, 204, 67,  92,  18,  167, 201, 77,  110, 228, 167, 177, 174, 72,
      119, 47,  206, 171, 169, 48,  163, 14,  43,  252, 32,  202, 160, 2,   82,
      196, 59,  176, 38,  128, 206, 173, 247, 230, 144, 90,  81,  17,  81,  84,
      94,  162, 31,  223, 66,  213, 30,  128, 233, 204, 21,  164, 118, 144, 136,
      156, 132, 117, 143, 149, 191, 84,  174, 250, 209, 203, 86,  3,   18,  39,
      104, 96,  197, 30,  12,  225, 220, 9,   235, 9,   171, 233, 82,  152, 187,
      75,  65,  210, 73,  18,  199, 16,  145, 66,  108, 111, 127, 132, 49,  181,
      108, 128, 99,  207, 236, 110, 5,   233, 110, 50,  25,  231, 180, 43,  173,
      212, 137, 140, 216, 29,  207, 229, 83,  78,  41,  146, 219, 138, 132, 22,
      239, 229, 90,  118, 82,  36,  43,  31,  251, 237, 99,  62,  172, 58,  24,
      181, 221, 123, 229, 136, 39,  81,  64,  129, 120, 120, 188, 184, 14,  22,
      95,  138, 102, 28,  229, 41,  234, 131, 11,  172, 203, 222, 153, 202, 231,
      168, 53,  130, 41,  39,  209, 34,  79,  181, 17,  92,  252, 103, 217, 179,
      73,  156, 120, 155, 103, 111, 147, 158, 169, 222, 205, 172, 247, 61,  12,
      54,  160, 49,  189, 252, 23,  142, 232, 46,  123, 249, 148, 39,  61,  231,
      243, 8,   108, 192, 226, 160, 175, 13,  141, 217, 122, 203, 206, 170, 172,
      242, 86,  153, 252, 172, 231, 215, 120, 253, 110, 145, 203, 174, 251, 228,
      194, 174, 236, 187, 233, 229, 131, 114, 217, 253, 196, 231, 153, 206, 185,
      92,  86,  218, 223, 155, 214, 59,  125, 245, 253, 119, 94,  241, 107, 188,
      22,  188, 138, 83,  119, 175, 78,  161, 158, 61,  215, 239, 237, 139, 215,
      177, 65,  25,  31,  205, 95,  225, 149, 163, 230, 187, 123, 153, 127, 47,
      46,  218, 151, 151, 112, 170, 220, 110, 150, 145, 178, 15,  205, 27,  181,
      33,  221, 228, 110, 100, 148, 104, 184, 123, 154, 189, 90,  54,  150, 204,
      246, 75,  69,  216, 173, 176, 246, 23,  207, 228, 235, 146, 240, 37,  207,
      86,  186, 54,  122, 122, 214, 222, 40,  237, 213, 32,  221, 189, 238, 27,
      222, 171, 190, 107, 165, 222, 141, 246, 234, 148, 103, 155, 139, 246, 224,
      121, 81,  138, 66,  120, 248, 12,  148, 178, 105, 13,  199, 200, 149, 118,
      209, 78,  199, 202, 242, 167, 42,  173, 92,  214, 238, 234, 67,  93,  31,
      196, 9,   172, 146, 147, 227, 38,  8,   30,  178, 116, 90,  124, 183, 93,
      249, 233, 3,   23,  203, 244, 30,  38,  108, 161, 179, 130, 248, 117, 211,
      79,  108, 177, 101, 253, 8,   146, 240, 198, 26,  96,  121, 13,  94,  252,
      5,   141, 157, 210, 55,  154, 160, 143, 157, 64,  51,  52,  0,   96,  103,
      157, 173, 17,  176, 137, 122, 123, 248, 79,  118, 99,  73,  107, 120, 13,
      157, 125, 93,  203, 154, 82,  109, 165, 208, 55,  136, 231, 143, 99,  6,
      222, 64,  5,   0,   162, 103, 237, 197, 16,  60,  134, 35,  75,  202, 167,
      230, 221, 248, 246, 122, 97,  168, 212, 181, 212, 162, 222, 21,  125, 197,
      203, 31,  248, 47,  95,  241, 209, 144, 177, 70,  204, 254, 115, 236, 3,
      66,  231, 160, 9,   103, 70,  194, 173, 120, 130, 77,  208, 12,  150, 60,
      255, 173, 205, 248, 161, 109, 248, 166, 173, 22,  250, 230, 129, 21,  244,
      98,  45,  26,  176, 245, 17,  152, 37,  26,  27,  134, 218, 109, 208, 84,
      61,  13,  250, 76,  231, 25,  100, 199, 63,  34,  117, 4,   110, 8,   192,
      114, 8,   95,  115, 96,  185, 211, 80,  83,  107, 219, 202, 166, 62,  51,
      52,  24,  12,  193, 54,  232, 217, 28,  36,  195, 214, 55,  133, 186, 205,
      236, 213, 115, 247, 87,  207, 223, 217, 253, 59,  187, 255, 33,  187, 77,
      191, 141, 242, 0,   243, 233, 96,  202, 154, 93,  36,  78,  210, 118, 46,
      185, 131, 226, 85,  221, 38,  9,   183, 248, 121, 231, 228, 216, 241, 252,
      116, 67,  171, 167, 99,  39,  43,  110, 189, 178, 155, 46,  191, 179, 251,
      127, 158, 221, 95,  2,   53,  8,   97,  58,  215, 194, 126, 179, 116, 222,
      46,  250, 153, 25,  98,  107, 63,  105, 55,  12,  165, 166, 206, 223, 149,
      206, 34,  241, 30,  184, 254, 168, 196, 217, 185, 232, 251, 56,  249, 157,
      116, 118, 79,  97,  69,  243, 233, 149, 53,  102, 253, 185, 32,  206, 89,
      239, 123, 13,  82,  5,   254, 4,   129, 106, 99,  175, 244, 10,  0,   0,
  };
  unsigned char output_buffer[2804] = {
      67,  104, 77,  73,  65,  83,  67,  86,  108, 102, 87,  112, 118, 75,  76,
      113, 115, 65,  111, 111, 119, 57,  98,  81,  43,  43,  115, 114, 71,  103,
      73,  73,  65,  83,  75,  98,  69,  65,  111, 48,  82,  87,  99,  52,  101,
      69,  53,  54,  84,  88,  86,  78,  86,  71,  115, 119, 84,  71,  112, 74,
      100, 48,  53,  53,  78,  72,  104, 78,  97,  109, 78,  90,  81,  85,  82,
      66,  81,  107, 57,  108, 98,  69,  100, 50,  85,  107, 104, 66,  90,  48,
      108, 89,  89,  86,  70,  82,  99,  71,  82,  81,  97,  49,  99,  53,  98,
      69,  107, 50,  83,  66,  114, 86,  66,  103, 103, 69,  69,  105, 115, 73,
      65,  104, 111, 72,  100, 72,  74,  112, 89,  50,  116, 115, 90,  83,  73,
      69,  86,  84,  66,  85,  82,  105, 111, 89,  86,  50,  74,  90,  77,  68,
      100, 84,  97,  69,  74,  118, 86,  110, 82,  75,  85,  107, 57,  71,  90,
      67,  56,  118, 86,  87,  112, 68,  86,  71,  100, 74,  71,  106, 85,  73,
      98,  120, 73,  69,  98,  51,  66,  49,  99,  120, 103, 66,  75,  65,  65,
      119, 103, 80,  99,  67,  79,  65,  74,  67,  68,  103, 111, 73,  98,  87,
      108, 117, 99,  72,  82,  112, 98,  87,  85,  83,  65,  106, 69,  119, 81,
      104, 69,  75,  68,  72,  86,  122, 90,  87,  108, 117, 89,  109, 70,  117,
      90,  71,  90,  108, 89,  120, 73,  66,  77,  82,  111, 82,  67,  71,  99,
      83,  66,  69,  108, 84,  81,  85,  77,  89,  65,  83,  103, 65,  77,  73,
      66,  57,  79,  65,  69,  97,  69,  81,  103, 74,  69,  103, 82,  72,  78,
      122, 73,  121, 71,  65,  69,  111, 65,  68,  68,  65,  80,  106, 103, 66,
      71,  104, 69,  73,  65,  66,  73,  69,  85,  69,  78,  78,  86,  82,  103,
      66,  75,  65,  65,  119, 119, 68,  52,  52,  65,  82,  111, 82,  67,  65,
      103, 83,  66,  70,  66,  68,  84,  85,  69,  89,  65,  83,  103, 65,  77,
      77,  65,  43,  79,  65,  69,  97,  68,  119, 104, 112, 69,  103, 74,  68,
      84,  104, 103, 66,  75,  65,  65,  119, 103, 72,  48,  52,  65,  82,  111,
      80,  67,  65,  48,  83,  65,  107, 78,  79,  71,  65,  69,  111, 65,  68,
      68,  65,  80,  106, 103, 66,  71,  104, 48,  73,  98,  104, 73,  80,  100,
      71,  86,  115, 90,  88,  66,  111, 98,  50,  53,  108, 76,  87,  86,  50,
      90,  87,  53,  48,  71,  65,  69,  111, 65,  68,  67,  65,  57,  119, 73,
      52,  65,  82,  111, 99,  67,  72,  69,  83,  68,  51,  82,  108, 98,  71,
      86,  119, 97,  71,  57,  117, 90,  83,  49,  108, 100, 109, 86,  117, 100,
      66,  103, 66,  75,  65,  65,  119, 103, 72,  48,  52,  65,  82,  111, 99,
      67,  72,  52,  83,  68,  51,  82,  108, 98,  71,  86,  119, 97,  71,  57,
      117, 90,  83,  49,  108, 100, 109, 86,  117, 100, 66,  103, 66,  75,  65,
      65,  119, 119, 68,  52,  52,  65,  82,  111, 110, 67,  71,  85,  83,  66,
      69,  103, 121, 78,  106, 81,  89,  65,  106, 67,  81,  118, 119, 86,  67,
      70,  119, 111, 83,  99,  71,  70,  106, 97,  50,  86,  48,  97,  88,  112,
      104, 100, 71,  108, 118, 98,  105, 49,  116, 98,  50,  82,  108, 69,  103,
      69,  120, 71,  103, 48,  73,  89,  66,  73,  68,  86,  108, 65,  52,  71,
      65,  73,  119, 107, 76,  56,  70,  71,  104, 103, 73,  89,  82,  73,  68,
      99,  110, 82,  52,  71,  65,  73,  119, 107, 76,  56,  70,  81,  103, 107,
      75,  65,  50,  70,  119, 100, 66,  73,  67,  79,  84,  89,  97,  71,  81,
      104, 109, 69,  103, 78,  121, 100, 72,  103, 89,  65,  106, 67,  81,  118,
      119, 86,  67,  67,  103, 111, 68,  89,  88,  66,  48,  69,  103, 77,  120,
      77,  68,  69,  97,  70,  81,  104, 116, 69,  103, 116, 110, 98,  50,  57,
      110, 98,  71,  85,  116, 90,  71,  70,  48,  89,  82,  103, 68,  77,  74,
      67,  47,  66,  83,  73,  51,  67,  65,  69,  83,  76,  50,  108, 117, 98,
      71,  108, 117, 90,  84,  112, 119, 85,  108, 99,  51,  85,  70,  82,  48,
      83,  72,  112, 105, 99,  122, 90,  116, 85,  68,  90,  54,  100, 71,  81,
      51,  86,  109, 90,  52,  90,  108, 90,  89,  84,  49,  78,  69,  78,  107,
      120, 119, 75,  49,  108, 66,  82,  107, 108, 114, 100, 70,  89,  122, 71,
      103, 65,  103, 65,  83,  111, 120, 67,  65,  69,  83,  75,  51,  86,  121,
      98,  106, 112, 112, 90,  88,  82,  109, 79,  110, 66,  104, 99,  109, 70,
      116, 99,  122, 112, 121, 100, 72,  65,  116, 97,  71,  82,  121, 90,  88,
      104, 48,  79,  110, 78,  122, 99,  109, 77,  116, 89,  88,  86,  107, 97,
      87,  56,  116, 98,  71,  86,  50,  90,  87,  119, 89,  65,  83,  111, 111,
      67,  65,  73,  83,  73,  110, 86,  121, 98,  106, 112, 112, 90,  88,  82,
      109, 79,  110, 66,  104, 99,  109, 70,  116, 99,  122, 112, 121, 100, 72,
      65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  79,  110, 82,  118, 90,
      109, 90,  122, 90,  88,  81,  89,  65,  105, 112, 65,  67,  65,  77,  83,
      79,  109, 104, 48,  100, 72,  65,  54,  76,  121, 57,  51,  100, 51,  99,
      117, 100, 50,  86,  105, 99,  110, 82,  106, 76,  109, 57,  121, 90,  121,
      57,  108, 101, 72,  66,  108, 99,  109, 108, 116, 90,  87,  53,  48,  99,
      121, 57,  121, 100, 72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,
      76,  50,  70,  105, 99,  121, 49,  122, 90,  87,  53,  107, 76,  88,  82,
      112, 98,  87,  85,  89,  65,  105, 112, 65,  67,  65,  89,  83,  79,  109,
      104, 48,  100, 72,  65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100,
      50,  86,  105, 99,  110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108,
      101, 72,  66,  108, 99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,
      121, 100, 72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  51,
      66,  115, 89,  88,  108, 118, 100, 88,  81,  116, 90,  71,  86,  115, 89,
      88,  107, 89,  65,  105, 112, 70,  67,  65,  99,  83,  80,  50,  104, 48,
      100, 72,  65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,
      105, 99,  110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,
      66,  108, 99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100,
      72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  51,  90,  112,
      90,  71,  86,  118, 76,  87,  78,  118, 98,  110, 82,  108, 98,  110, 81,
      116, 100, 72,  108, 119, 90,  82,  103, 67,  75,  106, 56,  73,  67,  66,
      73,  53,  97,  72,  82,  48,  99,  68,  111, 118, 76,  51,  100, 51,  100,
      121, 53,  51,  90,  87,  74,  121, 100, 71,  77,  117, 98,  51,  74,  110,
      76,  50,  86,  52,  99,  71,  86,  121, 97,  87,  49,  108, 98,  110, 82,
      122, 76,  51,  74,  48,  99,  67,  49,  111, 90,  72,  74,  108, 101, 72,
      81,  118, 100, 109, 108, 107, 90,  87,  56,  116, 100, 71,  108, 116, 97,
      87,  53,  110, 71,  65,  73,  105, 121, 103, 103, 73,  66,  66,  75,  102,
      65,  103, 103, 67,  73,  104, 66,  112, 82,  72,  90,  67,  98,  107, 116,
      80,  100, 51,  78,  116, 100, 85,  100, 72,  77,  107, 57,  104, 75,  104,
      104, 75,  89,  88,  65,  49,  84,  88,  70,  72,  84,  68,  66,  75,  87,
      85,  116, 121, 78,  72,  107, 49,  78,  85,  120, 69,  98,  121, 116, 114,
      81,  84,  77,  121, 73,  119, 103, 66,  69,  65,  69,  97,  68,  122, 69,
      51,  77,  121, 52,  120, 79,  84,  81,  117, 77,  106, 65,  51,  76,  106,
      69,  121, 78,  121, 68,  112, 108, 103, 69,  111, 65,  84,  68,  47,  108,
      89,  68,  119, 66,  48,  65,  65,  77,  105, 77,  73,  65,  82,  65,  67,
      71,  103, 56,  120, 78,  122, 77,  117, 77,  84,  107, 48,  76,  106, 73,
      119, 78,  121, 52,  120, 77,  106, 99,  103, 54,  90,  89,  66,  75,  65,
      69,  119, 47,  112, 87,  65,  56,  65,  100, 65,  65,  68,  73,  105, 67,
      65,  69,  81,  65,  120, 111, 80,  77,  84,  99,  122, 76,  106, 69,  53,
      78,  67,  52,  121, 77,  68,  99,  117, 77,  84,  73,  51,  73,  76,  115,
      68,  75,  65,  69,  119, 47,  90,  87,  65,  56,  65,  100, 65,  65,  68,
      73,  113, 67,  65,  69,  81,  65,  82,  111, 87,  77,  106, 89,  119, 78,
      122, 112, 109, 79,  71,  73,  119, 79,  106, 81,  119, 77,  71,  81,  54,
      89,  122, 65,  53,  79,  106, 111, 51,  90,  105, 68,  112, 108, 103, 69,
      111, 65,  84,  68,  47,  48,  89,  68,  119, 66,  48,  65,  65,  77,  105,
      111, 73,  65,  82,  65,  67,  71,  104, 89,  121, 78,  106, 65,  51,  79,
      109, 89,  52,  89,  106, 65,  54,  78,  68,  65,  119, 90,  68,  112, 106,
      77,  68,  107, 54,  79,  106, 100, 109, 73,  79,  109, 87,  65,  83,  103,
      66,  77,  80,  55,  82,  103, 80,  65,  72,  81,  65,  65,  121, 75,  81,
      103, 66,  69,  65,  77,  97,  70,  106, 73,  50,  77,  68,  99,  54,  90,
      106, 104, 105, 77,  68,  111, 48,  77,  68,  66,  107, 79,  109, 77,  119,
      79,  84,  111, 54,  78,  50,  89,  103, 117, 119, 77,  111, 65,  84,  68,
      57,  48,  89,  68,  119, 66,  48,  65,  65,  71,  106, 85,  73,  98,  120,
      73,  69,  98,  51,  66,  49,  99,  120, 103, 66,  75,  65,  65,  119, 103,
      80,  99,  67,  79,  65,  74,  67,  68,  103, 111, 73,  98,  87,  108, 117,
      99,  72,  82,  112, 98,  87,  85,  83,  65,  106, 69,  119, 81,  104, 69,
      75,  68,  72,  86,  122, 90,  87,  108, 117, 89,  109, 70,  117, 90,  71,
      90,  108, 89,  120, 73,  66,  77,  82,  111, 82,  67,  71,  99,  83,  66,
      69,  108, 84,  81,  85,  77,  89,  65,  83,  103, 65,  77,  73,  66,  57,
      79,  65,  69,  97,  69,  81,  103, 74,  69,  103, 82,  72,  78,  122, 73,
      121, 71,  65,  69,  111, 65,  68,  68,  65,  80,  106, 103, 66,  71,  104,
      69,  73,  65,  66,  73,  69,  85,  69,  78,  78,  86,  82,  103, 66,  75,
      65,  65,  119, 119, 68,  52,  52,  65,  82,  111, 82,  67,  65,  103, 83,
      66,  70,  66,  68,  84,  85,  69,  89,  65,  83,  103, 65,  77,  77,  65,
      43,  79,  65,  69,  97,  68,  119, 104, 112, 69,  103, 74,  68,  84,  104,
      103, 66,  75,  65,  65,  119, 103, 72,  48,  52,  65,  82,  111, 80,  67,
      65,  48,  83,  65,  107, 78,  79,  71,  65,  69,  111, 65,  68,  68,  65,
      80,  106, 103, 66,  71,  104, 48,  73,  98,  104, 73,  80,  100, 71,  86,
      115, 90,  88,  66,  111, 98,  50,  53,  108, 76,  87,  86,  50,  90,  87,
      53,  48,  71,  65,  69,  111, 65,  68,  67,  65,  57,  119, 73,  52,  65,
      82,  111, 99,  67,  72,  69,  83,  68,  51,  82,  108, 98,  71,  86,  119,
      97,  71,  57,  117, 90,  83,  49,  108, 100, 109, 86,  117, 100, 66,  103,
      66,  75,  65,  65,  119, 103, 72,  48,  52,  65,  82,  111, 99,  67,  72,
      52,  83,  68,  51,  82,  108, 98,  71,  86,  119, 97,  71,  57,  117, 90,
      83,  49,  108, 100, 109, 86,  117, 100, 66,  103, 66,  75,  65,  65,  119,
      119, 68,  52,  52,  65,  82,  111, 110, 67,  71,  85,  83,  66,  69,  103,
      121, 78,  106, 81,  89,  65,  106, 67,  81,  118, 119, 86,  67,  70,  119,
      111, 83,  99,  71,  70,  106, 97,  50,  86,  48,  97,  88,  112, 104, 100,
      71,  108, 118, 98,  105, 49,  116, 98,  50,  82,  108, 69,  103, 69,  120,
      71,  103, 48,  73,  89,  66,  73,  68,  86,  108, 65,  52,  71,  65,  73,
      119, 107, 76,  56,  70,  71,  104, 103, 73,  89,  82,  73,  68,  99,  110,
      82,  52,  71,  65,  73,  119, 107, 76,  56,  70,  81,  103, 107, 75,  65,
      50,  70,  119, 100, 66,  73,  67,  79,  84,  89,  97,  71,  81,  104, 109,
      69,  103, 78,  121, 100, 72,  103, 89,  65,  106, 67,  81,  118, 119, 86,
      67,  67,  103, 111, 68,  89,  88,  66,  48,  69,  103, 77,  120, 77,  68,
      69,  97,  70,  81,  104, 116, 69,  103, 116, 110, 98,  50,  57,  110, 98,
      71,  85,  116, 90,  71,  70,  48,  89,  82,  103, 68,  77,  74,  67,  47,
      66,  83,  73,  51,  67,  65,  69,  83,  76,  50,  108, 117, 98,  71,  108,
      117, 90,  84,  112, 50,  78,  107, 82,  97,  79,  69,  86,  116, 75,  50,
      90,  104, 77,  110, 90,  69,  84,  85,  107, 119, 99,  86,  70,  68,  83,
      49,  104, 107, 84,  84,  86,  51,  86,  105, 57,  70,  97,  69,  70,  76,
      78,  85,  53,  73,  100, 122, 70,  67,  84,  49,  86,  51,  71,  103, 65,
      103, 73,  121, 111, 120, 67,  65,  69,  83,  75,  51,  86,  121, 98,  106,
      112, 112, 90,  88,  82,  109, 79,  110, 66,  104, 99,  109, 70,  116, 99,
      122, 112, 121, 100, 72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,
      79,  110, 78,  122, 99,  109, 77,  116, 89,  88,  86,  107, 97,  87,  56,
      116, 98,  71,  86,  50,  90,  87,  119, 89,  65,  83,  111, 111, 67,  65,
      73,  83,  73,  110, 86,  121, 98,  106, 112, 112, 90,  88,  82,  109, 79,
      110, 66,  104, 99,  109, 70,  116, 99,  122, 112, 121, 100, 72,  65,  116,
      97,  71,  82,  121, 90,  88,  104, 48,  79,  110, 82,  118, 90,  109, 90,
      122, 90,  88,  81,  89,  65,  105, 112, 65,  67,  65,  77,  83,  79,  109,
      104, 48,  100, 72,  65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100,
      50,  86,  105, 99,  110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108,
      101, 72,  66,  108, 99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,
      121, 100, 72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  50,
      70,  105, 99,  121, 49,  122, 90,  87,  53,  107, 76,  88,  82,  112, 98,
      87,  85,  89,  65,  105, 112, 65,  67,  65,  89,  83,  79,  109, 104, 48,
      100, 72,  65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,
      105, 99,  110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,
      66,  108, 99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100,
      72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  51,  66,  115,
      89,  88,  108, 118, 100, 88,  81,  116, 90,  71,  86,  115, 89,  88,  107,
      89,  65,  105, 112, 70,  67,  65,  99,  83,  80,  50,  104, 48,  100, 72,
      65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,  105, 99,
      110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,  66,  108,
      99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100, 72,  65,
      116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  51,  90,  112, 90,  71,
      86,  118, 76,  87,  78,  118, 98,  110, 82,  108, 98,  110, 81,  116, 100,
      72,  108, 119, 90,  82,  103, 67,  75,  106, 56,  73,  67,  66,  73,  53,
      97,  72,  82,  48,  99,  68,  111, 118, 76,  51,  100, 51,  100, 121, 53,
      51,  90,  87,  74,  121, 100, 71,  77,  117, 98,  51,  74,  110, 76,  50,
      86,  52,  99,  71,  86,  121, 97,  87,  49,  108, 98,  110, 82,  122, 76,
      51,  74,  48,  99,  67,  49,  111, 90,  72,  74,  108, 101, 72,  81,  118,
      100, 109, 108, 107, 90,  87,  56,  116, 100, 71,  108, 116, 97,  87,  53,
      110, 71,  65,  73,  113, 79,  103, 111, 89,  89,  50,  70,  115, 98,  67,
      56,  51,  79,  84,  81,  121, 79,  84,  81,  119, 77,  68,  77,  52,  78,
      84,  107, 53,  77,  68,  85,  50,  77,  106, 65,  49,  69,  65,  77,  89,
      114, 79,  72,  78,  112, 103, 89,  105, 70,  110, 86,  117, 100, 88,  78,
      108, 90,  70,  57,  122, 89,  51,  82,  119, 88,  51,  66,  49,  99,  50,
      104, 102, 98,  71,  70,  105, 90,  87,  119, 52,  65,  85,  103, 65,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

// Same as GzipCorrectness except that last 8 bytes are removed to test that the
// implementation can handle missing footer.
TEST_P(GzipSourceStreamTest, GzipCorrectnessBad1190) {
  Init(SourceStream::TYPE_GZIP);
  unsigned char input_buffer[1190] = {
      31,  139, 8,   0,   0,   0,   0,   0,   0,   0,   237, 84,  77,  175, 155,
      56,  20,  253, 65,  93,  148, 64,  146, 62,  22,  179, 176, 177, 49,  14,
      24,  194, 103, 98,  239, 248, 104, 77,  248, 8,   244, 61,  18,  2,   191,
      126, 156, 190, 78,  71,  149, 70,  163, 145, 186, 27,  117, 129, 132, 172,
      115, 207, 61,  186, 247, 220, 99,  213, 33,  5,   49,  186, 125, 250, 240,
      213, 52,  43,  47,  237, 4,   133, 46,  253, 34,  180, 227, 253, 213, 125,
      184, 5,   6,   131, 22,  157, 202, 237, 103, 188, 219, 39,  231, 204, 207,
      200, 219, 156, 144, 241, 80,  105, 187, 157, 239, 212, 126, 222, 251, 34,
      76,  35,  24,  182, 102, 87,  224, 74,  79,  219, 26,  10,  173, 227, 60,
      179, 163, 146, 68,  97,  190, 41,  119, 5,   110, 245, 24,  190, 102, 80,
      74,  140, 47,  111, 20,  212, 131, 83,  57,  135, 145, 235, 211, 155, 136,
      41,  206, 18,  152, 70,  151, 129, 103, 250, 65,  48,  84,  37,  57,  62,
      220, 179, 107, 228, 166, 173, 73,  132, 245, 114, 207, 78,  35,  202, 72,
      117, 32,  77,  74,  139, 7,   197, 133, 1,   55,  229, 67,  66,  23,  128,
      89,  30,  75,  43,  0,   7,   11,  201, 129, 22,  167, 238, 86,  58,  209,
      88,  156, 210, 24,  52,  120, 14,  107, 236, 34,  39,  91,  133, 122, 231,
      189, 125, 19,  68,  116, 252, 65,  33,  139, 134, 200, 34,  101, 12,  113,
      151, 132, 41,  227, 32,  150, 128, 81,  104, 6,   0,   231, 56,  148, 7,
      44,  35,  199, 95,  233, 66,  0,   30,  0,   66,  224, 216, 72,  72,  106,
      76,  1,   164, 56,  197, 190, 159, 69,  239, 189, 103, 180, 221, 130, 39,
      23,  144, 49,  180, 33,  74,  82,  252, 206, 197, 192, 135, 39,  23,  154,
      235, 17,  203, 3,   74,  234, 239, 90,  29,  237, 137, 63,  90,  64,  139,
      65,  235, 7,   63,  243, 107, 180, 168, 233, 177, 34,  217, 155, 56,  195,
      161, 208, 119, 157, 119, 202, 116, 113, 218, 105, 239, 56,  11,  152, 51,
      125, 214, 151, 150, 131, 99,  100, 68,  93,  65,  178, 57,  39,  230, 77,
      196, 155, 174, 234, 179, 91,  5,   127, 234, 163, 112, 219, 127, 197, 125,
      215, 127, 181, 72,  170, 102, 33,  23,  191, 9,   57,  104, 172, 240, 62,
      103, 150, 61,  15,  113, 73,  236, 38,  215, 51,  45,  63,  143, 117, 69,
      186, 123, 113, 217, 76,  133, 30,  117, 88,  226, 7,   145, 26,  229, 144,
      162, 172, 3,   91,  2,   232, 220, 122, 47,  54,  169, 37,  229, 17,  69,
      229, 53,  250, 241, 22,  202, 214, 5,   186, 61,  87,  144, 90,  65,  194,
      115, 18,  214, 61,  150, 254, 82,  57,  242, 71,  47,  75,  14,  136, 159,
      161, 134, 37,  123, 48,  132, 115, 59,  172, 39,  44,  167, 107, 161, 155,
      215, 130, 164, 147, 32,  182, 198, 35,  137, 216, 193, 250, 8,   99,  106,
      88,  0,   199, 158, 222, 221, 10,  210, 221, 68,  50,  206, 105, 87,  26,
      169, 29,  105, 177, 51,  94,  202, 85,  76,  41,  18,  251, 138, 132, 70,
      214, 139, 173, 232, 4,   79,  54,  62,  246, 219, 199, 236, 110, 58,  24,
      181, 221, 107, 101, 243, 149, 72,  32,  65,  60,  60,  158, 92,  174, 145,
      45,  69,  51,  142, 226, 28,  245, 193, 21,  214, 101, 111, 79,  229, 58,
      42,  141, 96,  202, 73,  180, 136, 115, 173, 5,   87,  127, 45,  123, 54,
      241, 115, 214, 230, 167, 151, 73,  205, 84,  237, 102, 86,  251, 30,  6,
      11,  208, 152, 94,  255, 11,  71,  116, 23,  189, 88,  197, 89,  205, 249,
      50,  2,   11,  176, 56,  232, 107, 77,  97,  246, 222, 98,  26,  149, 81,
      222, 42,  61,  187, 168, 249, 53,  94,  111, 46,  98,  49,  187, 207, 14,
      236, 202, 190, 155, 158, 62,  40,  23,  243, 39,  62,  79,  183, 47,  229,
      178, 81,  254, 222, 181, 222, 249, 155, 239, 191, 243, 242, 95,  227, 53,
      224, 27,  63,  119, 247, 234, 28,  170, 217, 103, 234, 191, 125, 242, 218,
      22,  40,  227, 163, 254, 43,  188, 98,  84,  124, 119, 239, 228, 223, 139,
      171, 242, 229, 53,  156, 42,  167, 155, 69,  36,  45,  183, 121, 161, 22,
      164, 187, 220, 137, 180, 18,  13,  119, 79,  177, 87,  203, 206, 16,  167,
      195, 82,  17,  118, 43,  140, 195, 213, 211, 179, 109, 73,  178, 37,  63,
      109, 84,  109, 180, 122, 198, 65,  43,  173, 205, 32,  156, 131, 234, 27,
      222, 171, 190, 107, 133, 218, 141, 242, 234, 148, 159, 118, 87,  229, 193,
      203, 34,  37,  133, 208, 253, 2,   164, 180, 104, 13,  199, 200, 17,  86,
      209, 78,  199, 202, 240, 167, 42,  173, 28,  214, 154, 181, 91,  215, 46,
      63,  131, 77,  114, 182, 157, 4,   65,  247, 148, 78,  139, 239, 180, 27,
      63,  125, 224, 98,  153, 94,  195, 132, 45,  116, 150, 16,  63,  111, 122,
      197, 6,   91,  182, 143, 32,  9,   111, 172, 1,   134, 215, 224, 197, 95,
      208, 216, 73,  117, 163, 9,   250, 216, 113, 52,  67,  13,  0,   118, 97,
      20,  68,  192, 34,  242, 229, 225, 175, 236, 198, 146, 86,  243, 26,  58,
      251, 170, 150, 53,  165, 220, 11,  174, 110, 16,  207, 31,  199, 19,  120,
      1,   21,  0,   136, 94,  148, 23,  67,  240, 24,  142, 44,  41,  87,  197,
      187, 243, 173, 237, 194, 80,  169, 106, 169, 65,  189, 55,  244, 13,  47,
      126, 224, 191, 126, 195, 71,  195, 137, 53,  124, 246, 215, 177, 15,  8,
      157, 131, 38,  156, 25,  9,   247, 124, 5,   187, 160, 25,  12,  113, 249,
      91,  155, 246, 67,  219, 240, 174, 173, 230, 234, 230, 129, 17,  244, 124,
      203, 27,  176, 247, 17,  152, 5,   26,  27,  134, 218, 125, 208, 84,  61,
      13,  250, 147, 202, 51,  200, 142, 159, 34,  121, 4,   78,  8,   192, 226,
      134, 207, 57,  176, 220, 110, 168, 174, 180, 237, 69,  83,  95,  24,  26,
      52,  134, 96,  27,  244, 108, 14,  146, 97,  239, 235, 92,  222, 102, 246,
      236, 105, 254, 213, 243, 119, 118, 255, 206, 238, 127, 200, 110, 221, 111,
      163, 60,  192, 217, 228, 234, 162, 102, 87,  129, 147, 180, 157, 203, 204,
      70,  241, 166, 110, 147, 36,  51,  178, 139, 105, 231, 216, 246, 252, 116,
      71,  171, 213, 182, 146, 77,  102, 60,  179, 155, 46,  191, 179, 251, 127,
      158, 221, 95,  3,   57,  112, 174, 219, 111, 133, 245, 98,  168, 188, 93,
      212, 55,  51,  196, 182, 126, 210, 238, 24,  74,  117, 149, 191, 27,  149,
      69,  252, 53,  112, 252, 81,  242, 139, 125, 85,  247, 113, 246, 59,  97,
      155, 43,  55,  162, 249, 252, 204, 26,  189, 254, 82,  16,  251, 162, 246,
      189, 5,   169, 4,   68,  82,  10,  194, 63,  254, 248, 19,  8,   50,  153,
      156, 248, 10,  0,   0,
  };
  unsigned char output_buffer[2808] = {
      67,  104, 81,  73,  65,  83,  68,  117, 55,  43,  113, 57,  57,  100, 76,
      85,  108, 90,  73,  66,  75,  73,  102, 90,  48,  80,  118, 114, 75,  120,
      75,  98,  69,  65,  111, 48,  82,  87,  99,  52,  101, 69,  53,  54,  84,
      88,  86,  78,  86,  71,  115, 119, 84,  71,  112, 74,  100, 48,  53,  53,
      78,  72,  104, 78,  97,  109, 78,  90,  81,  85,  82,  66,  81,  107, 57,
      108, 98,  69,  100, 50,  85,  107, 104, 66,  90,  48,  108, 89,  89,  86,
      70,  82,  99,  71,  82,  81,  97,  49,  99,  53,  98,  69,  107, 50,  83,
      66,  114, 86,  66,  103, 103, 69,  69,  105, 115, 73,  65,  104, 111, 72,
      100, 72,  74,  112, 89,  50,  116, 115, 90,  83,  73,  69,  86,  84,  66,
      85,  82,  105, 111, 89,  86,  50,  74,  90,  77,  68,  100, 84,  97,  69,
      74,  118, 86,  110, 82,  75,  85,  107, 57,  71,  90,  67,  56,  118, 86,
      87,  112, 68,  86,  71,  100, 74,  71,  106, 85,  73,  98,  120, 73,  69,
      98,  51,  66,  49,  99,  120, 103, 66,  75,  65,  65,  119, 103, 80,  99,
      67,  79,  65,  74,  67,  68,  103, 111, 73,  98,  87,  108, 117, 99,  72,
      82,  112, 98,  87,  85,  83,  65,  106, 69,  119, 81,  104, 69,  75,  68,
      72,  86,  122, 90,  87,  108, 117, 89,  109, 70,  117, 90,  71,  90,  108,
      89,  120, 73,  66,  77,  82,  111, 82,  67,  71,  99,  83,  66,  69,  108,
      84,  81,  85,  77,  89,  65,  83,  103, 65,  77,  73,  66,  57,  79,  65,
      69,  97,  69,  81,  103, 74,  69,  103, 82,  72,  78,  122, 73,  121, 71,
      65,  69,  111, 65,  68,  68,  65,  80,  106, 103, 66,  71,  104, 69,  73,
      65,  66,  73,  69,  85,  69,  78,  78,  86,  82,  103, 66,  75,  65,  65,
      119, 119, 68,  52,  52,  65,  82,  111, 82,  67,  65,  103, 83,  66,  70,
      66,  68,  84,  85,  69,  89,  65,  83,  103, 65,  77,  77,  65,  43,  79,
      65,  69,  97,  68,  119, 104, 112, 69,  103, 74,  68,  84,  104, 103, 66,
      75,  65,  65,  119, 103, 72,  48,  52,  65,  82,  111, 80,  67,  65,  48,
      83,  65,  107, 78,  79,  71,  65,  69,  111, 65,  68,  68,  65,  80,  106,
      103, 66,  71,  104, 48,  73,  98,  104, 73,  80,  100, 71,  86,  115, 90,
      88,  66,  111, 98,  50,  53,  108, 76,  87,  86,  50,  90,  87,  53,  48,
      71,  65,  69,  111, 65,  68,  67,  65,  57,  119, 73,  52,  65,  82,  111,
      99,  67,  72,  69,  83,  68,  51,  82,  108, 98,  71,  86,  119, 97,  71,
      57,  117, 90,  83,  49,  108, 100, 109, 86,  117, 100, 66,  103, 66,  75,
      65,  65,  119, 103, 72,  48,  52,  65,  82,  111, 99,  67,  72,  52,  83,
      68,  51,  82,  108, 98,  71,  86,  119, 97,  71,  57,  117, 90,  83,  49,
      108, 100, 109, 86,  117, 100, 66,  103, 66,  75,  65,  65,  119, 119, 68,
      52,  52,  65,  82,  111, 110, 67,  71,  85,  83,  66,  69,  103, 121, 78,
      106, 81,  89,  65,  106, 67,  81,  118, 119, 86,  67,  70,  119, 111, 83,
      99,  71,  70,  106, 97,  50,  86,  48,  97,  88,  112, 104, 100, 71,  108,
      118, 98,  105, 49,  116, 98,  50,  82,  108, 69,  103, 69,  120, 71,  103,
      48,  73,  89,  66,  73,  68,  86,  108, 65,  52,  71,  65,  73,  119, 107,
      76,  56,  70,  71,  104, 103, 73,  89,  82,  73,  68,  99,  110, 82,  52,
      71,  65,  73,  119, 107, 76,  56,  70,  81,  103, 107, 75,  65,  50,  70,
      119, 100, 66,  73,  67,  79,  84,  89,  97,  71,  81,  104, 109, 69,  103,
      78,  121, 100, 72,  103, 89,  65,  106, 67,  81,  118, 119, 86,  67,  67,
      103, 111, 68,  89,  88,  66,  48,  69,  103, 77,  120, 77,  68,  69,  97,
      70,  81,  104, 116, 69,  103, 116, 110, 98,  50,  57,  110, 98,  71,  85,
      116, 90,  71,  70,  48,  89,  82,  103, 68,  77,  74,  67,  47,  66,  83,
      73,  51,  67,  65,  69,  83,  76,  50,  108, 117, 98,  71,  108, 117, 90,
      84,  112, 119, 85,  108, 99,  51,  85,  70,  82,  48,  83,  72,  112, 105,
      99,  122, 90,  116, 85,  68,  90,  54,  100, 71,  81,  51,  86,  109, 90,
      52,  90,  108, 90,  89,  84,  49,  78,  69,  78,  107, 120, 119, 75,  49,
      108, 66,  82,  107, 108, 114, 100, 70,  89,  122, 71,  103, 65,  103, 65,
      83,  111, 120, 67,  65,  69,  83,  75,  51,  86,  121, 98,  106, 112, 112,
      90,  88,  82,  109, 79,  110, 66,  104, 99,  109, 70,  116, 99,  122, 112,
      121, 100, 72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  79,  110,
      78,  122, 99,  109, 77,  116, 89,  88,  86,  107, 97,  87,  56,  116, 98,
      71,  86,  50,  90,  87,  119, 89,  65,  83,  111, 111, 67,  65,  73,  83,
      73,  110, 86,  121, 98,  106, 112, 112, 90,  88,  82,  109, 79,  110, 66,
      104, 99,  109, 70,  116, 99,  122, 112, 121, 100, 72,  65,  116, 97,  71,
      82,  121, 90,  88,  104, 48,  79,  110, 82,  118, 90,  109, 90,  122, 90,
      88,  81,  89,  65,  105, 112, 65,  67,  65,  77,  83,  79,  109, 104, 48,
      100, 72,  65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,
      105, 99,  110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,
      66,  108, 99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100,
      72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  50,  70,  105,
      99,  121, 49,  122, 90,  87,  53,  107, 76,  88,  82,  112, 98,  87,  85,
      89,  65,  105, 112, 65,  67,  65,  89,  83,  79,  109, 104, 48,  100, 72,
      65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,  105, 99,
      110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,  66,  108,
      99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100, 72,  65,
      116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  51,  66,  115, 89,  88,
      108, 118, 100, 88,  81,  116, 90,  71,  86,  115, 89,  88,  107, 89,  65,
      105, 112, 70,  67,  65,  99,  83,  80,  50,  104, 48,  100, 72,  65,  54,
      76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,  105, 99,  110, 82,
      106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,  66,  108, 99,  109,
      108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100, 72,  65,  116, 97,
      71,  82,  121, 90,  88,  104, 48,  76,  51,  90,  112, 90,  71,  86,  118,
      76,  87,  78,  118, 98,  110, 82,  108, 98,  110, 81,  116, 100, 72,  108,
      119, 90,  82,  103, 67,  75,  106, 56,  73,  67,  66,  73,  53,  97,  72,
      82,  48,  99,  68,  111, 118, 76,  51,  100, 51,  100, 121, 53,  51,  90,
      87,  74,  121, 100, 71,  77,  117, 98,  51,  74,  110, 76,  50,  86,  52,
      99,  71,  86,  121, 97,  87,  49,  108, 98,  110, 82,  122, 76,  51,  74,
      48,  99,  67,  49,  111, 90,  72,  74,  108, 101, 72,  81,  118, 100, 109,
      108, 107, 90,  87,  56,  116, 100, 71,  108, 116, 97,  87,  53,  110, 71,
      65,  73,  105, 121, 103, 103, 73,  66,  66,  75,  102, 65,  103, 103, 67,
      73,  104, 66,  112, 82,  72,  90,  67,  98,  107, 116, 80,  100, 51,  78,
      116, 100, 85,  100, 72,  77,  107, 57,  104, 75,  104, 104, 75,  89,  88,
      65,  49,  84,  88,  70,  72,  84,  68,  66,  75,  87,  85,  116, 121, 78,
      72,  107, 49,  78,  85,  120, 69,  98,  121, 116, 114, 81,  84,  77,  121,
      73,  119, 103, 66,  69,  65,  69,  97,  68,  122, 69,  51,  77,  121, 52,
      120, 79,  84,  81,  117, 77,  106, 65,  51,  76,  106, 69,  121, 78,  121,
      68,  112, 108, 103, 69,  111, 65,  84,  68,  47,  108, 89,  68,  119, 66,
      48,  65,  65,  77,  105, 77,  73,  65,  82,  65,  67,  71,  103, 56,  120,
      78,  122, 77,  117, 77,  84,  107, 48,  76,  106, 73,  119, 78,  121, 52,
      120, 77,  106, 99,  103, 54,  90,  89,  66,  75,  65,  69,  119, 47,  112,
      87,  65,  56,  65,  100, 65,  65,  68,  73,  105, 67,  65,  69,  81,  65,
      120, 111, 80,  77,  84,  99,  122, 76,  106, 69,  53,  78,  67,  52,  121,
      77,  68,  99,  117, 77,  84,  73,  51,  73,  76,  115, 68,  75,  65,  69,
      119, 47,  90,  87,  65,  56,  65,  100, 65,  65,  68,  73,  113, 67,  65,
      69,  81,  65,  82,  111, 87,  77,  106, 89,  119, 78,  122, 112, 109, 79,
      71,  73,  119, 79,  106, 81,  119, 77,  71,  81,  54,  89,  122, 65,  53,
      79,  106, 111, 51,  90,  105, 68,  112, 108, 103, 69,  111, 65,  84,  68,
      47,  48,  89,  68,  119, 66,  48,  65,  65,  77,  105, 111, 73,  65,  82,
      65,  67,  71,  104, 89,  121, 78,  106, 65,  51,  79,  109, 89,  52,  89,
      106, 65,  54,  78,  68,  65,  119, 90,  68,  112, 106, 77,  68,  107, 54,
      79,  106, 100, 109, 73,  79,  109, 87,  65,  83,  103, 66,  77,  80,  55,
      82,  103, 80,  65,  72,  81,  65,  65,  121, 75,  81,  103, 66,  69,  65,
      77,  97,  70,  106, 73,  50,  77,  68,  99,  54,  90,  106, 104, 105, 77,
      68,  111, 48,  77,  68,  66,  107, 79,  109, 77,  119, 79,  84,  111, 54,
      78,  50,  89,  103, 117, 119, 77,  111, 65,  84,  68,  57,  48,  89,  68,
      119, 66,  48,  65,  65,  71,  106, 85,  73,  98,  120, 73,  69,  98,  51,
      66,  49,  99,  120, 103, 66,  75,  65,  65,  119, 103, 80,  99,  67,  79,
      65,  74,  67,  68,  103, 111, 73,  98,  87,  108, 117, 99,  72,  82,  112,
      98,  87,  85,  83,  65,  106, 69,  119, 81,  104, 69,  75,  68,  72,  86,
      122, 90,  87,  108, 117, 89,  109, 70,  117, 90,  71,  90,  108, 89,  120,
      73,  66,  77,  82,  111, 82,  67,  71,  99,  83,  66,  69,  108, 84,  81,
      85,  77,  89,  65,  83,  103, 65,  77,  73,  66,  57,  79,  65,  69,  97,
      69,  81,  103, 74,  69,  103, 82,  72,  78,  122, 73,  121, 71,  65,  69,
      111, 65,  68,  68,  65,  80,  106, 103, 66,  71,  104, 69,  73,  65,  66,
      73,  69,  85,  69,  78,  78,  86,  82,  103, 66,  75,  65,  65,  119, 119,
      68,  52,  52,  65,  82,  111, 82,  67,  65,  103, 83,  66,  70,  66,  68,
      84,  85,  69,  89,  65,  83,  103, 65,  77,  77,  65,  43,  79,  65,  69,
      97,  68,  119, 104, 112, 69,  103, 74,  68,  84,  104, 103, 66,  75,  65,
      65,  119, 103, 72,  48,  52,  65,  82,  111, 80,  67,  65,  48,  83,  65,
      107, 78,  79,  71,  65,  69,  111, 65,  68,  68,  65,  80,  106, 103, 66,
      71,  104, 48,  73,  98,  104, 73,  80,  100, 71,  86,  115, 90,  88,  66,
      111, 98,  50,  53,  108, 76,  87,  86,  50,  90,  87,  53,  48,  71,  65,
      69,  111, 65,  68,  67,  65,  57,  119, 73,  52,  65,  82,  111, 99,  67,
      72,  69,  83,  68,  51,  82,  108, 98,  71,  86,  119, 97,  71,  57,  117,
      90,  83,  49,  108, 100, 109, 86,  117, 100, 66,  103, 66,  75,  65,  65,
      119, 103, 72,  48,  52,  65,  82,  111, 99,  67,  72,  52,  83,  68,  51,
      82,  108, 98,  71,  86,  119, 97,  71,  57,  117, 90,  83,  49,  108, 100,
      109, 86,  117, 100, 66,  103, 66,  75,  65,  65,  119, 119, 68,  52,  52,
      65,  82,  111, 110, 67,  71,  85,  83,  66,  69,  103, 121, 78,  106, 81,
      89,  65,  106, 67,  81,  118, 119, 86,  67,  70,  119, 111, 83,  99,  71,
      70,  106, 97,  50,  86,  48,  97,  88,  112, 104, 100, 71,  108, 118, 98,
      105, 49,  116, 98,  50,  82,  108, 69,  103, 69,  120, 71,  103, 48,  73,
      89,  66,  73,  68,  86,  108, 65,  52,  71,  65,  73,  119, 107, 76,  56,
      70,  71,  104, 103, 73,  89,  82,  73,  68,  99,  110, 82,  52,  71,  65,
      73,  119, 107, 76,  56,  70,  81,  103, 107, 75,  65,  50,  70,  119, 100,
      66,  73,  67,  79,  84,  89,  97,  71,  81,  104, 109, 69,  103, 78,  121,
      100, 72,  103, 89,  65,  106, 67,  81,  118, 119, 86,  67,  67,  103, 111,
      68,  89,  88,  66,  48,  69,  103, 77,  120, 77,  68,  69,  97,  70,  81,
      104, 116, 69,  103, 116, 110, 98,  50,  57,  110, 98,  71,  85,  116, 90,
      71,  70,  48,  89,  82,  103, 68,  77,  74,  67,  47,  66,  83,  73,  51,
      67,  65,  69,  83,  76,  50,  108, 117, 98,  71,  108, 117, 90,  84,  112,
      50,  78,  107, 82,  97,  79,  69,  86,  116, 75,  50,  90,  104, 77,  110,
      90,  69,  84,  85,  107, 119, 99,  86,  70,  68,  83,  49,  104, 107, 84,
      84,  86,  51,  86,  105, 57,  70,  97,  69,  70,  76,  78,  85,  53,  73,
      100, 122, 70,  67,  84,  49,  86,  51,  71,  103, 65,  103, 73,  121, 111,
      120, 67,  65,  69,  83,  75,  51,  86,  121, 98,  106, 112, 112, 90,  88,
      82,  109, 79,  110, 66,  104, 99,  109, 70,  116, 99,  122, 112, 121, 100,
      72,  65,  116, 97,  71,  82,  121, 90,  88,  104, 48,  79,  110, 78,  122,
      99,  109, 77,  116, 89,  88,  86,  107, 97,  87,  56,  116, 98,  71,  86,
      50,  90,  87,  119, 89,  65,  83,  111, 111, 67,  65,  73,  83,  73,  110,
      86,  121, 98,  106, 112, 112, 90,  88,  82,  109, 79,  110, 66,  104, 99,
      109, 70,  116, 99,  122, 112, 121, 100, 72,  65,  116, 97,  71,  82,  121,
      90,  88,  104, 48,  79,  110, 82,  118, 90,  109, 90,  122, 90,  88,  81,
      89,  65,  105, 112, 65,  67,  65,  77,  83,  79,  109, 104, 48,  100, 72,
      65,  54,  76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,  105, 99,
      110, 82,  106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,  66,  108,
      99,  109, 108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100, 72,  65,
      116, 97,  71,  82,  121, 90,  88,  104, 48,  76,  50,  70,  105, 99,  121,
      49,  122, 90,  87,  53,  107, 76,  88,  82,  112, 98,  87,  85,  89,  65,
      105, 112, 65,  67,  65,  89,  83,  79,  109, 104, 48,  100, 72,  65,  54,
      76,  121, 57,  51,  100, 51,  99,  117, 100, 50,  86,  105, 99,  110, 82,
      106, 76,  109, 57,  121, 90,  121, 57,  108, 101, 72,  66,  108, 99,  109,
      108, 116, 90,  87,  53,  48,  99,  121, 57,  121, 100, 72,  65,  116, 97,
      71,  82,  121, 90,  88,  104, 48,  76,  51,  66,  115, 89,  88,  108, 118,
      100, 88,  81,  116, 90,  71,  86,  115, 89,  88,  107, 89,  65,  105, 112,
      70,  67,  65,  99,  83,  80,  50,  104, 48,  100, 72,  65,  54,  76,  121,
      57,  51,  100, 51,  99,  117, 100, 50,  86,  105, 99,  110, 82,  106, 76,
      109, 57,  121, 90,  121, 57,  108, 101, 72,  66,  108, 99,  109, 108, 116,
      90,  87,  53,  48,  99,  121, 57,  121, 100, 72,  65,  116, 97,  71,  82,
      121, 90,  88,  104, 48,  76,  51,  90,  112, 90,  71,  86,  118, 76,  87,
      78,  118, 98,  110, 82,  108, 98,  110, 81,  116, 100, 72,  108, 119, 90,
      82,  103, 67,  75,  106, 56,  73,  67,  66,  73,  53,  97,  72,  82,  48,
      99,  68,  111, 118, 76,  51,  100, 51,  100, 121, 53,  51,  90,  87,  74,
      121, 100, 71,  77,  117, 98,  51,  74,  110, 76,  50,  86,  52,  99,  71,
      86,  121, 97,  87,  49,  108, 98,  110, 82,  122, 76,  51,  74,  48,  99,
      67,  49,  111, 90,  72,  74,  108, 101, 72,  81,  118, 100, 109, 108, 107,
      90,  87,  56,  116, 100, 71,  108, 116, 97,  87,  53,  110, 71,  65,  73,
      113, 79,  103, 111, 89,  89,  50,  70,  115, 98,  67,  56,  51,  79,  84,
      81,  121, 79,  84,  81,  119, 77,  68,  77,  52,  78,  84,  107, 53,  77,
      68,  85,  50,  77,  106, 65,  49,  69,  65,  77,  89,  114, 79,  72,  78,
      112, 103, 89,  105, 70,  110, 86,  117, 100, 88,  78,  108, 90,  70,  57,
      122, 89,  51,  82,  119, 88,  51,  66,  49,  99,  50,  104, 102, 98,  71,
      70,  105, 90,  87,  119, 52,  65,  85,  103, 65,  71,  103, 73,  73,  65,
      81,  61,  61,
  };

  source()->AddReadResult(reinterpret_cast<const char*>(input_buffer),
                          sizeof(input_buffer), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(sizeof(output_buffer)), rv);
  EXPECT_EQ(0, memcmp(output_buffer, actual_output.c_str(), rv));
  EXPECT_EQ("GZIP", stream()->Description());
}

// Test with the same compressed data as the above tests, but uses deflate with
// header and checksum. Tests the Z_STREAM_END case in
// STATE_SNIFFING_DEFLATE_HEADER.
TEST_P(GzipSourceStreamTest, DeflateWithAdler32) {
  Init(SourceStream::TYPE_DEFLATE);
  const char kDecompressedData[] = "Hello, World!";
  const unsigned char kGzipData[] = {0x78, 0x01, 0xf3, 0x48, 0xcd, 0xc9, 0xc9,
                                     0xd7, 0x51, 0x08, 0xcf, 0x2f, 0xca, 0x49,
                                     0x51, 0x04, 0x00, 0x1f, 0x9e, 0x04, 0x6a};
  source()->AddReadResult(reinterpret_cast<const char*>(kGzipData),
                          sizeof(kGzipData), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(strlen(kDecompressedData)), rv);
  EXPECT_EQ(kDecompressedData, actual_output);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, DeflateWithBadAdler32) {
  Init(SourceStream::TYPE_DEFLATE);
  const unsigned char kGzipData[] = {0x78, 0x01, 0xf3, 0x48, 0xcd, 0xc9, 0xc9,
                                     0xd7, 0x51, 0x08, 0xcf, 0x2f, 0xca, 0x49,
                                     0x51, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
  source()->AddReadResult(reinterpret_cast<const char*>(kGzipData),
                          sizeof(kGzipData), OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(ERR_CONTENT_DECODING_FAILED, rv);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, DeflateWithoutHeaderWithAdler32) {
  Init(SourceStream::TYPE_DEFLATE);
  const char kDecompressedData[] = "Hello, World!";
  const unsigned char kGzipData[] = {0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0xd7, 0x51,
                                     0x08, 0xcf, 0x2f, 0xca, 0x49, 0x51, 0x04,
                                     0x00, 0x1f, 0x9e, 0x04, 0x6a};
  source()->AddReadResult(reinterpret_cast<const char*>(kGzipData),
                          sizeof(kGzipData), OK, GetParam().mode);
  source()->AddReadResult(nullptr, 0, OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(static_cast<int>(strlen(kDecompressedData)), rv);
  EXPECT_EQ(kDecompressedData, actual_output);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

TEST_P(GzipSourceStreamTest, DeflateWithoutHeaderWithBadAdler32) {
  Init(SourceStream::TYPE_DEFLATE);
  const unsigned char kGzipData[] = {0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0xd7, 0x51,
                                     0x08, 0xcf, 0x2f, 0xca, 0x49, 0x51, 0x04,
                                     0x00, 0xFF, 0xFF, 0xFF, 0xFF};
  source()->AddReadResult(reinterpret_cast<const char*>(kGzipData),
                          sizeof(kGzipData), OK, GetParam().mode);
  std::string actual_output;
  int rv = ReadStream(&actual_output);
  EXPECT_EQ(ERR_CONTENT_DECODING_FAILED, rv);
  EXPECT_EQ("DEFLATE", stream()->Description());
}

}  // namespace net
