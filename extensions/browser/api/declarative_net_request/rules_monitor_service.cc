// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/rules_monitor_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/ruleset_manager.h"
#include "extensions/browser/ruleset_matcher.h"
#include "extensions/common/api/declarative_net_request/rules_manifest_info.h"
#include "extensions/common/file_util.h"

namespace extensions {
namespace declarative_net_request {

namespace {

bool HasIndexedRuleset(const Extension* extension) {
  // TODO(karandeepb): This needs to change once dynamic rules are supported,
  // since an extension may only use dynamic rules without having a manifest
  // ruleset. This can use the ruleset file checksum once it is supported.
  return declarative_net_request::RulesManifestData::GetJSONRulesetPath(
             extension) != nullptr;
}

// COMMENT: Most existing BrowserContextKeyedAPIs use DestructionAtExit.
// LazyInstance recommends using Leaky (for faster shutdown). Any reason we
// should use DestructionAtExit? The e.g. in BrowserContextKeyedAPI also uses
// DestructionAtExit.
static base::LazyInstance<BrowserContextKeyedAPIFactory<RulesMonitorService>>::
    DestructorAtExit g_factory = LAZY_INSTANCE_INITIALIZER;

void LoadRulesetOnIOThread(std::string extension_id,
                           base::Time install_time,
                           std::unique_ptr<RulesetMatcher> ruleset_matcher,
                           InfoMap* info_map) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  info_map->GetRulesetManager()->AddRuleset(extension_id, install_time,
                                            std::move(ruleset_matcher));
}

void UnloadRulesetOnIOThread(const std::string& extension_id,
                             InfoMap* info_map) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  info_map->GetRulesetManager()->RemoveRuleset(extension_id);
}

// Constructs the RulesetMatcher and forwards the same to the IO thread.
void LoadRulesetOnFileTaskRunner(std::string extension_id,
                                 base::Time install_time,
                                 base::FilePath indexed_file_path,
                                 scoped_refptr<InfoMap> info_map) {
  base::ThreadRestrictions::AssertIOAllowed();

  std::unique_ptr<RulesetMatcher> ruleset_matcher;
  RulesetMatcher::LoadRulesetResult result =
      RulesetMatcher::CreateVerifiedMatcher(indexed_file_path,
                                            &ruleset_matcher);
  UMA_HISTOGRAM_ENUMERATION(
      "Extensions.DeclarativeNetRequest.LoadRulesetResult", result,
      RulesetMatcher::LOAD_RESULT_MAX);
  if (result != RulesetMatcher::LOAD_SUCCESS)
    return;

  base::OnceClosure task = base::BindOnce(
      &LoadRulesetOnIOThread, std::move(extension_id), std::move(install_time),
      std::move(ruleset_matcher), base::RetainedRef(std::move(info_map)));
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   std::move(task));
}

// Forwards the ruleset unloading to the IO thread.
void UnloadRulesetOnFileTaskRunner(std::string extension_id,
                                   scoped_refptr<InfoMap> info_map) {
  base::ThreadRestrictions::AssertIOAllowed();

  base::OnceClosure task =
      base::BindOnce(&UnloadRulesetOnIOThread, std::move(extension_id),
                     base::RetainedRef(std::move(info_map)));
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   std::move(task));
}

}  // namespace

// static
BrowserContextKeyedAPIFactory<RulesMonitorService>*
RulesMonitorService::GetFactoryInstance() {
  return g_factory.Pointer();
}

RulesMonitorService::RulesMonitorService(
    content::BrowserContext* browser_context)
    : registry_observer_(this),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  registry_observer_.Add(ExtensionRegistry::Get(browser_context));
}

RulesMonitorService::~RulesMonitorService() = default;

void RulesMonitorService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (!HasIndexedRuleset(extension))
    return;

  base::OnceClosure task = base::BindOnce(
      &LoadRulesetOnFileTaskRunner, extension->id(),
      ExtensionPrefs::Get(browser_context)->GetInstallTime(extension->id()),
      file_util::GetIndexedRulesetPath(extension->path()),
      make_scoped_refptr(ExtensionSystem::Get(browser_context)->info_map()));
  file_task_runner_->PostTask(FROM_HERE, std::move(task));
}

void RulesMonitorService::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  if (!HasIndexedRuleset(extension))
    return;

  // Post the task first to the |file_task_runner_| and then to the IO thread,
  // even though we don't need to do any file IO. Posting the task directly to
  // the IO thread here can lead to RulesetManager::RemoveRuleset being called
  // before a corresponding RulesetManager::AddRuleset.
  // COMMENT: Is there a easier way to accomplish this?
  base::OnceClosure task = base::BindOnce(
      &UnloadRulesetOnFileTaskRunner, extension->id(),
      make_scoped_refptr(ExtensionSystem::Get(browser_context)->info_map()));
  file_task_runner_->PostTask(FROM_HERE, std::move(task));
}

}  // namespace declarative_net_request
}  // namespace extensions
