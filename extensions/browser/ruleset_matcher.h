// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_RULESET_MATCHER_H_
#define EXTENSIONS_BROWSER_RULESET_MATCHER_H_

#include <memory>
#include "components/url_pattern_index/flat/url_pattern_index_generated.h"
#include "components/url_pattern_index/url_pattern_index.h"
#include "content/public/browser/browser_thread.h"

namespace base {
class MemoryMappedFile;
class FilePath;
}  // namespace base

class GURL;

namespace url {
class Origin;
}

namespace extensions {

namespace declarative_net_request {

namespace flat {
struct ExtensionIndexedRuleset;
struct ExtensionRuleMetadata;
}  // namespace flat

// This is created on the file thread and used on the IO thread. However,
// there are no thread safety issues since this is only accessed on a single
// thread at a time.
class RulesetMatcher {
 public:
  bool ShouldBlockRequest(const GURL& url,
                          const url::Origin& first_party_origin,
                          ::url_pattern_index::flat::ElementType element_type,
                          bool is_third_party) const;

  bool ShouldRedirectRequest(
      const GURL& url,
      const url::Origin& first_party_origin,
      ::url_pattern_index::flat::ElementType element_type,
      bool is_third_party,
      GURL* redirect_url) const;

  // Factory function to create a verified RulesetMatcher. Must
  // be called on a sequence where file IO is allowed.
  static std::unique_ptr<RulesetMatcher> CreateVerifiedMatcher(
      const base::FilePath& indexed_ruleset_path);

 private:
  using UrlPatternIndexMatcher = url_pattern_index::UrlPatternIndexMatcher;
  using ExtensionRuleMetadataList =
      flatbuffers::Vector<flatbuffers::Offset<flat::ExtensionRuleMetadata>>;

  explicit RulesetMatcher(
      std::unique_ptr<base::MemoryMappedFile,
                      content::BrowserThread::DeleteOnFileThread> ruleset_file);

  // This owns the actual buffer. Are there any potential issues of using
  // DeleteOnFileThread.
  std::unique_ptr<base::MemoryMappedFile,
                  content::BrowserThread::DeleteOnFileThread>
      ruleset_file_;
  const flat::ExtensionIndexedRuleset* root_;
  const UrlPatternIndexMatcher blacklist_matcher_;
  const UrlPatternIndexMatcher whitelist_matcher_;
  const UrlPatternIndexMatcher redirect_matcher_;
  const ExtensionRuleMetadataList* extension_metdata_;

  DISALLOW_COPY_AND_ASSIGN(RulesetMatcher);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_RULESET_MATCHER_H_
