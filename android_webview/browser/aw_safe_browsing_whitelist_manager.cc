// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_whitelist_manager.h"

#include <map>

#include "base/bind.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/url_formatter/url_fixer.h"
#include "net/base/url_util.h"
#include "url/url_util.h"

#include "base/strings/string_split.h"

namespace android_webview {

// A manager's poor trie
struct TrieNode {
  std::map<std::string, std::unique_ptr<TrieNode>> children;
  bool match_subdomains = false;
  bool is_leaf = false;
};

namespace {

void AddHostToFilters(std::string host, TrieNode* root) {
  url::Parsed parsed;
  std::string scheme = url_formatter::SegmentURL(host, &parsed);
  if (!url::IsStandard(scheme.c_str(), url::Component(0, scheme.length()))) {
    LOG(ERROR) << "Non standard scheme" << scheme;
    return;
  }
  if (!parsed.host.is_nonempty()) {
    LOG(ERROR) << "Invalid pattern " << host;
    return;
  }
  std::string lc_host = base::ToLowerASCII(
      base::StringPiece(host).substr(parsed.host.begin, parsed.host.len));
  if (lc_host == "*") {
    LOG(ERROR) << "Wildcards not alloweded" << host;
    return;
  }

  bool match_subdomains = false;
  std::vector<base::StringPiece> components;
  url::CanonHostInfo host_info;
  std::string canonicalized_host = net::CanonicalizeHost(lc_host, &host_info);
  if (host_info.family == url::CanonHostInfo::BROKEN) {
    LOG(ERROR) << "Host address could not be canonicalized " << lc_host;
    return;
  }
  if (host_info.family == url::CanonHostInfo::NEUTRAL) {
    if (lc_host.at(0) == '.') {
      // A leading dot in the pattern syntax means that we don't want to match
      // subdomains.
      lc_host.erase(0, 1);
    } else {
      match_subdomains = true;
    }
    components = base::SplitStringPiece(lc_host, ".", base::TRIM_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
  } else {
    components.push_back(canonicalized_host);
  }

  TrieNode* node = root;
  for (auto hostcomp = components.rbegin(); hostcomp != components.rend();
       ++hostcomp) {
    std::string component = hostcomp->as_string();
    // If the component is empty, break the loop, i.e. in ".google.com", the
    // last component will be empty.
    if (component.empty())
      break;

    auto child_node = node->children.find(component);
    if (child_node == node->children.end()) {
      std::unique_ptr<TrieNode> temp = base::MakeUnique<TrieNode>();
      TrieNode* current = temp.get();
      node->children[component] = std::move(temp);
      node = current;
    } else {
      node = child_node->second.get();
    }
  }
  if (node != root) {
    node->match_subdomains = match_subdomains;
    node->is_leaf = true;
  }
}

void AddFilters(const std::vector<std::string>& allow, TrieNode* root) {
  for (auto host : allow) {
    AddHostToFilters(host, root);
  }
}

// A task that builds the whitelist on a background thread.
std::unique_ptr<TrieNode> BuildWhitelist(
    const std::vector<std::string>& allow) {
  std::unique_ptr<TrieNode> whitelist(new TrieNode);
  AddFilters(allow, whitelist.get());
  return whitelist;
}

bool IsWhitelisted(const GURL& url, TrieNode* node) {
  std::string host = url.host();
  url::CanonHostInfo host_info;
  std::string ignored = net::CanonicalizeHost(host.c_str(), &host_info);

  std::vector<base::StringPiece> components;
  if (host_info.family == url::CanonHostInfo::NEUTRAL) {
    components = base::SplitStringPiece(host, ".", base::TRIM_WHITESPACE,
                                        base::SPLIT_WANT_ALL);
  } else {
    components.push_back(host);
  }
  for (auto component = components.rbegin(); component != components.rend();
       ++component) {
    // If the component is empty, break the loop, i.e. in ".google.com", the
    // last component will be empty.
    if (component->empty())
      break;

    auto child_node = node->children.find(component->as_string());
    if (child_node == node->children.end()) {
      return node->match_subdomains;
    } else {
      node = child_node->second.get();
    }
  }
  // If trie search finished in a leaf node, host is found in trie. The
  // root node is not a leaf node.
  return node->is_leaf;
}

}  // namespace

AwSafeBrowsingWhitelistManager::AwSafeBrowsingWhitelistManager(
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner)
    : background_task_runner_(background_task_runner),
      io_task_runner_(io_task_runner),
      ui_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      whitelist_(new TrieNode) {}

AwSafeBrowsingWhitelistManager::~AwSafeBrowsingWhitelistManager() {}

void AwSafeBrowsingWhitelistManager::SetWhitelist(
    std::unique_ptr<TrieNode> whitelist) {
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

void AwSafeBrowsingWhitelistManager::SetWhitelistOnUIThread(
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
  return IsWhitelisted(url, whitelist_.get());
}

}  // namespace android_webview
