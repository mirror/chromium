// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if ENABLE_BACKGROUND_TASK == 1

#include "chrome/browser/background_task_manager.h"

#include "base/logging.h"
#include "base/string_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/web_contents.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "googleurl/src/gurl.h"

// Key names.
static const wchar_t* kIdKey = L"id";
static const wchar_t* kUrlKey = L"url";

// static
void BackgroundTaskManager::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterListPref(prefs::kBackgroundTasks);
}

BackgroundTaskManager::BackgroundTaskManager(Browser* browser, Profile* profile)
    : browser_(browser),
      prefs_(profile->GetPrefs()) {
  LoadUserPrefs();
}

BackgroundTaskManager::~BackgroundTaskManager() {
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

void BackgroundTaskManager::LoadUserPrefs() {
  const ListValue* task_list = prefs_->GetList(prefs::kBackgroundTasks);
  if (!task_list) {
    return;
  }

  tasks_.clear();
  for (ListValue::const_iterator iter = task_list->begin();
       iter != task_list->end();
       ++iter) {
    if (!(*iter)->IsType(Value::TYPE_DICTIONARY)) {
      NOTREACHED();
      continue;
    }
    DictionaryValue* task_dict = static_cast<DictionaryValue*>(*iter);
    BackgroundTask task;
    if (!task_dict->GetString(kIdKey, &task.id) || task.id.empty() ||
        !task_dict->GetString(kUrlKey, &task.url) || task.url.empty()) {
      NOTREACHED();
      continue;
    }
    task.started = false;

    GURL task_url(task.url);
    tasks_[ConstructMapID(task_url, task.id)] = task;
  }
}

bool BackgroundTaskManager::RegisterTask(WebContents* source,
                                         const std::wstring& task_id,
                                         const std::wstring& task_url) {
  DCHECK(source);

  // Background task is not supported for "off the record" mode.
  if (!source->profile() || source->profile()->IsOffTheRecord()) {
    return false;
  }

  // Some sanity checks for arguments.
  if (task_id.empty() || task_url.empty()) {
    return false;
  }

  // Checks for cross-origin.
  GURL parsed_task_url(task_url);
  if (source->GetURL().GetOrigin() != parsed_task_url.GetOrigin()) {
    DLOG(ERROR) << "Cross origin not allowed for background task "
                << task_id.c_str() << std::endl;
    return false;
  }

  // Checks if the task has already been registered.
  std::wstring map_id = ConstructMapID(parsed_task_url, task_id);
  if (tasks_.find(map_id) != tasks_.end()) {
    return false;
  }

  // Adds the new task.
  BackgroundTask task;
  task.id = task_id;
  task.url = task_url;
  task.started = false;
  tasks_[map_id] = task;

  // Updates the prefs store.
  DictionaryValue* value = new DictionaryValue();
  value->Set(kIdKey, Value::CreateStringValue(task_id));
  value->Set(kUrlKey, Value::CreateStringValue(task_url));
  prefs_->GetMutableList(prefs::kBackgroundTasks)->Append(value);

  return true;
}

bool BackgroundTaskManager::UnregisterTask(WebContents* source,
                                           const std::wstring& task_id) {
  DCHECK(source);

  // Background task is not supported for "off the record" mode.
  if (!source->profile() || source->profile()->IsOffTheRecord()) {
    return false;
  }

  // Some sanity checks for arguments.
  if (task_id.empty()) {
    return false;
  }

  // Finds and removes the task.
  std::wstring map_id = ConstructMapID(source->GetURL(), task_id);
  BackgroundTaskMap::iterator iter = tasks_.find(map_id);
  if (iter == tasks_.end()) {
    return false;
  }
  tasks_.erase(iter);

  // Updates the prefs store.
  ListValue* task_list = prefs_->GetMutableList(prefs::kBackgroundTasks);
  for (ListValue::iterator iter = task_list->begin();
       iter != task_list->end();
       ++iter) {
    if (!(*iter)->IsType(Value::TYPE_DICTIONARY)) {
      NOTREACHED();
      continue;
    }
    DictionaryValue* task_dict = static_cast<DictionaryValue*>(*iter);

    std::wstring id, url;
    if (!task_dict->GetString(kIdKey, &id) || id.empty() ||
        !task_dict->GetString(kUrlKey, &url) || url.empty()) {
      NOTREACHED();
      continue;
    }

    GURL parsed_url(url);
    if (parsed_url.GetOrigin() == source->GetURL().GetOrigin() &&
        std::equal(id.begin(), id.end(), task_id.begin(),
                   CaseInsensitiveCompare<wchar_t>())) {
      task_list->Erase(iter);
      return true;
    }
  }

  return false;
}

bool BackgroundTaskManager::StartTask(WebContents* source,
                                      const std::wstring& task_id) {
  DCHECK(source);

  // Browser can be NULL during testing.
  if (!browser_) {
    return false;
  }

  // Finds the task.
  std::wstring map_id = ConstructMapID(source->GetURL(), task_id);
  BackgroundTaskMap::iterator iter = tasks_.find(map_id);
  if (iter == tasks_.end()) {
    return false;
  }

  // Has the task been already started?
  if (iter->second.started) {
    return false;
  }

  // Starts the task.
  // TODO: to be implemented.
  iter->second.started = true;

  return true;
}

bool BackgroundTaskManager::EndTask(WebContents* source,
                                    const std::wstring& task_id) {
  // TODO: to be implemented.
  return false;
}

#endif  // ENABLE_BACKGROUND_TASK == 1
