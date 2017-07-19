// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_reader.h"
#include "chrome/installer/zucchini/patch_writer.h"

#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

using vec = std::vector<uint8_t>;

bool operator==(const vec& a, ConstBufferView b) {
  return a == vec(b.begin(), b.end());
}

template <class T>
T TestInitialize(const std::vector<uint8_t>* buffer) {
  T value;
  BufferSource buffer_source(buffer->data(), buffer->size());
  EXPECT_TRUE(value.Initialize(&buffer_source));
  EXPECT_TRUE(buffer_source.empty());  // Make sure all data has been consumed
  return value;
}

template <class T>
void TestInvalidInitialize(const std::vector<uint8_t>* buffer) {
  T value;
  BufferSource buffer_source(buffer->data(), buffer->size());
  EXPECT_FALSE(value.Initialize(&buffer_source));
}

template <class T>
void TestSerialize(const vec& expected, const T& value) {
  size_t size = value.SerializedSize();
  EXPECT_EQ(expected.size(), size);
  std::vector<uint8_t> buffer(size);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_TRUE(value.SerializeInto(&buffer_sink));
  EXPECT_EQ(expected, buffer);
}

TEST(PatchTest, ParseSerializeElementMatch) {
  vec data = {
      1, 0, 0, 0,              // old_offset
      3, 0, 0, 0,              // new_offset
      2, 0, 0, 0, 0, 0, 0, 0,  // old_length
      4, 0, 0, 0, 0, 0, 0, 0,  // new_length
      6, 0, 0, 0,              // kExeTypeDex
  };
  BufferSource buffer_source(data.data(), data.size());
  ElementMatch element_match = {};
  EXPECT_TRUE(patch::ParseElementMatch(&buffer_source, &element_match));

  EXPECT_EQ(kExeTypeDex, element_match.old_element.exe_type);
  EXPECT_EQ(kExeTypeDex, element_match.new_element.exe_type);
  EXPECT_EQ(size_t(1), element_match.old_element.offset);
  EXPECT_EQ(size_t(2), element_match.old_element.length);
  EXPECT_EQ(size_t(3), element_match.new_element.offset);
  EXPECT_EQ(size_t(4), element_match.new_element.length);

  size_t size = patch::SerializedElementMatchSize(element_match);
  EXPECT_EQ(data.size(), size);
  std::vector<uint8_t> buffer(size);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_TRUE(patch::SerializeElementMatch(element_match, &buffer_sink));
  EXPECT_EQ(data, buffer);
}

TEST(PatchTest, ParseElementMatchTooSmall) {
  vec data = {4};
  BufferSource buffer_source(data.data(), data.size());
  ElementMatch element_match = {};
  EXPECT_FALSE(patch::ParseElementMatch(&buffer_source, &element_match));
}

TEST(PatchTest, ParseSerializeElementMatchExeMismatch) {
  std::vector<uint8_t> buffer(28);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_FALSE(patch::SerializeElementMatch(
      ElementMatch{{kExeTypeNoOp, 1, 2}, {kExeTypeWin32X86, 3, 4}},
      &buffer_sink));
}

TEST(PatchTest, SerializeElementMatchTooSmall) {
  std::vector<uint8_t> buffer(4);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_FALSE(patch::SerializeElementMatch(
      ElementMatch{{kExeTypeDex, 1, 2}, {kExeTypeDex, 3, 4}}, &buffer_sink));
}

