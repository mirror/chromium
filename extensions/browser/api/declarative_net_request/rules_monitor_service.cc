// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/rules_monitor_service.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/memory_mapped_file.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/ruleset_manager.h"
#include "extensions/browser/ruleset_matcher.h"
#include "extensions/common/file_util.h"

namespace extensions {
namespace declarative_net_request {

namespace {

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

void LoadRulesetOnFileTaskRunner(std::string extension_id,
                                 base::Time install_time,
                                 base::FilePath indexed_file_path,
                                 scoped_refptr<InfoMap> info_map) {
  base::ThreadRestrictions::AssertIOAllowed();

  std::unique_ptr<RulesetMatcher> ruleset_matcher =
      RulesetMatcher::CreateVerifiedMatcher(indexed_file_path);

  if (!ruleset_matcher)
    return;

  base::OnceClosure task = base::BindOnce(
      &LoadRulesetOnIOThread, std::move(extension_id), std::move(install_time),
      std::move(ruleset_matcher), base::RetainedRef(std::move(info_map)));

  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   std::move(task));
}

void UnloadRulesetOnIOThread(const std::string& extension_id,
                             InfoMap* info_map) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  info_map->GetRulesetManager()->RemoveRuleset(extension_id);
}

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

RulesMonitorService::RulesMonitorService(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      registry_observer_(this),
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  registry_observer_.Add(ExtensionRegistry::Get(browser_context_));
}

RulesMonitorService::~RulesMonitorService() = default;

// static
BrowserContextKeyedAPIFactory<RulesMonitorService>*
RulesMonitorService::GetFactoryInstance() {
  return g_factory.Pointer();
}

void RulesMonitorService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  LOG(ERROR) << "--------OnExtensionLoaded " << extension->id();
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_EQ(browser_context_, browser_context);

  if (!util::HasIndexedRuleset(extension))
    return;

  // Pass a weak pointer to the ruleset manager since while the function is
  // invoked on the |file_task_runner_|, |this| may be destroyed leading to the
  // posting of a task to delete |ruleset_manager_| on the IO thread.
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
  LOG(ERROR) << "--------RulesMonitorService::OnExtensionUnloaded "
             << extension->id();
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_EQ(browser_context_, browser_context);

  if (!util::HasIndexedRuleset(extension))
    return;

  // We forward first to |file_task_runner_| and then to IO, to ensure
  // AddRuleset is always followed by RemoveRuleset. Consider, OnExtensionLoaded
  // is immediately followed by unloading, then if we dont do this,
  // RemoveRuleset might be called first. COMMENT: Easier way to do this?
  base::OnceClosure task = base::BindOnce(
      &UnloadRulesetOnFileTaskRunner, extension->id(),
      make_scoped_refptr(ExtensionSystem::Get(browser_context)->info_map()));
  file_task_runner_->PostTask(FROM_HERE, std::move(task));
}

}  // namespace declarative_net_request
}  // namespace extensions
