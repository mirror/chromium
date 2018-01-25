// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/problematic_programs_updater_win.h"

#include <utility>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "base/win/registry.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/conflicts/module_database_win.h"
#include "chrome/browser/conflicts/third_party_metrics_recorder_win.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {

// Converts a list of programs in json format to a vector of ProgramInfos.
std::vector<InstalledPrograms::ProgramInfo> ConvertToProgramInfoVector(
    const base::ListValue& programs) {
  std::vector<InstalledPrograms::ProgramInfo> result;

  for (const base::Value& element : programs.GetList()) {
    if (!element.is_dict())
      continue;

    const base::Value* name_value =
        element.FindKeyOfType("name", base::Value::Type::STRING);
    const base::Value* registry_is_hkcu_value =
        element.FindKeyOfType("registry_is_hkcu", base::Value::Type::BOOLEAN);
    const base::Value* registry_key_path_value =
        element.FindKeyOfType("registry_key_path", base::Value::Type::STRING);
    const base::Value* registry_wow64_access_value = element.FindKeyOfType(
        "registry_wow64_access", base::Value::Type::INTEGER);

    // If any is missing, skip this element.
    if (!name_value || !registry_is_hkcu_value || !registry_key_path_value ||
        !registry_wow64_access_value) {
      continue;
    }

    result.emplace_back(base::UTF8ToUTF16(name_value->GetString()),
                        registry_is_hkcu_value->GetBool() ? HKEY_CURRENT_USER
                                                          : HKEY_LOCAL_MACHINE,
                        base::UTF8ToUTF16(registry_key_path_value->GetString()),
                        registry_wow64_access_value->GetInt());
  }

  return result;
}

// Serializes a vector of ProgramInfos to a json list.
base::ListValue ConvertToListValue(
    const std::vector<InstalledPrograms::ProgramInfo>& programs) {
  base::ListValue result;

  for (const InstalledPrograms::ProgramInfo& program : programs) {
    base::DictionaryValue element;

    element.SetString("name", program.name);
    element.SetBoolean("registry_is_hkcu",
                       program.registry_root == HKEY_CURRENT_USER);
    element.SetString("registry_key_path", program.registry_key_path);
    element.SetInteger("registry_wow64_access", program.registry_wow64_access);

    result.GetList().push_back(std::move(element));
  }

  return result;
}

}  // namespace

const base::Feature kThirdPartyConflictsWarning{
    "ThirdPartyConflictsWarning", base::FEATURE_ENABLED_BY_DEFAULT};

ProblematicProgramsUpdater::~ProblematicProgramsUpdater() = default;

// static
std::unique_ptr<ProblematicProgramsUpdater>
ProblematicProgramsUpdater::MaybeCreate(
    const InstalledPrograms& installed_programs) {
  std::unique_ptr<ProblematicProgramsUpdater> instance;

  if (base::FeatureList::IsEnabled(kThirdPartyConflictsWarning))
    instance.reset(new ProblematicProgramsUpdater(installed_programs));

  return instance;
}

// static
bool ProblematicProgramsUpdater::TrimCache() {
  if (!base::FeatureList::IsEnabled(kThirdPartyConflictsWarning))
    return false;

  const base::ListValue& uninstallable_programs =
      *g_browser_process->local_state()->GetList(prefs::kProblematicPrograms);

  std::vector<InstalledPrograms::ProgramInfo> programs =
      ConvertToProgramInfoVector(uninstallable_programs);

  // Remove entries who can no longer be found in the registry.
  programs.erase(
      std::remove_if(programs.begin(), programs.end(),
                     [](const InstalledPrograms::ProgramInfo& element) {
                       base::win::RegKey registry_key(
                           element.registry_root,
                           element.registry_key_path.c_str(),
                           KEY_QUERY_VALUE | element.registry_wow64_access);
                       return !registry_key.Valid();
                     }),
      programs.end());

  // Write it back.
  g_browser_process->local_state()->Set(prefs::kProblematicPrograms,
                                        ConvertToListValue(programs));
  return !programs.empty();
}

// static
bool ProblematicProgramsUpdater::HasCachedPrograms() {
  if (!base::FeatureList::IsEnabled(kThirdPartyConflictsWarning))
    return false;

  return !g_browser_process->local_state()
              ->GetList(prefs::kProblematicPrograms)
              ->GetList()
              .empty();
}

// static
base::ListValue ProblematicProgramsUpdater::GetCachedProgramNames() {
  base::ListValue program_names;

  if (base::FeatureList::IsEnabled(kThirdPartyConflictsWarning)) {
    const base::ListValue& uninstallable_programs =
        *g_browser_process->local_state()->GetList(prefs::kProblematicPrograms);

    std::vector<InstalledPrograms::ProgramInfo> programs =
        ConvertToProgramInfoVector(uninstallable_programs);
    for (const auto& program : programs)
      // Only the name is returned
      program_names.GetList().push_back(base::Value(program.name));
  }

  return program_names;
}

void ProblematicProgramsUpdater::OnNewModuleFound(
    const ModuleInfoKey& module_key,
    const ModuleInfoData& module_data) {
  // Only consider loaded modules.
  if (module_data.module_types & ModuleInfoData::kTypeLoadedModule) {
    installed_programs_.GetInstalledPrograms(module_key.module_path,
                                             &programs_);
  }
}

void ProblematicProgramsUpdater::OnModuleDatabaseIdle() {
  base::ListValue list = ConvertToListValue(programs_);
  programs_.clear();

  if (overwrite_previous_list_) {
    // Directly write the list over the previous one.
    g_browser_process->local_state()->Set(prefs::kProblematicPrograms, list);
    overwrite_previous_list_ = false;
  } else {
    // Update the existing list.
    ListPrefUpdate update(g_browser_process->local_state(),
                          prefs::kProblematicPrograms);
    base::ListValue* new_list = update.Get();

    for (auto&& element : list)
      new_list->GetList().push_back(std::move(element));
  }
}

ProblematicProgramsUpdater::ProblematicProgramsUpdater(
    const InstalledPrograms& installed_programs)
    : installed_programs_(installed_programs) {}
