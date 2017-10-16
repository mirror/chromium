// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MATCHER_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MATCHER_H_

#include <memory>

#include "base/files/memory_mapped_file.h"
#include "base/sequenced_task_runner.h"
#include "components/url_pattern_index/url_pattern_index.h"

namespace base {
class FilePath;
}  // namespace base

class GURL;

namespace url {
class Origin;
}  // namespace url

namespace extensions {
namespace declarative_net_request {

namespace flat {
struct ExtensionIndexedRuleset;
struct UrlRuleMetadata;
}  // namespace flat

// RulesetMatcher encapsulates the Declarative Net Request API ruleset
// corresponding to a single extension. The ruleset file is memory mapped. This
// uses the url_pattern_index component to achieve fast matching of network
// requests against declarative rules. Since this class is immutable, it is
// thread-safe. In practice it is accessed on the IO thread but created on a
// sequence where file IO is allowed.
class RulesetMatcher {
 public:
  // Describes the result of creating a RulesetMatcher instance.
  enum LoadRulesetResult {
    LOAD_SUCCESS = 0,
    LOAD_ERROR_INVALID_PATH = 1,
    LOAD_ERROR_MEMORY_MAP = 2,
    LOAD_ERROR_RULESET_VERIFICATION = 3,
    LOAD_RESULT_MAX
  };

  // Factory function to create a verified RulesetMatcher.
  // |indexed_ruleset_path| is the path of the extension ruleset in flatbuffer
  // format. Must be called on a sequence where file IO is allowed. Returns
  // LOAD_SUCCESS on success along with the ruleset |matcher|.
  static LoadRulesetResult CreateVerifiedMatcher(
      const base::FilePath& indexed_ruleset_path,
      int expected_ruleset_checksum,
      std::unique_ptr<RulesetMatcher>* matcher);

  // Returns whether the network request as specified by the passed parameters
  // should be blocked.
  bool ShouldBlockRequest(const GURL& url,
                          const url::Origin& first_party_origin,
                          url_pattern_index::flat::ElementType element_type,
                          bool is_third_party) const;

  // Returns true along with the |redirect_url| if the network request as
  // specified by the passed parameters should be redirected.
  bool ShouldRedirectRequest(const GURL& url,
                             const url::Origin& first_party_origin,
                             url_pattern_index::flat::ElementType element_type,
                             bool is_third_party,
                             GURL* redirect_url) const;

 private:
  using UrlPatternIndexMatcher = url_pattern_index::UrlPatternIndexMatcher;
  using MemoryMappedRuleset =
      std::unique_ptr<base::MemoryMappedFile, base::OnTaskRunnerDeleter>;

  explicit RulesetMatcher(MemoryMappedRuleset ruleset_file);

  // The memory mapped ruleset file. Deleted on a sequence which supports
  // blocking calls.
  const MemoryMappedRuleset ruleset_;

  const flat::ExtensionIndexedRuleset* root_;
  const UrlPatternIndexMatcher blacklist_matcher_;
  const UrlPatternIndexMatcher whitelist_matcher_;
  const UrlPatternIndexMatcher redirect_matcher_;
  const flatbuffers::Vector<flatbuffers::Offset<flat::UrlRuleMetadata>>*
      extension_metadata_;

  DISALLOW_COPY_AND_ASSIGN(RulesetMatcher);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MATCHER_H_
