// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/test/tab_socket_test.h"

#include "headless/public/headless_devtools_client.h"

namespace headless {

TabSocketTest::TabSocketTest() {}
TabSocketTest::~TabSocketTest() {}

void TabSocketTest::SetUp() {
  options()->mojo_service_names.insert("headless::TabSocket");
  HeadlessAsyncDevTooledBrowserTest::SetUp();
}

void TabSocketTest::RunDevTooledTest() {
  // Note we must enable the page domain or the browser won't get told the
  // devtools frame ids.
  devtools_client_->GetPage()->AddObserver(this);
  devtools_client_->GetPage()->Enable();
  devtools_client_->GetRuntime()->AddObserver(this);
  devtools_client_->GetRuntime()->Enable();

  devtools_client_->GetPage()->GetExperimental()->GetResourceTree(
      page::GetResourceTreeParams::Builder().Build(),
      base::Bind(&TabSocketTest::OnResourceTree, base::Unretained(this)));
}

void TabSocketTest::OnResourceTree(
    std::unique_ptr<page::GetResourceTreeResult> result) {
  main_frame_id_ = result->GetFrameTree()->GetFrame()->GetId();
  RunTabSocketTest();
}

void TabSocketTest::OnExecutionContextCreated(
    const runtime::ExecutionContextCreatedParams& params) {
  const base::DictionaryValue* dictionary;
  bool is_main_world;
  if (params.GetContext()->HasAuxData() &&
      params.GetContext()->GetAuxData()->GetAsDictionary(&dictionary)) {
    if (dictionary->GetBoolean("isDefault", &is_main_world) && !is_main_world) {
      world_name_to_v8_execution_context_id_[params.GetContext()->GetName()] =
          params.GetContext()->GetId();

      auto find_it =
          pending_world_name_callbacks_.find(params.GetContext()->GetName());
      if (find_it != pending_world_name_callbacks_.end()) {
        find_it->second.Run(params.GetContext()->GetId());
        pending_world_name_callbacks_.erase(find_it);
      }
    }
    std::string frame_id;
    if (dictionary->GetString("frameId", &frame_id)) {
      frame_id_to_v8_execution_context_ids_[frame_id].insert(
          params.GetContext()->GetId());

      auto find_it =
          pending_frame_id_callbacks_.find(params.GetContext()->GetName());
      if (find_it != pending_frame_id_callbacks_.end()) {
        find_it->second.Run(params.GetContext()->GetId());
        pending_frame_id_callbacks_.erase(find_it);
      }
    }
  }
}

void TabSocketTest::ExpectJsException(
    std::unique_ptr<runtime::EvaluateResult> result) {
  EXPECT_TRUE(result->HasExceptionDetails());
  FinishAsynchronousTest();
}

void TabSocketTest::FailOnJsEvaluateException(
    std::unique_ptr<runtime::EvaluateResult> result) {
  if (!result->HasExceptionDetails())
    return;

  FinishAsynchronousTest();

  const runtime::ExceptionDetails* exception_details =
      result->GetExceptionDetails();
  FAIL() << exception_details->GetText()
         << (exception_details->HasException()
                 ? exception_details->GetException()->GetDescription().c_str()
                 : "");
}

bool TabSocketTest::GetAllowTabSockets() {
  return true;
}

int TabSocketTest::GetV8ExecutionContextIdByWorldName(const std::string& name) {
  const auto find_it = world_name_to_v8_execution_context_id_.find(name);
  if (find_it == world_name_to_v8_execution_context_id_.end()) {
    FinishAsynchronousTest();
    CHECK(false) << "Unknown isolated world: " << name;
    return -1;
  }
  return find_it->second;
}

const std::set<int>* TabSocketTest::GetV8ExecutionContextIdsForFrame(
    const std::string& devtools_frame_id) const {
  const auto find_it =
      frame_id_to_v8_execution_context_ids_.find(devtools_frame_id);
  if (find_it == frame_id_to_v8_execution_context_ids_.end())
    return nullptr;
  return &find_it->second;
}

void TabSocketTest::CreateMainWorldTabSocket(
    std::string devtools_frame_id,
    base::Callback<void(int)> callback) {
  const auto find_it =
      frame_id_to_v8_execution_context_ids_.find(devtools_frame_id);
  if (find_it == frame_id_to_v8_execution_context_ids_.end()) {
    // We need to wait until notification of the creation of the execution
    // context for |devtools_frame_id| has been received.
    pending_frame_id_callbacks_.insert(std::make_pair(
        devtools_frame_id,
        base::Bind(&TabSocketTest::InstallHeadlessTabSocketBindings,
                   base::Unretained(this), devtools_frame_id, callback)));
  } else {
    if (find_it->second.size() != 1u) {
      FinishAsynchronousTest();
      FAIL() << "More than one v8 execution context exists for the main frame!";
    }
    InstallHeadlessTabSocketBindings(devtools_frame_id, callback,
                                     *find_it->second.begin());
  }
}

void TabSocketTest::CreateIsolatedWorldTabSocket(
    std::string isolated_world_name,
    std::string devtools_frame_id,
    base::Callback<void(int)> callback) {
  // We need to know the execution context id of the world we're about to create
  // so insert a callback into |pending_world_name_callbacks_| which should get
  // executed by TabSocketTest::OnExecutionContextCreated.
  // TODO(alexclarke): Make CreateIsolatedWorld return the execution context id.
  pending_world_name_callbacks_.insert(std::make_pair(
      isolated_world_name,
      base::Bind(&TabSocketTest::InstallHeadlessTabSocketBindings,
                 base::Unretained(this), devtools_frame_id, callback)));

  devtools_client_->GetPage()->GetExperimental()->CreateIsolatedWorld(
      page::CreateIsolatedWorldParams::Builder()
          .SetFrameId(devtools_frame_id)
          .SetWorldName(isolated_world_name)
          .Build());
}

void TabSocketTest::InstallHeadlessTabSocketBindings(
    std::string devtools_frame_id,
    base::Callback<void(int)> callback,
    int execution_context_id) {
  web_contents_->InstallHeadlessTabSocketBindings(
      devtools_frame_id, execution_context_id,
      base::Bind(&TabSocketTest::OnInstalledHeadlessTabSocketBindings,
                 base::Unretained(this), execution_context_id, callback));
}

void TabSocketTest::OnInstalledHeadlessTabSocketBindings(
    int execution_context_id,
    base::Callback<void(int)> callback,
    bool success) {
  if (!success) {
    FinishAsynchronousTest();
    CHECK(false) << "InstallHeadlessTabSocketBindings failed";
  }
  callback.Run(execution_context_id);
}

}  // namespace headless
