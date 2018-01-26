// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/tools/indexing_tool.h"

#include <utility>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "components/subresource_filter/core/common/indexed_ruleset.h"
#include "components/url_pattern_index/copying_file_stream.h"
#include "components/url_pattern_index/proto/rules.pb.h"
#include "components/url_pattern_index/unindexed_ruleset.h"

namespace subresource_filter {

void ConvertToIndexedRuleset(const base::FilePath& unindexed_path,
                             const base::FilePath& indexed_path) {
  base::File unindexed_file(unindexed_path,
                            base::File::FLAG_OPEN | base::File::FLAG_READ);

  subresource_filter::RulesetIndexer indexer;

  url_pattern_index::CopyingFileInputStream copying_stream(
      std::move(unindexed_file));
  google::protobuf::io::CopyingInputStreamAdaptor zero_copy_stream_adaptor(
      &copying_stream, 4096 /* buffer_size */);
  url_pattern_index::UnindexedRulesetReader reader(&zero_copy_stream_adaptor);

  size_t num_unsupported_rules = 0;
  url_pattern_index::proto::FilteringRules ruleset_chunk;
  while (reader.ReadNextChunk(&ruleset_chunk)) {
    for (const auto& rule : ruleset_chunk.url_rules()) {
      if (!indexer.AddUrlRule(rule))
        ++num_unsupported_rules;
    }
  }
  indexer.Finish();

  base::WriteFile(indexed_path, reinterpret_cast<const char*>(indexer.data()),
                  base::checked_cast<int>(indexer.size()));
}

}  // namespace subresource_filter
