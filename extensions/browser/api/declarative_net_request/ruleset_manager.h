// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MANAGER_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MANAGER_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"

class GURL;

namespace net {
class URLRequest;
}  // namespace net

namespace extensions {
class InfoMap;

namespace declarative_net_request {
class RulesetMatcher;

// Manages the set of active rulesets for the Declarative Net Request API. Can
// be constructed on any sequence but must be accessed from the same sequence.
class RulesetManager {
 public:
  explicit RulesetManager(const InfoMap* info_map);
  ~RulesetManager();

  // Adds the ruleset for the given |extension_id| with the given
  // |install_time|.
  void AddRuleset(const std::string& extension_id,
                  const base::Time& install_time,
                  std::unique_ptr<RulesetMatcher> ruleset_matcher);

  // Removes the ruleset for |extension_id|.
  void RemoveRuleset(const std::string& extension_id);

  // Returns whether the given |request| should be blocked.
  bool ShouldBlockRequest(bool is_incognito_context,
                          const net::URLRequest& request) const;

  // Returns whether the given |request| should be redirected along with the
  // |redirect_url|.
  bool ShouldRedirectRequest(bool is_incognito_context,
                             const net::URLRequest& request,
                             GURL* redirect_url) const;

  // Returns the number of RulesetMatcher currently being managed.
  size_t GetMatcherCountForTest() const;

 private:
  struct RulesMapKey {
    std::string id;
    base::Time install_time;
  };

  struct RulesMapKeyComparator {
    bool operator()(const RulesMapKey& lhs, const RulesMapKey& rhs) const;
  };

  using RulesMap = std::
      map<RulesMapKey, std::unique_ptr<RulesetMatcher>, RulesMapKeyComparator>;

  // A map from the (extension id, install time) to the extension ruleset. The
  // extension install time is a part of the key to ensure we can iterate over
  // the map by decreasing installation time.
  RulesMap rules_;

  // A map from extension id to its install time as seen in AddRuleset.
  // We maintain an internal map from extension id -> install time instead of
  // retrieving it from an external dependency during RemoveRuleset, since the
  // install time is used as a key in |rules_| and we want it to be consistent
  // throughout.
  std::map<std::string, base::Time> install_time_map_;

  // COMMENT: Doesn't iterating from the last enabled extension make more sense
  // instead of relying on install time? In that case,

  // COMMENT: Alternative 1: Use info_map to always retrieve the installation
  // time. But we'd be depending on the fact that InfoMap::AddExtension is
  // scheduled before OnExtensionLoaded is called and InfoMap::RemoveExtension
  // is scheduled after OnExtensionUnloaded is called. (This is true, but think
  // it's better to not depend on it.).

  // COMMENT: Alternatively, do not keep sorted by installation time. nlogn
  // iteration, O(1) insertion, deletion.

  // COMMENT: Alternatively, use insertion sort. O(n) iteration, insertion,
  // deletion.

  // Weak pointer. Owns us.
  const InfoMap* info_map_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(RulesetManager);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MANAGER_H_