TEST(PatchTest, ParseSerializeBuffer) {
  auto TestSerialize = [](const std::vector<uint8_t>& expected,
                          const std::vector<uint8_t>& value) {
    size_t size = patch::SerializedBufferSize(value);
    EXPECT_EQ(expected.size(), size);
    std::vector<uint8_t> buffer(size);
    BufferSink buffer_sink(buffer.data(), buffer.size());
    EXPECT_TRUE(patch::SerializeBuffer(value, &buffer_sink));
    EXPECT_EQ(expected, buffer);
  };

  auto TestParse = [](const std::vector<uint8_t>* data) {
    BufferSource value;
    BufferSource buffer_source(data->data(), data->size());
    EXPECT_TRUE(patch::ParseBuffer(&buffer_source, &value));
    EXPECT_TRUE(buffer_source.empty());  // Make sure all data has been consumed
    return value;
  };

  vec data = {
      0, 0, 0, 0, 0, 0, 0, 0  // size
  };
  BufferSource buffer = TestParse(&data);
  EXPECT_TRUE(buffer.empty());
  TestSerialize(data, vec({}));

  data = {
      3, 0, 0, 0, 0, 0, 0, 0,  // size
      1, 2, 3                  // content
  };
  buffer = TestParse(&data);
  EXPECT_EQ(3U, buffer.size());
  EXPECT_EQ(vec({1, 2, 3}), vec(buffer.begin(), buffer.end()));
  TestSerialize(data, vec({1, 2, 3}));

  // Ill-formed input.
  data = {
      3, 0, 0, 0, 0, 0, 0, 0,  // size
      1, 2                     // insufficient content
  };
  BufferSource value;
  BufferSource buffer_source(data.data(), data.size());
  EXPECT_FALSE(patch::ParseBuffer(&buffer_source, &value));
  EXPECT_TRUE(value.empty());
}

TEST(PatchTest, SerializeBufferTooSmall) {
  std::vector<uint8_t> buffer(7);
  BufferSink buffer_sink(buffer.data(), buffer.size());
  EXPECT_FALSE(patch::SerializeBuffer(vec(), &buffer_sink));
}

TEST(EquivalenceSinkSourceTest, Empty) {
  vec data = {
      0, 0, 0, 0, 0, 0, 0, 0,  // src_skip size
      0, 0, 0, 0, 0, 0, 0, 0,  // dst_skip size
      0, 0, 0, 0, 0, 0, 0, 0   // copy_count size
  };
  EquivalenceSource equivalence_source =
      TestInitialize<EquivalenceSource>(&data);

  EXPECT_FALSE(equivalence_source.GetNext());
  EXPECT_TRUE(equivalence_source.Done());

  TestSerialize(data, EquivalenceSink());
}

TEST(EquivalenceSourceSinkTest, Normal) {
  vec data = {
      2, 0, 0, 0, 0, 0, 0, 0,  // src_skip size
      6, 7,                    // src_skip content
      2, 0, 0, 0, 0, 0, 0, 0,  // dst_skip size
      7, 1,                    // dst_skip content
      2, 0, 0, 0, 0, 0, 0, 0,  // copy_count size
      2, 1                     // copy_count content
  };
  EquivalenceSource equivalence_source =
      TestInitialize<EquivalenceSource>(&data);
  auto equivalence = equivalence_source.GetNext();
  EXPECT_FALSE(equivalence_source.Done());
  EXPECT_TRUE(equivalence.has_value());
  EXPECT_EQ(offset_t(3), equivalence->src_offset);
  EXPECT_EQ(offset_t(7), equivalence->dst_offset);
  EXPECT_EQ(offset_t(2), equivalence->length);

  equivalence = equivalence_source.GetNext();
  EXPECT_TRUE(equivalence_source.Done());
  EXPECT_TRUE(equivalence.has_value());
  EXPECT_EQ(offset_t(1), equivalence->src_offset);
  EXPECT_EQ(offset_t(10), equivalence->dst_offset);
  EXPECT_EQ(offset_t(1), equivalence->length);

  equivalence = equivalence_source.GetNext();
  EXPECT_FALSE(equivalence.has_value());

  EquivalenceSink equivalence_sink;
  equivalence_sink.PutNext(Equivalence{3, 7, 2});
  equivalence_sink.PutNext(Equivalence{1, 10, 1});
  TestSerialize(data, equivalence_sink);
}

