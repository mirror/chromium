// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_sink.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

using vec = std::vector<uint8_t>;

template <class T>
void TestSerialize(const std::vector<uint8_t>& expected, const T& value) {
  size_t size = SerializedSize(value);
  EXPECT_EQ(expected.size(), size);
  std::vector<uint8_t> buffer(size);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_TRUE(SerializeInto(value, &buffer_sink));
  EXPECT_EQ(expected, buffer);
}

template <class T>
void TestSerializeMember(const vec& expected, const T& value) {
  size_t size = value.SerializedSize();
  EXPECT_EQ(expected.size(), size);
  std::vector<uint8_t> buffer(size);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_TRUE(value.SerializeInto(&buffer_sink));
  EXPECT_EQ(expected, buffer);
}

TEST(Serialize, ElementMatch) {
  ElementMatch element_match = {{EXE_TYPE_DEX, 1, 2}, {EXE_TYPE_DEX, 3, 4}};

  TestSerialize(
      {
          1, 0, 0, 0,              // old_offset
          3, 0, 0, 0,              // new_offset
          2, 0, 0, 0, 0, 0, 0, 0,  // old_length
          4, 0, 0, 0, 0, 0, 0, 0,  // new_length
          6, 0, 0, 0,              // EXE_TYPE_DEX
      },
      element_match);
}

TEST(Serialize, ElementMatchExeMismatch) {
  std::vector<uint8_t> buffer(28);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_FALSE(SerializeInto(
      ElementMatch{{EXE_TYPE_NO_OP, 1, 2}, {EXE_TYPE_WIN32_X86, 3, 4}},
      &buffer_sink));
}

TEST(Serialize, ElementMatchTooSmall) {
  std::vector<uint8_t> buffer(4);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_FALSE(SerializeInto(
      ElementMatch{{EXE_TYPE_DEX, 1, 2}, {EXE_TYPE_DEX, 3, 4}}, &buffer_sink));
}

TEST(Serialize, Vector) {
  TestSerialize({0, 0, 0, 0, 0, 0, 0, 0}, vec({}));
  TestSerialize({3, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3}, vec({1, 2, 3}));
}

TEST(Serialize, VectorTooSmall) {
  std::vector<uint8_t> buffer(4);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_FALSE(SerializeInto(vec(), &buffer_sink));
}

TEST(EquivalenceSinkTest, Serialize) {
  EquivalenceSink equivalence_sink;
  TestSerializeMember(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      equivalence_sink);

  equivalence_sink.PutNext(Equivalence{3, 7, 2});
  TestSerializeMember({1, 0, 0, 0, 0, 0, 0, 0, 6, 1, 0, 0, 0, 0,
                       0, 0, 0, 7, 1, 0, 0, 0, 0, 0, 0, 0, 2},
                      equivalence_sink);

  equivalence_sink.PutNext(Equivalence{1, 10, 1});
  TestSerializeMember({2, 0, 0, 0, 0, 0, 0, 0, 6, 7, 2, 0, 0, 0, 0,
                       0, 0, 0, 7, 1, 2, 0, 0, 0, 0, 0, 0, 0, 2, 1},
                      equivalence_sink);
}

TEST(ExtraDataSinkTest, Serialize) {
  ExtraDataSink extra_data_sink;
  TestSerializeMember(
      {
          0, 0, 0, 0, 0, 0, 0, 0,
      },
      extra_data_sink);

  vec data = {1, 2, 3};
  extra_data_sink.PutNext({data.data(), data.size()});
  TestSerializeMember(
      {
          3, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3,
      },
      extra_data_sink);

  data = {4, 5};
  extra_data_sink.PutNext({data.data(), data.size()});
  TestSerializeMember(
      {
          5, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
      },
      extra_data_sink);
}

TEST(RawDeltaSinkTest, Serialize) {
  RawDeltaSink raw_delta_sink;
  TestSerializeMember(
      {
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      },
      raw_delta_sink);

  raw_delta_sink.PutNext({1, 42});
  TestSerializeMember(
      {
          1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 42,
      },
      raw_delta_sink);

  raw_delta_sink.PutNext({3, 24});
  TestSerializeMember(
      {
          2, 0, 0, 0, 0, 0, 0, 0, 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 42, 24,
      },
      raw_delta_sink);
}

TEST(ReferenceDeltaSinkTest, Serialize) {
  ReferenceDeltaSink reference_delta;
  TestSerializeMember(
      {
          0, 0, 0, 0, 0, 0, 0, 0,
      },
      reference_delta);

  reference_delta.PutNext(42);
  TestSerializeMember(
      {
          1, 0, 0, 0, 0, 0, 0, 0, 84,
      },
      reference_delta);

  reference_delta.PutNext(-24);
  TestSerializeMember(
      {
          2, 0, 0, 0, 0, 0, 0, 0, 84, 47,
      },
      reference_delta);
}

TEST(TargetSinkTest, Serialize) {
  TargetSink target_sink;
  TestSerializeMember(
      {
          0, 0, 0, 0, 0, 0, 0, 0,
      },
      target_sink);

  target_sink.PutNext(3);
  TestSerializeMember(
      {
          1, 0, 0, 0, 0, 0, 0, 0, 3,
      },
      target_sink);

  target_sink.PutNext(5);
  TestSerializeMember(
      {
          2, 0, 0, 0, 0, 0, 0, 0, 3, 2,
      },
      target_sink);
}

TEST(ElementSinkPatchTest, SerializeEmpty) {
  ElementMatch element_match = {{EXE_TYPE_WIN32_X86, 0x01, 0x02},
                                {EXE_TYPE_WIN32_X86, 0x03, 0x04}};
  ElementPatchSink element_sink_patch(element_match);

  element_sink_patch.SetEquivalenceSink(
      EquivalenceSink({0x10}, {0x11}, {0x12}));
  element_sink_patch.SetExtraDataSink(ExtraDataSink({0x13}));
  element_sink_patch.SetRawDeltaSink(RawDeltaSink({0x14}, {0x15}));
  element_sink_patch.SetReferenceDeltaSink(ReferenceDeltaSink({0x16}));
  element_sink_patch.SetTargetSink(0, TargetSink({0x17}));
  element_sink_patch.SetTargetSink(2, TargetSink({0x18}));

  TestSerializeMember(
      {
          0x01, 0, 0, 0,              // old_offset
          0x03, 0, 0, 0,              // new_offset
          0x02, 0, 0, 0, 0, 0, 0, 0,  // old_length
          0x04, 0, 0, 0, 0, 0, 0, 0,  // new_length
          1,    0, 0, 0,              // EXE_TYPE_WIN32_X86

          1,    0, 0, 0, 0, 0, 0, 0, 0x10, 1,    0, 0, 0, 0, 0, 0, 0, 0x11,
          1,    0, 0, 0, 0, 0, 0, 0, 0x12,

          1,    0, 0, 0, 0, 0, 0, 0, 0x13,

          1,    0, 0, 0, 0, 0, 0, 0, 0x14, 1,    0, 0, 0, 0, 0, 0, 0, 0x15,

          1,    0, 0, 0, 0, 0, 0, 0, 0x16,

          2,    0, 0, 0, 0, 0, 0, 0, 0,    1,    0, 0, 0, 0, 0, 0, 0, 0x17,
          2,    1, 0, 0, 0, 0, 0, 0, 0,    0x18,
      },
      element_sink_patch);
}

}  // namespace zucchini
