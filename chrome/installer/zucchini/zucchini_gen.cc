// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_gen.h"

#include <utility>

#include "chrome/installer/zucchini/encoded_view.h"
#include "chrome/installer/zucchini/suffix_array.h"

namespace zucchini {

// Parameters for patch generation.
constexpr int kMinEquivalenceSimilarity = 12;

bool GenerateEquivalencesAndExtraData(ConstBufferView new_image,
                                      const EquivalenceMap& equivalence_map,
                                      PatchElementWriter* patch_writer) {
  // Store data in gaps in |new_image| before / between / after
  // |local_equivalence_map| blocks as "extra data".
  ExtraDataSink extra_data_sink;
  EquivalenceSink equivalences_sink;

  ConstBufferView::iterator dst_it = new_image.begin();
  for (EquivalenceCandidate equivalence : equivalence_map) {
    equivalences_sink.PutNext(Equivalence(equivalence));
    ConstBufferView::iterator next_dst =
        new_image.begin() + equivalence.dst_offset;
    extra_data_sink.PutNext(ConstBufferView::FromRange(dst_it, next_dst));
    dst_it = next_dst + equivalence.length;
    if (dst_it > new_image.end())
      return false;
  }
  extra_data_sink.PutNext(ConstBufferView::FromRange(dst_it, new_image.end()));

  patch_writer->SetEquivalenceSink(std::move(equivalences_sink));
  patch_writer->SetExtraDataSink(std::move(extra_data_sink));
  return true;
}

bool GenerateRawDelta(ConstBufferView old_image,
                      ConstBufferView new_image,
                      const EquivalenceMap& equivalence_map,
                      const ImageIndex& new_index,
                      PatchElementWriter* patch_writer) {
  RawDeltaSink raw_delta_sink;

  // Visit |local_equivalence_map| blocks in |local_dst| order, in lockstep
  // with |new_references|. Find and emit all bytewise differences.
  offset_t base_offset = 0;
  for (EquivalenceCandidate equivalence : equivalence_map) {
    // For each bytewise delta from |src| to |local_dst|, compute "copy offset"
    // and pass it along with delta to the receptor.
    for (offset_t i = 0; i < equivalence.length; ++i) {
      if (new_index.IsReference(equivalence.dst_offset + i))
        continue;

      if (old_image[equivalence.src_offset + i] !=
          new_image[equivalence.dst_offset + i]) {
        int8_t diff = new_image[equivalence.dst_offset + i] -
                      old_image[equivalence.src_offset + i];
        raw_delta_sink.PutNext({base_offset + i, diff});
      }
    }
    base_offset += equivalence.length;
  }
  patch_writer->SetRawDeltaSink(std::move(raw_delta_sink));

  return true;
}

bool GenerateRawElement(std::vector<offset_t> old_sa,
                        ConstBufferView old_image,
                        ConstBufferView new_image,
                        PatchElementWriter* patch_writer) {
  ImageIndex old_image_index(old_image, {});
  ImageIndex new_image_index(new_image, {});

  EquivalenceMap equivalences;
  equivalences.Build(old_sa, old_image_index, new_image_index,
                     kMinEquivalenceSimilarity);

  patch_writer->SetReferenceDeltaSink({});
  return GenerateEquivalencesAndExtraData(new_image, equivalences,
                                          patch_writer) &&
         GenerateRawDelta(old_image, new_image, equivalences, new_image_index,
                          patch_writer);
}

status::Code GenerateEnsemble(ConstBufferView old_image,
                              ConstBufferView new_image,
                              EnsemblePatchWriter* patch_writer) {
  patch_writer->SetPatchType(PatchType::kEnsemblePatch);

  // Dummy patch element to fill patch_writer.
  PatchElementWriter patch_element(
      {Element(old_image.region()), Element(new_image.region())});
  patch_element.SetEquivalenceSink({});
  patch_element.SetExtraDataSink({});
  patch_element.SetRawDeltaSink({});
  patch_element.SetReferenceDeltaSink({});
  patch_writer->AddElement(std::move(patch_element));

  // TODO(etiennep): Implement.
  return status::kStatusSuccess;
}

status::Code GenerateRaw(ConstBufferView old_image,
                         ConstBufferView new_image,
                         EnsemblePatchWriter* patch_writer) {
  patch_writer->SetPatchType(PatchType::kRawPatch);

  ImageIndex old_image_index(old_image, {});
  EncodedView old_view(&old_image_index);
  std::vector<offset_t> old_sa =
      MakeSuffixArray<InducedSuffixSort>(old_view, old_view.Cardinality());

  PatchElementWriter patch_element(
      {Element(old_image.region()), Element(new_image.region())});
  if (!GenerateRawElement(old_sa, old_image, new_image, &patch_element))
    return status::kStatusFatal;
  patch_writer->AddElement(std::move(patch_element));
  return status::kStatusSuccess;
}

}  // namespace zucchini