TEST(ExtraDataSourceSinkTest, Empty) {
  vec data = {
      0, 0, 0, 0, 0, 0, 0, 0,  // extra_data size
  };
  ExtraDataSource extra_data_source = TestInitialize<ExtraDataSource>(&data);

  EXPECT_FALSE(extra_data_source.GetNext(2));
  EXPECT_TRUE(extra_data_source.Done());

  TestSerialize(data, ExtraDataSink());
}

TEST(ExtraDataSourceSinkTest, Normal) {
  vec data = {
      5, 0, 0, 0, 0, 0, 0, 0,  // extra_data size
      1, 2, 3, 4, 5,           // extra_data content
  };
  ExtraDataSource extra_data_source = TestInitialize<ExtraDataSource>(&data);
  EXPECT_FALSE(extra_data_source.Done());

  auto extra_data = extra_data_source.GetNext(3);
  EXPECT_FALSE(extra_data_source.Done());
  EXPECT_TRUE(extra_data.has_value());
  EXPECT_EQ(size_t(3), extra_data->size());
  EXPECT_EQ(vec({1, 2, 3}), vec(extra_data->begin(), extra_data->end()));

  extra_data = extra_data_source.GetNext(2);
  EXPECT_TRUE(extra_data_source.Done());
  EXPECT_TRUE(extra_data.has_value());
  EXPECT_EQ(vec({4, 5}), vec(extra_data->begin(), extra_data->end()));

  extra_data = extra_data_source.GetNext(2);
  EXPECT_FALSE(extra_data.has_value());

  ExtraDataSink extra_data_sink;

  vec content = {1, 2, 3};
  extra_data_sink.PutNext({content.data(), content.size()});
  content = {4, 5};
  extra_data_sink.PutNext({content.data(), content.size()});
  TestSerialize(data, extra_data_sink);
}

TEST(RawDeltaSourceSinkTest, Empty) {
  vec data = {
      0, 0, 0, 0, 0, 0, 0, 0,  // raw_delta_skip size
      0, 0, 0, 0, 0, 0, 0, 0,  // raw_delta_diff size
  };
  RawDeltaSource raw_delta_source = TestInitialize<RawDeltaSource>(&data);

  EXPECT_FALSE(raw_delta_source.GetNext());
  EXPECT_TRUE(raw_delta_source.Done());

  TestSerialize(data, RawDeltaSink());
}

TEST(RawDeltaSinkSourceSinkTest, Normal) {
  vec data = {
      2,  0,  0, 0, 0, 0, 0, 0,  // raw_delta_skip size
      1,  3,                     // raw_delta_skip content
      2,  0,  0, 0, 0, 0, 0, 0,  // raw_delta_diff size
      42, 24,                    // raw_delta_diff content
  };
  RawDeltaSource raw_delta_source = TestInitialize<RawDeltaSource>(&data);
  EXPECT_FALSE(raw_delta_source.Done());

  auto raw_delta = raw_delta_source.GetNext();
  EXPECT_FALSE(raw_delta_source.Done());
  EXPECT_TRUE(raw_delta.has_value());
  EXPECT_EQ(1U, raw_delta->copy_offset);
  EXPECT_EQ(42, raw_delta->diff);

  raw_delta = raw_delta_source.GetNext();
  EXPECT_TRUE(raw_delta_source.Done());
  EXPECT_TRUE(raw_delta.has_value());
  EXPECT_EQ(3U, raw_delta->copy_offset);
  EXPECT_EQ(24, raw_delta->diff);

  EXPECT_FALSE(raw_delta_source.GetNext());
  EXPECT_TRUE(raw_delta_source.Done());

  RawDeltaSink raw_delta_sink;
  raw_delta_sink.PutNext({1, 42});
  raw_delta_sink.PutNext({3, 24});
  TestSerialize(data, raw_delta_sink);
}

