// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/background_task_manager.h"

#include "base/logging.h"
#include "base/string_util.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profile_manager.h"
#include "chrome/browser/web_contents.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_types.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/browser/render_view_host.h"
#include "chrome/browser/render_widget_host_view_win.h"

// Key names.
static const wchar_t* kIdKey = L"id";
static const wchar_t* kUrlKey = L"url";
static const wchar_t* kStartModeKey = L"start";

// static
void BackgroundTaskManager::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterListPref(prefs::kBackgroundTasks);
}

// static
void BackgroundTaskManager::GetAllBackgroundTasks(
    std::vector<BackgroundTask*>* tasks) {
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
  GetAllBackgroundTasks(&tasks);

  for (std::vector<BackgroundTask*>::iterator iter(tasks.begin());
       iter != tasks.end();
       ++iter) {
     if ((*iter)->runner) {
       (*iter)->runner->Shutdown();
     }
  }
}

BackgroundTaskManager::BackgroundTaskManager(Profile* profile)
    : profile_(profile) {
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

void BackgroundTaskManager::LoadAndStartAllRegisteredTasks() {
  const ListValue* task_list =
      profile_->GetPrefs()->GetList(prefs::kBackgroundTasks);
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
    std::wstring id, url;
    BackgroundTaskStartMode start_mode = START_BACKGROUND_TASK_ON_DEMAND;
    if (!task_dict->GetString(kIdKey, &id) || id.empty() ||
        !task_dict->GetString(kUrlKey, &url) || url.empty() ||
        !task_dict->GetInteger(kStartModeKey,
                               reinterpret_cast<int*>(&start_mode))) {
      NOTREACHED();
      continue;
    }

    BackgroundTask task(id, url, start_mode);
    std::wstring map_id = ConstructMapID(task.url, task.id);
    tasks_[map_id] = task;

    // Starts the task if not configured as start on demand.
    if (task.start_mode != START_BACKGROUND_TASK_ON_DEMAND) {
      BackgroundTaskRunner* runner =
          new BackgroundTaskRunner(this, &(tasks_.find(map_id)->second));
      runner->Start();
    }
  }
}

bool BackgroundTaskManager::RegisterTask(WebContents* source,
                                         const std::wstring& id,
                                         const std::wstring& url,
                                         BackgroundTaskStartMode start_mode) {
  DCHECK(source);

  // Background task is not supported for "off the record" mode.
  if (!source->profile() || source->profile()->IsOffTheRecord()) {
    return false;
  }

  // Some sanity checks for arguments.
  if (id.empty() || url.empty()) {
    return false;
  }

  // Checks for cross-origin.
  GURL parsed_url(url);
  if (source->GetURL().GetOrigin() != parsed_url.GetOrigin()) {
    DLOG(ERROR) << "Cross origin not allowed for background task "
                << id.c_str() << std::endl;
    return false;
  }

  // Checks if the task has already been registered.
  std::wstring map_id = ConstructMapID(parsed_url, id);
  if (tasks_.find(map_id) != tasks_.end()) {
    return false;
  }

  // Adds the new task.
  BackgroundTask task(id, url, start_mode);
  tasks_[map_id] = task;

  // Updates the prefs store.
  DictionaryValue* value = new DictionaryValue();
  value->Set(kIdKey, Value::CreateStringValue(id));
  value->Set(kUrlKey, Value::CreateStringValue(url));
  value->Set(kStartModeKey,
             Value::CreateIntegerValue(static_cast<int>(start_mode)));
  profile_->GetPrefs()->GetMutableList(prefs::kBackgroundTasks)->Append(value);
  profile_->GetPrefs()->ScheduleSavePersistentPrefs(
      g_browser_process->file_thread());

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

// static
BackgroundTask* BackgroundTaskRunner::GetBackgroundTaskByID(
    int render_process_id, int render_view_id) {
  RenderViewHost* render_view_host =
      RenderViewHost::FromID(render_process_id, render_view_id);
  if (!render_view_host) {
    return NULL;
  }

  return static_cast<BackgroundTaskRunner*>(render_view_host->delegate())
      ->background_task_;
}

BackgroundTaskRunner::BackgroundTaskRunner(
    BackgroundTaskManager* background_task_manager,
    BackgroundTask* background_task)
    : background_task_manager_(background_task_manager),
      background_task_(background_task),
      render_view_host_(NULL),
      notify_disconnection_(false),
      method_factory_(this) {
  DCHECK(background_task_ && !background_task_->runner);
  background_task_->runner = this;
}

void BackgroundTaskRunner::Shutdown() {
  NotifyDisconnected();

  // Need to do this asynchronously as it will close the RenderViewHost, which
  // is currently on the call stack above us.
  MessageLoop::current()->PostTask(FROM_HERE,
      method_factory_.NewRunnableMethod(
          &BackgroundTaskRunner::DelayedShutdown));
}

void BackgroundTaskRunner::DelayedShutdown() {
  if (render_view_host_) {
    render_view_host_->Shutdown();
    render_view_host_ = NULL;
  }

  background_task_->runner = NULL;
  delete this;
}

bool BackgroundTaskRunner::Start() {
  SiteInstance* task_instance =
      SiteInstance::CreateSiteInstance(background_task_manager_->profile());
  task_instance->SetSite(background_task_->url.GetOrigin());
  
  render_view_host_ = new RenderViewHost(task_instance,
                                         this,
                                         MSG_ROUTING_NONE,
                                         NULL);

  // TODO (jianli): remove HWND since we don't render background task.
  RenderWidgetHostViewWin* view =
      new RenderWidgetHostViewWin(render_view_host_);
  render_view_host_->set_view(view);
  HWND root_hwnd = NULL;
  if (BrowserList::GetLastActive()) {
    root_hwnd = BrowserList::GetLastActive()->GetTopLevelHWND();
  }
  if (!root_hwnd) {
    NOTREACHED();
    return false;
  }
  view->Create(root_hwnd);

  render_view_host_->CreateRenderView();
  render_view_host_->NavigateToURL(background_task_->url);

  return true;
}

WebPreferences BackgroundTaskRunner::GetWebkitPrefs() {
  return WebPreferences();
}

Profile* BackgroundTaskRunner::GetProfile() const {
  return background_task_manager_->profile();
}

void BackgroundTaskRunner::RendererReady(RenderViewHost* render_view_host) {
  NotifyConnected();
}

void BackgroundTaskRunner::RendererGone(RenderViewHost* render_view_host) {
  Shutdown();
}

void BackgroundTaskRunner::UpdateTitle(RenderViewHost* render_view_host,
                                       int32 page_id,
                                       const std::wstring& title) {
  title_ = title;
}

void BackgroundTaskRunner::NotifyConnected() {
  notify_disconnection_ = true;
  NotificationService::current()->
      Notify(NOTIFY_BACKGROUND_TASK_CONNECTED,
             Source<BackgroundTask>(background_task_),
             NotificationService::NoDetails());
}

void BackgroundTaskRunner::NotifyDisconnected() {
  if (!notify_disconnection_) {
    return;
  }
  notify_disconnection_ = false;
  NotificationService::current()->
      Notify(NOTIFY_BACKGROUND_TASK_DISCONNECTED,
             Source<BackgroundTask>(background_task_),
             NotificationService::NoDetails());
}

#endif  // ENABLE_BACKGROUND_TASK
