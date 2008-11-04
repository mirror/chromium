// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/background_task/background_task_manager.h"

#include "base/logging.h"
#include "base/path_service.h"
#include "base/registry.h"
#include "base/string_util.h"
#include "chrome/browser/background_task/background_task_runner.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profile_manager.h"
#include "chrome/browser/web_contents.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/browser/render_view_host.h"

// Key names.
static const wchar_t* kIdKey = L"id";
static const wchar_t* kUrlKey = L"url";
static const wchar_t* kStartModeKey = L"start";

RenderViewHost* BackgroundTask::render_view_host() const {
  return runner ? runner->render_view_host() : NULL;
}

// static
void BackgroundTaskManager::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterListPref(prefs::kBackgroundTasks);
}

// static
void BackgroundTaskManager::GetAllTasks(std::vector<BackgroundTask*>* tasks) {
  DCHECK(tasks);

  tasks->clear();
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager) {
    return;
  }

  for (ProfileManager::const_iterator profile_iter(profile_manager->begin());
       profile_iter != profile_manager->end();
       ++profile_iter) {    
    BackgroundTaskManager* bgtask_manager =
        (*profile_iter)->GetBackgroundTaskManager();
    if (!bgtask_manager) {
      continue;
    }

    for (BackgroundTaskMap::iterator task_iter(bgtask_manager->tasks_.begin());
         task_iter != bgtask_manager->tasks_.end();
         ++task_iter) {
      tasks->push_back(&(task_iter->second));
    }
  }
}

// static
void BackgroundTaskManager::CloseAllActiveTasks() {
  std::vector<BackgroundTask*> tasks;
  GetAllTasks(&tasks);

  for (std::vector<BackgroundTask*>::iterator iter(tasks.begin());
       iter != tasks.end();
       ++iter) {
     if ((*iter)->runner) {
       (*iter)->runner->Shutdown();
     }
  }
}

// static
bool BackgroundTaskManager::HasActiveTask() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager) {
    return false;
  }

  for (ProfileManager::const_iterator profile_iter(profile_manager->begin());
       profile_iter != profile_manager->end();
       ++profile_iter) {  
    BackgroundTaskManager* bgtask_manager =
        (*profile_iter)->GetBackgroundTaskManager();
    if (!bgtask_manager) {
      continue;
    }
    if (bgtask_manager->active_task_count_ > 0) {
      return true;
    }
  }

  return false;
}

BackgroundTaskManager::BackgroundTaskManager(Profile* profile)
    : profile_(profile),
      active_task_count_(0) {
}

BackgroundTaskManager::~BackgroundTaskManager() {
  // Makes sure that we've already terminated all running tasks.
#ifdef DEBUG
  for (BackgroundTaskMap::const_iterator iter(tasks_.begin());
       iter != tasks_.end();
       ++iter) {
    if (iter->second.runner) {
      NOTREACHED();
      break;
    }
  }
#endif  // DEBUG
}

std::wstring BackgroundTaskManager::ConstructMapID(const GURL& url,
                                                   const std::wstring& id) {
  // Since the task ID is unique in the given origin, we construct the map ID
  // in the following form:
  //      origin:id
  std::wstring map_id = UTF8ToWide(url.GetOrigin().spec());
  map_id += L":";
  map_id += id;
  StringToLowerASCII(map_id);
  return map_id;
}