TEST(ReferenceDeltaSourceSinkTest, Empty) {
  vec data = {
      0, 0, 0, 0, 0, 0, 0, 0,  // reference_delta size
  };
  ReferenceDeltaSource reference_delta_source =
      TestInitialize<ReferenceDeltaSource>(&data);

  EXPECT_FALSE(reference_delta_source.GetNext());
  EXPECT_TRUE(reference_delta_source.Done());

  TestSerialize(data, ReferenceDeltaSink());
}

TEST(ReferenceDeltaSourceSinkTest, Normal) {
  vec data = {
      2,  0,  0, 0, 0, 0, 0, 0,  // reference_delta size
      84, 47,                    // reference_delta content
  };
  ReferenceDeltaSource reference_delta_source =
      TestInitialize<ReferenceDeltaSource>(&data);
  EXPECT_FALSE(reference_delta_source.Done());

  auto delta = reference_delta_source.GetNext();
  EXPECT_FALSE(reference_delta_source.Done());
  EXPECT_TRUE(delta.has_value());
  EXPECT_EQ(42, *delta);

  delta = reference_delta_source.GetNext();
  EXPECT_TRUE(reference_delta_source.Done());
  EXPECT_TRUE(delta.has_value());
  EXPECT_EQ(-24, *delta);

  EXPECT_FALSE(reference_delta_source.GetNext());
  EXPECT_TRUE(reference_delta_source.Done());

  ReferenceDeltaSink reference_delta;
  reference_delta.PutNext(42);
  reference_delta.PutNext(-24);
  TestSerialize(data, reference_delta);
}

TEST(TargetSourceSinkTest, Empty) {
  vec data = {
      0, 0, 0, 0, 0, 0, 0, 0,  // extra_targets size
  };
  TargetSource target_source = TestInitialize<TargetSource>(&data);

  EXPECT_FALSE(target_source.GetNext());
  EXPECT_TRUE(target_source.Done());

  TestSerialize(data, TargetSink());
}

TEST(TargetSourceSinkTest, Normal) {
  vec data = {
      2, 0, 0, 0, 0, 0, 0, 0,  // extra_targets size
      3, 1,                    // extra_targets content
  };
  TargetSource target_source = TestInitialize<TargetSource>(&data);
  EXPECT_FALSE(target_source.Done());

  auto target = target_source.GetNext();
  EXPECT_FALSE(target_source.Done());
  EXPECT_TRUE(target.has_value());
  EXPECT_EQ(3U, *target);

  target = target_source.GetNext();
  EXPECT_TRUE(target_source.Done());
  EXPECT_TRUE(target.has_value());
  EXPECT_EQ(5U, *target);

  EXPECT_FALSE(target_source.GetNext());
  EXPECT_TRUE(target_source.Done());

  TargetSink target_sink;
  target_sink.PutNext(3);
  target_sink.PutNext(5);
  TestSerialize(data, target_sink);
}

