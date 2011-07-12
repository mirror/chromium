// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/task_manager_handler.h"

#include <algorithm>
#include <functional>
#include "base/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/task_manager/task_manager.h"
#include "chrome/browser/ui/webui/web_ui_util.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace {

static Value* CreateColumnValue(const TaskManagerModel* tm,
                                const std::string column_name,
                                const int i) {
  if (column_name == "processId")
    return Value::CreateStringValue(tm->GetResourceProcessId(i));
  if (column_name == "processIdValue")
    return Value::CreateIntegerValue(tm->GetProcessId(i));
  if (column_name == "cpuUsage")
    return Value::CreateStringValue(tm->GetResourceCPUUsage(i));
  if (column_name == "cpuUsageValue")
    return Value::CreateDoubleValue(tm->GetCPUUsage(i));
  if (column_name == "privateMemory")
    return Value::CreateStringValue(tm->GetResourcePrivateMemory(i));
  if (column_name == "privateMemoryValue") {
    size_t private_memory;
    tm->GetPrivateMemory(i, &private_memory);
    return Value::CreateDoubleValue(private_memory);
  }
  if (column_name == "icon")
    return Value::CreateStringValue(
               web_ui_util::GetImageDataUrl(tm->GetResourceIcon(i)));
  if (column_name == "title")
    return Value::CreateStringValue(tm->GetResourceTitle(i));
  if (column_name == "networkUsage")
    return Value::CreateStringValue(tm->GetResourceNetworkUsage(i));
  if (column_name == "networkUsageValue")
    return Value::CreateDoubleValue(tm->GetNetworkUsage(i));
  if (column_name == "networkUsage")
    return Value::CreateStringValue(tm->GetResourceNetworkUsage(i));

  NOTREACHED();
  return NULL;
}

static void CreateGroupColomnList(const TaskManagerModel* tm,
                                  const std::string column_name,
                                  const int index,
                                  const int length,
                                  DictionaryValue* val) {
  ListValue *list = new ListValue();
  for (int i = index; i < (index + length); i++) {
    list->Append(CreateColumnValue(tm, column_name, i));
  }
  val->Set(column_name, list);
}

static DictionaryValue* CreateTaskGroupValue(const TaskManagerModel* tm,
                                            const int group_index) {
  DictionaryValue* val = new DictionaryValue();

  const int group_count = tm->GroupCount();
  if (group_index >= group_count)
     return val;

  int index = tm->GetResourceIndexForGroup(group_index, 0);
  std::pair<int, int> group_range;
  group_range = tm->GetGroupRangeForResource(index);
  int length = group_range.second;

  val->SetInteger("index", index);
  val->SetInteger("group_range_start", group_range.first);
  val->SetInteger("group_range_length", group_range.second);
  val->SetInteger("index_in_group", index - group_range.first);
  val->SetBoolean("is_resource_first_in_group",
                  tm->IsResourceFirstInGroup(index));

  // Columns which have one datum in each group.
  CreateGroupColomnList(tm, "processId", index, 1, val);
  CreateGroupColomnList(tm, "processIdValue", index, 1, val);
  CreateGroupColomnList(tm, "cpuUsage", index, 1, val);
  CreateGroupColomnList(tm, "cpuUsageValue", index, 1, val);
  CreateGroupColomnList(tm, "privateMemory", index, 1, val);
  CreateGroupColomnList(tm, "privateMemoryValue", index, 1, val);

  // Columns which have some data in each group.
  CreateGroupColomnList(tm, "icon", index, length, val);
  CreateGroupColomnList(tm, "title", index, length, val);
  CreateGroupColomnList(tm, "networkUsage", index, length, val);
  CreateGroupColomnList(tm, "networkUsageValue", index, length, val);

  return val;
}

}  // namespace

TaskManagerHandler::TaskManagerHandler(TaskManager* tm)
    : task_manager_(tm),
      model_(tm->model()),
      is_enabled_(false) {
}

TaskManagerHandler::~TaskManagerHandler() {
  DisableTaskManager(NULL);
}

// TaskManagerHandler, public: -----------------------------------------------

void TaskManagerHandler::OnModelChanged() {
  const int count = model_->GroupCount();

  FundamentalValue start_value(0);
  FundamentalValue length_value(count);
  ListValue tasks_value;
  for (int i = 0; i < count; i++) {
    tasks_value.Append(CreateTaskGroupValue(model_, i));
  }
  if (is_enabled_) {
    web_ui_->CallJavascriptFunction("taskChanged",
                                    start_value, length_value, tasks_value);
  }
}

void TaskManagerHandler::OnItemsChanged(const int start, const int length) {
  UpdateResourceGroupTable(start, length);

  // Converts from an index of resources to an index of groups.
  int group_start = model_->GetGroupIndexForResource(start);
  int group_end = model_->GetGroupIndexForResource(start + length - 1);

  OnGroupChanged(group_start, group_end - group_start + 1);
}

void TaskManagerHandler::OnItemsAdded(const int start, const int length) {
  UpdateResourceGroupTable(start, length);

  // Converts from an index of resources to an index of groups.
  int group_start = model_->GetGroupIndexForResource(start);
  int group_end = model_->GetGroupIndexForResource(start + length - 1);

  // First group to add does not contain all the items in the group. Because the
  // first item to add and the previous one are in same group.
  if (!model_->IsResourceFirstInGroup(start)) {
    OnGroupChanged(group_start, 1);
    if (group_start == group_end)
      return;
    else
      group_start++;
  }

  // Last group to add does not contain all the items in the group. Because the
  // last item to add and the next one are in same group.
  if (!model_->IsResourceLastInGroup(start + length - 1)) {
    OnGroupChanged(group_end, 1);
    if (group_start == group_end)
      return;
    else
      group_end--;
  }

  OnGroupAdded(group_start, group_end - group_start + 1);
}