bool BackgroundTaskManager::LoadAndStartAllTasks(
    bool start_only_tasks_on_login) {
  const ListValue* task_list =
      profile_->GetPrefs()->GetList(prefs::kBackgroundTasks);
  if (!task_list) {
    return false;
  }

  bool task_started = false;
  tasks_.clear();
  for (ListValue::const_iterator iter = task_list->begin();
       iter != task_list->end();
       ++iter) {
    if (!(*iter)->IsType(Value::TYPE_DICTIONARY)) {
      NOTREACHED();
      continue;
    }
    DictionaryValue* task_dict = static_cast<DictionaryValue*>(*iter);
    std::wstring id, url;
    BackgroundTaskStartMode start_mode = START_BACKGROUND_TASK_ON_DEMAND;
    if (!task_dict->GetString(kIdKey, &id) || id.empty() ||
        !task_dict->GetString(kUrlKey, &url) || url.empty() ||
        !task_dict->GetInteger(kStartModeKey,
                               reinterpret_cast<int*>(&start_mode))) {
      NOTREACHED();
      continue;
    }

    GURL parsed_url(url);
    BackgroundTask task(id, parsed_url, start_mode);
    std::wstring map_id = ConstructMapID(task.url, task.id);
    tasks_[map_id] = task;

    bool to_start = start_only_tasks_on_login ?
      (start_mode == START_BACKGROUND_TASK_ON_LOGIN) :
      (start_mode != START_BACKGROUND_TASK_ON_DEMAND);
    if (to_start) {
      task_started = true;
      BackgroundTaskRunner* runner =
          new BackgroundTaskRunner(this, &(tasks_.find(map_id)->second));
      runner->Start();
    }
  }

  return task_started;
}

bool BackgroundTaskManager::EnableChromeStartup() {
  RegKey key(HKEY_CURRENT_USER,
             chrome::kChromeStartupKey,
             KEY_READ | KEY_WRITE);
  if (!key.Valid()) {
    return false;
  }

  if (key.ValueExists(chrome::kChromeStartupValue)) {
    return true;
  }

  std::wstring app_path;
  if (!PathService::Get(base::FILE_EXE, &app_path)) {
    return false;
  }

  std::wstring cmd_line(L"\"");
  cmd_line.append(app_path);
  cmd_line.append(L"\"");

  std::wstring user_data_dir;
  if (PathService::Get(chrome::DIR_USER_DATA, &user_data_dir)) {
    cmd_line.append(L" --");
    cmd_line.append(switches::kUserDataDir);
    cmd_line.append(L"=\"");
    cmd_line.append(user_data_dir);
    cmd_line.append(L"\"");
  }

  cmd_line.append(L" --");
  cmd_line.append(switches::kStartup);

  // TODO(jianli): support multiple profiles.
  return key.WriteValue(chrome::kChromeStartupValue, cmd_line.c_str());
}

bool BackgroundTaskManager::DisableChromeStartup() {
  RegKey key(HKEY_CURRENT_USER, chrome::kChromeStartupKey, KEY_WRITE);
  if (!key.Valid()) {
    return false;
  }

  return key.DeleteValue(chrome::kChromeStartupValue);
}

bool BackgroundTaskManager::RegisterTask(WebContents* source,
                                         const std::wstring& id,
                                         const GURL& url,
                                         BackgroundTaskStartMode start_mode) {
  DCHECK(source);

  // Background task is not supported for "off the record" mode.
  if (!source->profile() || source->profile()->IsOffTheRecord()) {
    return false;
  }

  // Some sanity checks for arguments.
  if (id.empty() || url.is_empty() || !url.is_valid()) {
    return false;
  }

  // Checks for cross-origin.
  if (source->GetURL().GetOrigin() != url.GetOrigin()) {
    DLOG(ERROR) << "Cross origin not allowed for background task "
                << id.c_str() << std::endl;
    return false;
  }

  // Checks if the task has already been registered.
  std::wstring map_id = ConstructMapID(url, id);
  if (tasks_.find(map_id) != tasks_.end()) {
    return false;
  }

  // Adds the new task.
  BackgroundTask task(id, url, start_mode);
  tasks_[map_id] = task;

  // Updates the prefs store.
  DictionaryValue* value = new DictionaryValue();
  value->Set(kIdKey, Value::CreateStringValue(id));
  value->Set(kUrlKey, Value::CreateStringValue(UTF8ToWide(url.spec())));
  value->Set(kStartModeKey,
             Value::CreateIntegerValue(static_cast<int>(start_mode)));
  profile_->GetPrefs()->GetMutableList(prefs::kBackgroundTasks)->Append(value);
  profile_->GetPrefs()->ScheduleSavePersistentPrefs(
      g_browser_process->file_thread());

  // Enables Chrome to auto-start if this task is set to start on login.
  if (start_mode == START_BACKGROUND_TASK_ON_LOGIN) {
    EnableChromeStartup();
  }

  return true;
}

