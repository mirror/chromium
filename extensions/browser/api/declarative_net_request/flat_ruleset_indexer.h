// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_FLAT_RULESET_INDEXER_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_FLAT_RULESET_INDEXER_H_

#include <utility>
#include <vector>

#include "base/macros.h"
#include "components/url_pattern_index/url_pattern_index.h"
#include "extensions/browser/api/declarative_net_request/flat/extension_ruleset_generated.h"
#include "extensions/common/api/declarative_net_request.h"
#include "third_party/flatbuffers/src/include/flatbuffers/flatbuffers.h"

namespace extensions {
namespace declarative_net_request {

struct IndexedRule;

// Helper class to index rules in the flatbuffer format for the Declarative Net
// Request API.
class FlatRulesetIndexer {
 public:
  // The data buffer storing the ruleset.
  using SerializedData = std::pair<uint8_t*, size_t>;

  FlatRulesetIndexer();
  ~FlatRulesetIndexer();

  // Adds |indexed_rule| to the ruleset.
  void AddUrlRule(const IndexedRule& indexed_rule);

  // Returns the number of rules added till now.
  size_t rules_count() const { return rules_count_; }

  // Finishes the ruleset construction and returns the data buffer, which is
  // still owned by FlatRulesetIndexer.
  SerializedData FinishAndGetData();

 private:
  using UrlPatternIndexBuilder = url_pattern_index::UrlPatternIndexBuilder;

  UrlPatternIndexBuilder* GetBuilder(
      api::declarative_net_request::RuleActionType type);

  flatbuffers::FlatBufferBuilder builder_;

  UrlPatternIndexBuilder blacklist_index_builder_;
  UrlPatternIndexBuilder whitelist_index_builder_;
  UrlPatternIndexBuilder redirect_index_builder_;
  std::vector<flatbuffers::Offset<flat::UrlRuleMetadata>> metadata_;

  size_t rules_count_ = 0;  // No of rules indexed till now.
  bool finished_ = false;   // Whether FinishAndGetData() has been called.

  DISALLOW_COPY_AND_ASSIGN(FlatRulesetIndexer);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_FLAT_RULESET_INDEXER_H_
