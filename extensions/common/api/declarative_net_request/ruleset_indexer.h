// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_RULESET_INDEXER_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_RULESET_INDEXER_H_

#include <utility>
#include <vector>

#include "base/macros.h"
#include "components/url_pattern_index/url_pattern_index.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/flat/extension_ruleset_generated.h"
#include "third_party/flatbuffers/src/include/flatbuffers/flatbuffers.h"

namespace extensions {
namespace declarative_net_request {

struct IndexedRule;

// Helper class to index rules in the flatbuffer format for the Declarative Net
// Request API.
class RulesetIndexer {
 public:
  // The data buffer storing the ruleset.
  using SerializedData = std::pair<uint8_t*, size_t>;

  RulesetIndexer();
  ~RulesetIndexer();

  // Adds |indexed_rule| to the ruleset.
  void AddUrlRule(const IndexedRule& indexed_rule);

  // Finishes the ruleset construction and returns the data buffer, which is
  // still owned by RulesetIndexer.
  SerializedData FinishAndGetData();

 private:
  using UrlPatternIndexBuilder = ::url_pattern_index::UrlPatternIndexBuilder;

  UrlPatternIndexBuilder* GetBuilder(
      api::declarative_net_request::RuleActionType type);

  flatbuffers::FlatBufferBuilder builder_;

  UrlPatternIndexBuilder blacklist_index_builder_;
  UrlPatternIndexBuilder whitelist_index_builder_;
  UrlPatternIndexBuilder redirect_index_builder_;
  std::vector<flatbuffers::Offset<flat::ExtensionRuleMetadata>> metadata_;

  size_t cnt_;  // No of rules indexed till now.

  bool finished_;  // Whether FinishAndGetData() has been called.

  DISALLOW_COPY_AND_ASSIGN(RulesetIndexer);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_RULESET_INDEXER_H_
