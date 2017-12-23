// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MANAGER_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MANAGER_H_

#include <stddef.h>
#include <memory>

#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "extensions/common/extension_id.h"
#include "url/gurl.h"
// #include "components/url_pattern_index/url_pattern_index.h"

class GURL;

namespace net {
class URLRequest;
}  // namespace net

namespace url {
class Origin;
}  // namespace url

namespace extensions {
class InfoMap;

namespace declarative_net_request {
class RulesetMatcher;

// Manages the set of active rulesets for the Declarative Net Request API. Can
// be constructed on any sequence but must be accessed and destroyed from the
// same sequence.
class RulesetManager {
 public:
  explicit RulesetManager(const InfoMap* info_map);
  ~RulesetManager();

  // An interface used for testing purposes.
  class TestObserver {
   public:
    virtual void OnEvaluateRuleset(const net::URLRequest& request,
                                   bool is_incognito_context) = 0;

   protected:
    virtual ~TestObserver() {}
  };

  struct Result {
    Result();
    ~Result();
    Result(const Result&);

    // Do nothing if |cancel| is false and |redirect_url| is empty.
    bool cancel = false;
    base::Optional<GURL> redirect_url = base::nullopt;
    std::string extension_id;
    base::Time extension_install_time;
  };

  struct ExtensionRulesetData {
    ExtensionRulesetData(ExtensionId,
                         base::Time,
                         bool is_incognito_enabled,
                         std::unique_ptr<RulesetMatcher>);
    ~ExtensionRulesetData();
    ExtensionRulesetData(ExtensionRulesetData&& other);
    ExtensionRulesetData& operator=(ExtensionRulesetData&& other);

    ExtensionId extension_id;
    base::Time extension_install_time;
    bool is_incognito_enabled;
    std::unique_ptr<RulesetMatcher> matcher;

    bool operator<(const ExtensionRulesetData& other) const;

    DISALLOW_COPY_AND_ASSIGN(ExtensionRulesetData);
  };

  using EvaluateRulesetCallback = base::OnceCallback<void(const Result&)>;

  // If nullopt, callback will be used (async).
  base::Optional<Result> EvaluateRuleset(
      const net::URLRequest& request,
      bool is_incognito_context,
      EvaluateRulesetCallback callback) const;

  // Adds the ruleset for the given |extension_id|. Call again if extension's
  // incognito status or in install time changes.
  void AddRuleset(const ExtensionId& extension_id,
                  std::unique_ptr<RulesetMatcher> ruleset_matcher);

  // Removes the ruleset for |extension_id|. Should be called only after a
  // corresponding AddRuleset.
  void RemoveRuleset(const ExtensionId& extension_id);

  // Returns the number of RulesetMatcher currently being managed.
  size_t GetMatcherCountForTest() const;

  void SetAsync();

  // Sets the TestObserver. Client maintains ownership of |observer|.
  void SetObserverForTest(TestObserver* observer);

  void PrintRulesetEvalTime();

 private:
  class Core;

  Core* GetCore() const;

  // Non-owning pointer to InfoMap. Owns us.
  const InfoMap* const info_map_;

  bool async_ = false;

  // Non-owning pointer to TestObserver.
  TestObserver* test_observer_ = nullptr;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  std::unique_ptr<Core> core_;

  std::unique_ptr<Core, base::OnTaskRunnerDeleter> core_async_;

  bool has_added_ruleset_ = false;

  mutable base::TimeDelta third_party_time_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(RulesetManager);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_RULESET_MANAGER_H_