TEST(PatchElementTest, Normal) {
  vec data = {
      0x01, 0, 0, 0,              // old_offset
      0x03, 0, 0, 0,              // new_offset
      0x02, 0, 0, 0, 0, 0, 0, 0,  // old_length
      0x04, 0, 0, 0, 0, 0, 0, 0,  // new_length
      1,    0, 0, 0,              // EXE_TYPE_WIN32_X86

      1,    0, 0, 0, 0, 0, 0, 0,  // src_skip size
      0x10,                       // src_skip content
      1,    0, 0, 0, 0, 0, 0, 0,  // dst_skip size
      0x11,                       // dst_skip content
      1,    0, 0, 0, 0, 0, 0, 0,  // copy_count size
      0x12,                       // copy_count content

      1,    0, 0, 0, 0, 0, 0, 0,  // extra_data size
      0x13,                       // extra_data content

      1,    0, 0, 0, 0, 0, 0, 0,  // raw_delta_skip size
      0x14,                       // raw_delta_skip content
      1,    0, 0, 0, 0, 0, 0, 0,  // raw_delta_diff size
      0x15,                       // raw_delta_diff content

      1,    0, 0, 0, 0, 0, 0, 0,  // reference_delta size
      0x16,                       // reference_delta content

      2,    0, 0, 0, 0, 0, 0, 0,  // pool count
      0,                          // pool_tag
      1,    0, 0, 0, 0, 0, 0, 0,  // extra_targets size
      0x17,                       // extra_targets content
      2,                          // pool_tag
      1,    0, 0, 0, 0, 0, 0, 0,  // extra_targets size
      0x18,                       // extra_targets content
  };

  PatchElementReader patch_element_reader =
      TestInitialize<PatchElementReader>(&data);

  ElementMatch element_match = patch_element_reader.element_match();
  EXPECT_EQ(kExeTypeWin32X86, element_match.old_element.exe_type);
  EXPECT_EQ(0x01U, element_match.old_element.offset);
  EXPECT_EQ(0x02U, element_match.old_element.length);
  EXPECT_EQ(kExeTypeWin32X86, element_match.new_element.exe_type);
  EXPECT_EQ(0x03U, element_match.new_element.offset);
  EXPECT_EQ(0x04U, element_match.new_element.length);

  EXPECT_EQ(3U, patch_element_reader.pool_count());

  EquivalenceSource equivalence_source =
      patch_element_reader.GetEquivalenceSource();
  EXPECT_EQ(vec({0x10}), equivalence_source.src_skip());
  EXPECT_EQ(vec({0x11}), equivalence_source.dst_skip());
  EXPECT_EQ(vec({0x12}), equivalence_source.copy_count());

  ExtraDataSource extra_data_source = patch_element_reader.GetExtraDataSource();
  EXPECT_EQ(vec({0x13}), extra_data_source.extra_data());

  RawDeltaSource raw_delta_source = patch_element_reader.GetRawDeltaSource();
  EXPECT_EQ(vec({0x14}), raw_delta_source.raw_delta_skip());
  EXPECT_EQ(vec({0x15}), raw_delta_source.raw_delta_diff());

  ReferenceDeltaSource reference_delta_source =
      patch_element_reader.GetReferenceDeltaSource();
  EXPECT_EQ(vec({0x16}), reference_delta_source.reference_delta());

  TargetSource target_source1 =
      patch_element_reader.GetExtraTargetSource(PoolTag(0));
  EXPECT_EQ(vec({0x17}), target_source1.extra_targets());
  TargetSource target_source2 =
      patch_element_reader.GetExtraTargetSource(PoolTag(1));
  EXPECT_EQ(vec({}), target_source2.extra_targets());
  TargetSource target_source3 =
      patch_element_reader.GetExtraTargetSource(PoolTag(2));
  EXPECT_EQ(vec({0x18}), target_source3.extra_targets());

  PatchElementWriter patch_element_writer(element_match);

  patch_element_writer.SetEquivalenceSink(
      EquivalenceSink({0x10}, {0x11}, {0x12}));
  patch_element_writer.SetExtraDataSink(ExtraDataSink({0x13}));
  patch_element_writer.SetRawDeltaSink(RawDeltaSink({0x14}, {0x15}));
  patch_element_writer.SetReferenceDeltaSink(ReferenceDeltaSink({0x16}));
  patch_element_writer.SetTargetSink(PoolTag(0), TargetSink({0x17}));
  patch_element_writer.SetTargetSink(PoolTag(2), TargetSink({0x18}));
  TestSerialize(data, patch_element_writer);
}

