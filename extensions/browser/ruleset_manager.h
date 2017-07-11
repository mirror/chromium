// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_RULESET_MANAGER_H_
#define EXTENSIONS_BROWSER_RULESET_MANAGER_H_

#include <map>
#include <memory>

#include <base/macros.h>
#include "base/files/memory_mapped_file.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "extensions/browser/ruleset_matcher.h"
#include "extensions/common/extension_id.h"

class GURL;

namespace base {
class Time;
}

namespace net {
class URLRequest;
}

namespace extensions {
class InfoMap;

namespace declarative_net_request {

// Manages the set of active rulesets for the Declarative Net Request API. Can
// be constructed on any sequence but must be accessed from the same sequence.
class RulesetManager {
 public:
  explicit RulesetManager(InfoMap* info_map);
  ~RulesetManager();

  // Adds ruleset for the given |extension_id| and the given |install_time|.
  // Must not be called in succession without an intermediate call to
  // RemoveRuleset.
  void AddRuleset(const ExtensionId& extension_id,
                  const base::Time& install_time,
                  std::unique_ptr<RulesetMatcher> ruleset_matcher);

  // Removes the ruleset for |extension_id|.
  void RemoveRuleset(const ExtensionId& extension_id);

  bool ShouldBlockRequest(bool is_incognito_context,
                          net::URLRequest* request) const;

  bool ShouldRedirectRequest(bool is_incognito_context,
                             net::URLRequest* request,
                             GURL* redirect_url) const;

 private:
  struct RulesMapKey {
    RulesMapKey(ExtensionId, base::Time);
    ~RulesMapKey();
    RulesMapKey(RulesMapKey&&);
    ExtensionId id;
    base::Time install_time;
    DISALLOW_COPY_AND_ASSIGN(RulesMapKey);
  };

  struct RulesMapKeyComparator {
    bool operator()(const RulesMapKey& lhs, const RulesMapKey& rhs) const;
  };

  using RulesMap = std::
      map<RulesMapKey, std::unique_ptr<RulesetMatcher>, RulesMapKeyComparator>;

  // A map from the (ExtensionId, extension install time) to the extension
  // ruleset. The extension install time is a part of the key to ensure we can
  // iterate over the map by decreasing installation time.
  RulesMap rules_;

  // A map from ExtensionId to its install time as seen in AddRuleset.
  // We maintain an internal map from ExtensionId -> install time instead of
  // retrieving it from an external dependency during RemoveRuleset, since the
  // install time is used as a key in |rules_| and we want it to be consistent
  // throughout.
  // COMMENT: Can the install time ever change? Extension updates? Alternative:
  // info_map_ but then we'd have to rely on ordering between IO thread calls to
  // info map and IO thread calls from rules monitor. Also it may change.
  // Alternative 2: Dont keep sorted by install time.
  // COMMENT: What if we iterate from last enabled. The most recently enabled
  // extension has the highest priority. Seems more reasonable.
  std::map<ExtensionId, base::Time> install_time_map_;

  // Weak pointer. Owns us.
  InfoMap* info_map_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(RulesetManager);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_RULESET_MANAGER_H_