bool BackgroundTaskManager::UnregisterTask(WebContents* source,
                                           const std::wstring& id) {
  DCHECK(source);

  // Background task is not supported for "off the record" mode.
  if (!source->profile() || source->profile()->IsOffTheRecord()) {
    return false;
  }

  // Some sanity checks for arguments.
  if (id.empty()) {
    return false;
  }

  // Finds and removes the task.
  std::wstring map_id = ConstructMapID(source->GetURL(), id);
  BackgroundTaskMap::iterator iter = tasks_.find(map_id);
  if (iter == tasks_.end()) {
    return false;
  }
  BackgroundTaskStartMode start_mode = iter->second.start_mode;
  tasks_.erase(iter);

  // Updates the prefs store.
  ListValue* task_list =
      profile_->GetPrefs()->GetMutableList(prefs::kBackgroundTasks);
  for (ListValue::iterator iter = task_list->begin();
       iter != task_list->end();
       ++iter) {
    if (!(*iter)->IsType(Value::TYPE_DICTIONARY)) {
      NOTREACHED();
      continue;
    }
    DictionaryValue* task_dict = static_cast<DictionaryValue*>(*iter);

    std::wstring current_id, current_url;
    if (!task_dict->GetString(kIdKey, &current_id) || current_id.empty() ||
        !task_dict->GetString(kUrlKey, &current_url) || current_url.empty()) {
      NOTREACHED();
      continue;
    }

    GURL parsed_current_url(current_url);
    if (parsed_current_url.GetOrigin() == source->GetURL().GetOrigin() &&
        std::equal(current_id.begin(), current_id.end(), id.begin(),
                   CaseInsensitiveCompare<wchar_t>())) {
      task_list->Erase(iter);
      profile_->GetPrefs()->ScheduleSavePersistentPrefs(
          g_browser_process->file_thread());
      return true;
    }
  }

  // Stops Chrome from being auto-started if no task is set to start on
  // login.
  if (start_mode == START_BACKGROUND_TASK_ON_LOGIN) {
    bool disable_chrome_startup = true;
    for (BackgroundTaskMap::const_iterator iter(tasks_.begin());
         iter != tasks_.end();
         ++iter) {
      if (iter->second.start_mode == START_BACKGROUND_TASK_ON_LOGIN) {
        disable_chrome_startup = false;
        break;
      }
    }
    if (disable_chrome_startup) {
      DisableChromeStartup();
    }
  }

  return false;
}

bool BackgroundTaskManager::StartTask(WebContents* source,
                                      const std::wstring& id) {
  DCHECK(source);

  // Finds the task.
  std::wstring map_id = ConstructMapID(source->GetURL(), id);
  BackgroundTaskMap::iterator iter = tasks_.find(map_id);
  if (iter == tasks_.end()) {
    return false;
  }

  // Has the task been already started?
  if (iter->second.render_view_host()) {
    return false;
  }

  // Starts the task.
  BackgroundTaskRunner* runner =
      new BackgroundTaskRunner(this, &(iter->second));
  runner->Start();

  return true;
}

bool BackgroundTaskManager::EndTask(WebContents* source,
                                    const std::wstring& id) {
  DCHECK(source);

  // Finds the task.
  std::wstring map_id = ConstructMapID(source->GetURL(), id);
  BackgroundTaskMap::iterator iter = tasks_.find(map_id);
  if (iter == tasks_.end()) {
    return false;
  }

  // Has the task been started?
  if (!iter->second.render_view_host()) {
    return true;
  }

  // Shuts down the task runner.
  iter->second.runner->Shutdown();

  return true;
}

BackgroundTask* BackgroundTaskManager::FindTask(const std::wstring& id,
                                                const GURL& url) {
  std::wstring map_id = ConstructMapID(url, id);
  BackgroundTaskMap::iterator iter = tasks_.find(map_id);
  return iter != tasks_.end() ? &(iter->second) : NULL;
}

bool BackgroundTaskManager::IsTaskRunning(WebContents* source,
                                          const std::wstring& id) const {
  DCHECK(source);

  BackgroundTask* task =
      const_cast<BackgroundTaskManager*>(this)->FindTask(id, source->GetURL());
  return task && task->runner != NULL;
}

#endif  // ENABLE_BACKGROUND_TASK