void TaskManagerHandler::OnItemsRemoved(const int start, const int length) {
  // Converts from an index of resources to an index of groups.
  int group_start = resource_to_group_table_[start];
  int group_end = resource_to_group_table_[start + length - 1];

  // First group to remove does not contain all the items in the group. Because
  // the first item to remove and the previous one are in same group.
  if (start != 0 && group_start == resource_to_group_table_[start - 1]) {
    OnGroupChanged(group_start, 1);
    if (group_start == group_end)
      return;
    else
      group_start++;
  }

  // Last group to remove does not contain all the items in the group. Because
  // the last item to remove and the next one are in same group.
  if (start + length != model_->ResourceCount() &&
      group_end == resource_to_group_table_[start + length]) {
    OnGroupChanged(group_end, 1);
    if (group_start == group_end)
      return;
    else
      group_end--;
  }

  std::vector<int>::iterator it_first =
      resource_to_group_table_.begin() + start;
  std::vector<int>::iterator it_last = it_first + length - 1;
  resource_to_group_table_.erase(it_first, it_last);

  OnGroupRemoved(group_start, group_end - group_start + 1);
}

void TaskManagerHandler::Init() {
}

void TaskManagerHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback("killProcess",
      NewCallback(this, &TaskManagerHandler::HandleKillProcess));
  web_ui_->RegisterMessageCallback("openAboutMemory",
      NewCallback(this, &TaskManagerHandler::OpenAboutMemory));
  web_ui_->RegisterMessageCallback("enableTaskManager",
      NewCallback(this, &TaskManagerHandler::EnableTaskManager));
  web_ui_->RegisterMessageCallback("disableTaskManager",
      NewCallback(this, &TaskManagerHandler::DisableTaskManager));
}

void TaskManagerHandler::HandleKillProcess(const ListValue* indexes) {
  for (ListValue::const_iterator i = indexes->begin();
       i != indexes->end(); i++) {
    int index = -1;
    string16 string16_index;
    double double_index;
    if ((*i)->GetAsString(&string16_index)) {
      base::StringToInt(string16_index, &index);
    } else if ((*i)->GetAsDouble(&double_index)) {
      index = static_cast<int>(double_index);
    } else {
      (*i)->GetAsInteger(&index);
    }
    if (index == -1)
      continue;

    int resource_index = model_->GetResourceIndexForGroup(index, 0);
    if (resource_index == -1)
      continue;

    LOG(INFO) << "kill PID:" << model_->GetResourceProcessId(resource_index);
    task_manager_->KillProcess(resource_index);
  }
}

void TaskManagerHandler::DisableTaskManager(const ListValue* indexes) {
  if (!is_enabled_)
    return;

  is_enabled_ = false;
  model_->StopUpdating();
  model_->RemoveObserver(this);
}

void TaskManagerHandler::EnableTaskManager(const ListValue* indexes) {
  if (is_enabled_)
    return;

  is_enabled_ = true;
  model_->AddObserver(this);
  model_->StartUpdating();
}

void TaskManagerHandler::OpenAboutMemory(const ListValue* indexes) {
  task_manager_->OpenAboutMemory();
}

// TaskManagerHandler, private: -----------------------------------------------

void TaskManagerHandler::UpdateResourceGroupTable(int start, int length) {
  if (resource_to_group_table_.size() < static_cast<size_t>(start)) {
    length += start - resource_to_group_table_.size();
    start = resource_to_group_table_.size();
  }

  // Makes room to fill group_index at first since inserting vector is too slow.
  std::vector<int>::iterator it = resource_to_group_table_.begin() + start;
  resource_to_group_table_.insert(it, static_cast<size_t>(length), -1);

  for (int i = start; i < start + length; i++) {
    const int group_index = model_->GetGroupIndexForResource(i);
    resource_to_group_table_[i] = group_index;
  }
}

void TaskManagerHandler::OnGroupChanged(const int group_start,
                                        const int group_length) {
  FundamentalValue start_value(group_start);
  FundamentalValue length_value(group_length);
  ListValue tasks_value;

  for (int i = 0; i < group_length; i++) {
    tasks_value.Append(CreateTaskGroupValue(model_, group_start + i));
  }
  if (is_enabled_) {
    web_ui_->CallJavascriptFunction("taskChanged",
                                    start_value, length_value, tasks_value);
  }
}

void TaskManagerHandler::OnGroupAdded(const int group_start,
                                      const int group_length) {
  FundamentalValue start_value(group_start);
  FundamentalValue length_value(group_length);
  ListValue tasks_value;
  for (int i = 0; i < group_length; i++) {
    tasks_value.Append(CreateTaskGroupValue(model_, group_start + i));
  }
  if (is_enabled_) {
    web_ui_->CallJavascriptFunction("taskAdded",
                                    start_value, length_value, tasks_value);
  }
}

void TaskManagerHandler::OnGroupRemoved(const int group_start,
                                        const int group_length) {
  FundamentalValue start_value(group_start);
  FundamentalValue length_value(group_length);
  if (is_enabled_)
    web_ui_->CallJavascriptFunction("taskRemoved", start_value, length_value);
}