TEST(EnsemblePatchTest, RawPatch) {
  vec data = {
      0x5A, 0x75, 0x63, 0x00,  // magic
      0x10, 0x32, 0x54, 0x76,  // old_size
      0x00, 0x11, 0x22, 0x33,  // old_crc
      0x98, 0xBA, 0xDC, 0xFE,  // new_size
      0x44, 0x55, 0x66, 0x77,  // new_crc

      0,    0,    0,    0,  // kRawPatch

      1,    0,    0,    0,    0, 0, 0, 0,  // number of element

      0x01, 0,    0,    0,                 // old_offset
      0x03, 0,    0,    0,                 // new_offset
      0x02, 0,    0,    0,    0, 0, 0, 0,  // old_length
      0x04, 0,    0,    0,    0, 0, 0, 0,  // new_length
      1,    0,    0,    0,                 // EXE_TYPE_WIN32_X86
      0,    0,    0,    0,    0, 0, 0, 0,  // src_skip size
      0,    0,    0,    0,    0, 0, 0, 0,  // dst_skip size
      0,    0,    0,    0,    0, 0, 0, 0,  // copy_count size
      0,    0,    0,    0,    0, 0, 0, 0,  // extra_data size
      0,    0,    0,    0,    0, 0, 0, 0,  // raw_delta_skip size
      0,    0,    0,    0,    0, 0, 0, 0,  // raw_delta_diff size
      0,    0,    0,    0,    0, 0, 0, 0,  // reference_delta size
      0,    0,    0,    0,    0, 0, 0, 0,  // pool count
  };

  EnsemblePatchReader ensemble_patch_reader =
      TestInitialize<EnsemblePatchReader>(&data);

  PatchHeader header = ensemble_patch_reader.header();
  EXPECT_EQ(PatchHeader::kMagic, header.magic);
  EXPECT_EQ(header.old_size, 0x76543210U);
  EXPECT_EQ(header.old_crc, 0x33221100U);
  EXPECT_EQ(header.new_size, 0xFEDCBA98U);
  EXPECT_EQ(header.new_crc, 0x77665544U);

  EXPECT_EQ(ensemble_patch_reader.patch_type(), PatchType::kRawPatch);

  std::vector<PatchElementReader> elements = ensemble_patch_reader.elements();
  EXPECT_EQ(size_t(1), elements.size());

  EnsemblePatchWriter ensemble_patch_writer(header);
  ensemble_patch_writer.SetPatchType(PatchType::kRawPatch);
  ensemble_patch_writer.AddElement(
      PatchElementWriter(elements[0].element_match()));

  TestSerialize(data, ensemble_patch_writer);
}

TEST(EnsemblePatchTest, InvalidMagic) {
  vec data = {
      0x42, 0x42, 0x42, 0x00,  // magic
      0x10, 0x32, 0x54, 0x76,  // old_size
      0x00, 0x11, 0x22, 0x33,  // old_crc
      0x98, 0xBA, 0xDC, 0xFE,  // new_size
      0x44, 0x55, 0x66, 0x77,  // new_crc
      0,    0,    0,    0,     // kRawPatch

      1,    0,    0,    0,    0, 0, 0, 0,  // number of element

      0x01, 0,    0,    0,                 // old_offset
      0x03, 0,    0,    0,                 // new_offset
      0x02, 0,    0,    0,    0, 0, 0, 0,  // old_length
      0x04, 0,    0,    0,    0, 0, 0, 0,  // new_length
      1,    0,    0,    0,                 // EXE_TYPE_WIN32_X86
      0,    0,    0,    0,    0, 0, 0, 0,  // src_skip size
      0,    0,    0,    0,    0, 0, 0, 0,  // dst_skip size
      0,    0,    0,    0,    0, 0, 0, 0,  // copy_count size
      0,    0,    0,    0,    0, 0, 0, 0,  // extra_data size
      0,    0,    0,    0,    0, 0, 0, 0,  // raw_delta_skip size
      0,    0,    0,    0,    0, 0, 0, 0,  // raw_delta_diff size
      0,    0,    0,    0,    0, 0, 0, 0,  // reference_delta size
      0,    0,    0,    0,    0, 0, 0, 0,  // pool count
  };

  TestInvalidInitialize<EnsemblePatchReader>(&data);
}

}  // namespace zucchini
