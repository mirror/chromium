// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_whitelist_manager.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/url_formatter/url_fixer.h"

using url_matcher::URLMatcher;
using url_matcher::URLMatcherCondition;
using url_matcher::URLMatcherConditionFactory;
using url_matcher::URLMatcherConditionSet;

namespace android_webview {

// Contains a set of filters to whitelist certain URLs.
class URLWhitelist {
 public:
  URLWhitelist();
  ~URLWhitelist() {}

  void AddFilters(const std::vector<std::string>& hosts);
  bool IsURLWhitelisted(const GURL& url) const;

 private:
  url_matcher::URLMatcherConditionSet::ID id_;
  std::unique_ptr<url_matcher::URLMatcher> url_matcher_;

  DISALLOW_COPY_AND_ASSIGN(URLWhitelist);
};

namespace {

// A task that builds the whitelist on a background thread.
std::unique_ptr<URLWhitelist> BuildWhitelist(
    const std::vector<std::string>& allow) {
  std::unique_ptr<URLWhitelist> whitelist(new URLWhitelist);
  whitelist->AddFilters(allow);
  return whitelist;
}

}  // namespace

URLWhitelist::URLWhitelist() : id_(0), url_matcher_(new URLMatcher) {}

void URLWhitelist::AddFilters(const std::vector<std::string>& hosts) {
  URLMatcherConditionSet::Vector all_conditions;

  URLMatcherConditionFactory* condition_factory =
      url_matcher_->condition_factory();
  for (const auto& host : hosts) {
    url::Parsed parsed;
    std::string ignored_scheme = url_formatter::SegmentURL(host, &parsed);
    if (!parsed.host.is_nonempty()) {
      // host cannot be empty
      LOG(ERROR) << "Invalid pattern " << host;
      continue;
    }
    std::string lc_host = base::ToLowerASCII(
        base::StringPiece(host).substr(parsed.host.begin, parsed.host.len));
    if (lc_host == "*") {
      // Wildcard hosts are not allowed and ignored.
      continue;
    }

    bool match_subdomains = true;
    if (lc_host.at(0) == '.') {
      // A leading dot in the pattern syntax means that we don't want to match
      // subdomains.
      lc_host.erase(0, 1);
      match_subdomains = false;
    } else {
      url::RawCanonOutputT<char> output;
      url::CanonHostInfo host_info;
      url::CanonicalizeHostVerbose(host.c_str(), parsed.host, &output,
                                   &host_info);
      if (host_info.family == url::CanonHostInfo::NEUTRAL) {
        // We want to match subdomains. Add a dot in front to make sure we only
        // match at domain component boundaries.
        lc_host.insert(lc_host.begin(), '.');
        match_subdomains = true;
      } else {
        match_subdomains = false;
      }
    }
    std::set<URLMatcherCondition> conditions;
    conditions.insert(
        match_subdomains
            ? condition_factory->CreateHostSuffixCondition(lc_host)
            : condition_factory->CreateHostEqualsCondition(lc_host));
    all_conditions.push_back(new URLMatcherConditionSet(++id_, conditions));
  }
  url_matcher_->AddConditionSets(all_conditions);
}

bool URLWhitelist::IsURLWhitelisted(const GURL& url) const {
  return !url_matcher_->MatchURL(url).empty();
}

AwSafeBrowsingWhitelistManager::AwSafeBrowsingWhitelistManager(
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner)
    : background_task_runner_(background_task_runner),
      io_task_runner_(io_task_runner),
      ui_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      whitelist_(new URLWhitelist) {}

AwSafeBrowsingWhitelistManager::~AwSafeBrowsingWhitelistManager() {}

void AwSafeBrowsingWhitelistManager::SetWhitelist(
    std::unique_ptr<URLWhitelist> whitelist) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  whitelist_ = std::move(whitelist);
}

void AwSafeBrowsingWhitelistManager::UpdateOnIO(
    std::vector<std::string>&& hosts) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  // use base::Unretained as AwSafeBrowsingWhitelistManager is a singleton and
  // not cleaned.
  base::PostTaskAndReplyWithResult(
      background_task_runner_.get(), FROM_HERE,
      base::Bind(&BuildWhitelist, std::move(hosts)),
      base::Bind(&AwSafeBrowsingWhitelistManager::SetWhitelist,
                 base::Unretained(this)));
}

void AwSafeBrowsingWhitelistManager::SetWhitelistOnUiThread(
    std::vector<std::string>&& hosts) {
  DCHECK(ui_task_runner_->RunsTasksInCurrentSequence());
  // use base::Unretained as AwSafeBrowsingWhitelistManager is a singleton and
  // not cleaned.
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&AwSafeBrowsingWhitelistManager::UpdateOnIO,
                 base::Unretained(this), base::Passed(std::move(hosts))));
}

bool AwSafeBrowsingWhitelistManager::IsURLWhitelisted(const GURL& url) const {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());
  return whitelist_->IsURLWhitelisted(url);
}

}  // namespace android_webview
